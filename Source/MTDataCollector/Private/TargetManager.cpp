// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetManager.h"

#include "Target.h"
#include "../MTDataCollectorCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ATargetManager::ATargetManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
}

void ATargetManager::SpawnNewTarget()
{
	if (spawnType)
	{
		UWorld* world = GetWorld();
		if (world)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;

			FVector Offset;
			Offset.X = FMath::FRandRange(0, Xmax);
			Offset.Z = FMath::FRandRange(0, Zmax);

			FVector SpawnLocation = GetActorLocation() + Offset;
			FRotator SpawnRotation = GetActorRotation();

			FTransform transform;
			transform.SetLocation(SpawnLocation);
			transform.SetScale3D(FVector(0.25f));

			currentTarget = world->SpawnActor<ATarget>(spawnType, transform, SpawnParams);
		}
	}
}

void ATargetManager::DoNextCycle()
{
	FVector playerDirection = Character->GetFirstPersonCameraComponent()->GetForwardVector();
	FRotator playerRotation = playerDirection.Rotation(); // Write to file

	FVector playerPos = Character->GetFirstPersonCameraComponent()->GetComponentLocation();

	if (currentTarget)
	{
		FVector targetPos = currentTarget->GetActorLocation();
		FVector vectorToTarget = targetPos - playerPos;

		FRotator targetAngle = vectorToTarget.Rotation(); // Write to file
		currentTarget->Destroy();
	}

	SpawnNewTarget();

	FVector newTargetPos = currentTarget->GetActorLocation();
	FVector newVectorToTarget = newTargetPos - playerPos;
	FRotator newTargetAngle = newVectorToTarget.Rotation(); // Write to file
}

// Called when the game starts or when spawned
void ATargetManager::BeginPlay()
{
	Super::BeginPlay();
	AActor* ActorToFind = UGameplayStatics::GetActorOfClass(GetWorld(), AMTDataCollectorCharacter::StaticClass());
	AMTDataCollectorCharacter* refer = Cast<AMTDataCollectorCharacter>(ActorToFind);

	if (refer)
	{
		Character = refer;
		Character->OnUseItem.AddDynamic(this, &ATargetManager::DoNextCycle);
	}

	/*FVector Corner = GetActorLocation();
	FVector Extent(Xmax / 2, 5.0, Zmax / 2);

	DrawDebugLine(GetWorld(), Corner, Corner + FVector(Xmax, 0, Zmax), FColor::Green, false, 60.0f);*/

	//DrawDebugBox(GetWorld(), Center, Extent, FColor::Green, false, 60.0f);
}

// Called every frame
void ATargetManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}