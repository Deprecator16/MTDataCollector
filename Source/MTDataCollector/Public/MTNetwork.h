#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork.h"

#include "MTNetwork.generated.h"

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
