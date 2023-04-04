// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetManager.generated.h"

class AMTDataCollectorCharacter;
class ATarget;

UCLASS()
class MTDATACOLLECTOR_API ATargetManager : public AActor
{
	GENERATED_BODY()

public:
	ATargetManager();
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
		void SpawnNewTarget();

	UFUNCTION()
		void DoNextCycle();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		ATarget* currentTarget;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AMTDataCollectorCharacter* Character;

	UPROPERTY(EditAnywhere)
	double Xmax = 1200.f;

	UPROPERTY(EditAnywhere)
	double Zmax = 700.f;

	UPROPERTY(EditAnywhere)
	float TargetScale = 0.25f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ATarget> spawnType;
};
