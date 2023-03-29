// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork.h"
#include "Misc/FileHelper.h"
#include "MTNetwork.generated.h"

/**
 * 
 */
UCLASS()
class MTDATACOLLECTOR_API UMTNetwork : public UNeuralNetwork
{
	GENERATED_BODY()
public:
	UMTNetwork();
	TArray<float> URunModel(TArray<float>& inArr, FString checkPath);
	bool InitModel(FString Path);

	bool hasBeenInit = false;
};
