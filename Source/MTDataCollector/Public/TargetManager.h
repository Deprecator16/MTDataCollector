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
	UPROPERTY(EditAnywhere)
		double Xmax = 1200.f;

	UPROPERTY(EditAnywhere)
		double Zmax = 700.f;

	UPROPERTY(EditAnywhere)
		TSubclassOf<class ATarget> spawnType;

	// Sets default values for this actor's properties
	ATargetManager();

	UFUNCTION(BlueprintCallable)
		void SpawnNewTarget();

	UFUNCTION()
		void DoNextCycle();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		ATarget* currentTarget;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// The character that will be shooting the targets
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		AMTDataCollectorCharacter* Character;
};
