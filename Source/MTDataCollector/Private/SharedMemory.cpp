#include "SharedMemory.h"

#include <tchar.h>

constexpr char GSharedmem_Base_Path[] = "MTDC_MEM";


#if PLATFORM_WINDOWS
// Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
	// Get the error message, if any.
	DWORD const ErrorMessageID = GetLastError();
	if (ErrorMessageID == 0) {
		return "NO ERROR - NO COLLUSION"; // No error message has been recorded
	}

	UE_LOG(LogTemp, Error, TEXT("Error: %d"), ErrorMessageID);

	LPSTR MessageBuffer = nullptr;

	std::string Message(MessageBuffer,
		(FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, ErrorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&MessageBuffer),
		0, nullptr)));

	// Free the buffer.
	LocalFree(MessageBuffer);

	return Message;
}
#endif

FSharedMemory::FSharedMemory(const std::string& Name, unsigned int const BufferSize) :
	MemSize(BufferSize)
{
	MemPath = GSharedmem_Base_Path;// +'_' + Name;
	MemPath += '_';
	MemPath += Name;

#if PLATFORM_WINDOWS

	const std::wstring STemp = std::wstring(MemPath.begin(), MemPath.end());
	const LPCWSTR WindowsMemPath = STemp.c_str();

	UE_LOG(LogTemp, Log, TEXT("SharedMemory:: Creating file mapping of size %d with path %hs..."), BufferSize,
	       ANSI_TO_TCHAR(MemPath.c_str()));

	MemFile = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, this->MemSize, WindowsMemPath);
	//MemFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, this->MemSize, SHAREDMEM_BASE_PATH);

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		UE_LOG(LogTemp, Warning,
		       TEXT(
			       "SharedMemory:: Unable to create MemFile! The path %hs already exists. Hopefully it is the correct size..."
		       ), ANSI_TO_TCHAR(MemPath.c_str()));
	}

	if (!MemFile) {
		const std::string Msg = GetLastErrorAsString();
		UE_LOG(LogTemp, Fatal, TEXT("SharedMemory:: Unable to create MemFile! %hs"), ANSI_TO_TCHAR(Msg.c_str()));
	}

	MemPointer = MapViewOfFile(MemFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (!MemPointer) {
		const std::string Msg = GetLastErrorAsString();
		UE_LOG(LogTemp, Fatal, TEXT("SharedMemory:: Unable to create MemPointer! %hs"), ANSI_TO_TCHAR(Msg.c_str()));
	}

#elif PLATFORM_LINUX

	MemFile = shm_open(MemPath.c_str(), O_CREAT | O_RDWR, 0777);
	if (MemFile == -1) {
		LogSystemError("Unable to create shared memory buffer");
	}

	int status = ftruncate(MemFile, this->MemSize);
	if (status == -1) {
		LogSystemError("Failed to truncate file");
	}

	MemPointer = static_cast<void*>(mmap(nullptr, this->MemSize, PROT_READ | PROT_WRITE,
		MAP_SHARED, MemFile, 0));
	if (MemPointer == MAP_FAILED) {
		LogSystemError("Failed to map shared memory");
	}

	// Doesn't need to stay open
	close(MemFile);
#endif
}

FSharedMemory::~FSharedMemory() {
#if PLATFORM_WINDOWS
	CloseHandle(MemFile);
	UnmapViewOfFile(MemPointer);
#elif PLATFORM_LINUX
	// the client still hangs on to this memory location. We need to figure out a
	// better way to release it.
	// munmap(MemPointer, MemSize);
#endif
}

void FSharedMemory::LogSystemError(const std::string& ErrorMessage) const {
	// ReSharper disable once CppDeprecatedEntity
	std::string ErrorMsg = ErrorMessage + " - Error code: " + std::to_string(errno) + " - " + std::string(strerror(errno));
	UE_LOG(LogTemp, Fatal, TEXT("%hs"), ANSI_TO_TCHAR(ErrorMessage.c_str()));
}
