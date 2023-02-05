// Fill out your copyright notice in the Description page of Project Settings.


#include "MTNetwork.h"

UMTNetwork::UMTNetwork()
{
	Network = nullptr;
}

TArray<float> UMTNetwork::URunModel(float x, float y)
{
	static int clicked = 0;
	if (Network == nullptr) {
		//Running inference
		Network = NewObject<UNeuralNetwork>((UObject*)GetTransientPackage(), UNeuralNetwork::StaticClass());
		//create array of the correct pixel values from results


		// Load model from file.
		const FString& ONNXModelFilePath = TEXT("D:/UE5 Projects/MTDataCollector/Content/Models/model.onnx");
		// Set Device
		Network->SetDeviceType(ENeuralDeviceType::CPU);
		// Check that the network was successfully loaded
		if (Network->Load(ONNXModelFilePath))
		{
			UE_LOG(LogTemp, Log, TEXT("Neural Network loaded successfully."))
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UNeuralNetwork could not loaded from %s."), *ONNXModelFilePath);
			TArray<float> failed;
			return failed;
		}
	}

	float dx = x / 64.f;
	float dy = y / 64.f;
	TArray<float> inArr;

	for (int i = 1; i <= 64; i++)
	{
		inArr.Add(dx * i);
		inArr.Add(dy * i);
	}

	UE_LOG(LogTemp, Warning, TEXT("Network Output Tensor Num: %d."), Network->GetOutputTensorNumber());
	Network->SetInputFromArrayCopy(inArr);
	Network->Run();

	auto oTensor = Network->GetOutputTensor();
	auto out = oTensor.GetArrayCopy<float>();

	clicked++;

	return out;
}
