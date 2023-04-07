// ReSharper disable CommentTypo
// Modified from https://github.com/BYU-PCCL/holodeck-engine/blob/develop/Source/Holodeck/HolodeckCore/Public/HolodeckSharedMemory.h
//
// Created by joshgreaves on 5/9/17.
//

#pragma once

#include <string>

#if PLATFORM_WINDOWS
#include <windows.h>
#elif PLATFORM_LINUX
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#endif

class MTDATACOLLECTOR_API FSharedMemory
{
public:
	/**
	  * Constructor
	  * Constructs a memory mapped file with a given name, prepended
	  * by "HOLODECK_MEM_".
	  * @param Name the name of the memory mapped file. Is prepended.
	  * @param BufferSize the number of bytes to allocated for the file.
	  */
	explicit FSharedMemory(const std::string& Name, unsigned int const BufferSize);

	~FSharedMemory();

	/**
	  * GetPtr
	  * Gets a pointer to the start of the memory mapped file.
	  * @return a void pointer to the start of the memory buffer.
	  */
	void* GetPtr() const { return MemPointer; }

	/**
	  * Size
	  * Gets the size of the allocated memory mapped file.
	  * @return the size in bytes of the file.
	  */
	int Size() const { return MemSize; }

private:
	std::string MemPath;
	unsigned int MemSize;
	void* MemPointer;

#if PLATFORM_WINDOWS
	HANDLE MemFile;
#elif PLATFORM_LINUX
	int MemFile;
#endif

	void LogSystemError(const std::string& ErrorMessage) const;
};
