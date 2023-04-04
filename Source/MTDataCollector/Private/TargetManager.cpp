// Fill out your copyright notice in the Description page of Project Settings.
#include "TargetManager.h"

#include "Target.h"
#include "../MTDataCollectorCharacter.h"
#include "Kismet/GameplayStatics.h"

ATargetManager::ATargetManager()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
}

void ATargetManager::SpawnNewTarget()
{
	UWorld* world = GetWorld();
	if (!world || !spawnType)
		return;

	FVector Offset;
	Offset.X = FMath::FRandRange(0, Xmax);
	Offset.Z = FMath::FRandRange(0, Zmax);

	FTransform transform;
	transform.SetLocation(GetActorLocation() + Offset);
	transform.SetScale3D(FVector(TargetScale));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	currentTarget = world->SpawnActor<ATarget>(spawnType, transform, SpawnParams);
}

void ATargetManager::DoNextCycle()
{
	if (IsValid(currentTarget))
		currentTarget->Destroy();

	SpawnNewTarget();
}

void ATargetManager::BeginPlay()
{
	Super::BeginPlay();
	AActor* actorToFind = UGameplayStatics::GetActorOfClass(GetWorld(), AMTDataCollectorCharacter::StaticClass());
	AMTDataCollectorCharacter* refer = actorToFind ? Cast<AMTDataCollectorCharacter>(actorToFind) : nullptr;

	if (refer)
	{
		Character = refer;
		Character->OnUseItem.AddDynamic(this, &ATargetManager::DoNextCycle);
	}
}

void ATargetManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}