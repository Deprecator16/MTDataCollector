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
	UWorld* World = GetWorld();
	if (!World || !SpawnType)
		return;

	FVector Offset;
	Offset.X = FMath::FRandRange(0, MaxX);
	Offset.Z = FMath::FRandRange(0, MaxZ);

	FTransform Transform;
	Transform.SetLocation(GetActorLocation() + Offset);
	Transform.SetScale3D(FVector(TargetScale));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	CurrentTarget = World->SpawnActor<ATarget>(SpawnType, Transform, SpawnParams);
}

void ATargetManager::DoNextCycle()
{
	if (IsValid(CurrentTarget))
		CurrentTarget->Destroy();

	SpawnNewTarget();
}

void ATargetManager::BeginPlay()
{
	Super::BeginPlay();
	AActor* ActorToFind = UGameplayStatics::GetActorOfClass(GetWorld(), AMTDataCollectorCharacter::StaticClass());

	if (AMTDataCollectorCharacter* Refer = ActorToFind ? Cast<AMTDataCollectorCharacter>(ActorToFind) : nullptr)
	{
		Character = Refer;
		Character->OnUseItem.AddDynamic(this, &ATargetManager::DoNextCycle);
	}
}

void ATargetManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}