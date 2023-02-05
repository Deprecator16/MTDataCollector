// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork.h"
#include "MTNetwork.generated.h"

/**
 * 
 */
UCLASS()
class MTDATACOLLECTOR_API UMTNetwork : public UNeuralNetwork
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	UNeuralNetwork* Network = nullptr;
	UMTNetwork();
	TArray<float> URunModel(float x, float y);
};
