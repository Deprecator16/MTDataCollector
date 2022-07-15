// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Target.generated.h"

class AMTDataCollectorCharacter;

UCLASS()
class MTDATACOLLECTOR_API ATarget : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATarget();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// The character that will be shooting the targets
	AMTDataCollectorCharacter* Character;

public:
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* TargetMesh;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void DestroyTarget();

	void DamageTarget(float Damage);
};
