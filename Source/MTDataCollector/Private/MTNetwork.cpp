// Fill out your copyright notice in the Description page of Project Settings.
#include "MTNetwork.h"

UMTNetwork::UMTNetwork()
{
}

TArray<float> UMTNetwork::URunModel(TArray<float>& inArr, FString checkPath)
{
	if (!hasBeenInit) {
		InitModel(checkPath);
		hasBeenInit = true;
	}

	if (checkPath != ModelFullFilePath)
	{
		TArray<float> broken;
		return broken;
	}

	if (hasBeenInit)
	{
		SetInputFromArrayCopy(inArr);
		Run();
		return GetOutputTensor().GetArrayCopy<float>();
	}

	TArray<float> failed;
	failed.AddZeroed(128);
	return failed;
}

bool UMTNetwork::InitModel(FString Path)
{
	return Load(Path);
}
