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
	TArray<float> URunModel(const TArray<float>& InArr);
	bool InitModel(const FString& Path);

private:
	bool bNetworkHasBeenInit;
};
