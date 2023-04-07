#include "MTNetwork.h"

UMTNetwork::UMTNetwork() :
	bNetworkHasBeenInit(false)
{
}

TArray<float> UMTNetwork::URunModel(const TArray<float>& InArr)
{
	if (!bNetworkHasBeenInit)
	{
		TArray<float> Failed;
		Failed.AddZeroed(128);
		return Failed;
	}

	SetInputFromArrayCopy(InArr);
	Run();
	return GetOutputTensor().GetArrayCopy<float>();
}

bool UMTNetwork::InitModel(const FString& Path)
{
	bNetworkHasBeenInit = Load(Path);
	return bNetworkHasBeenInit;
}
