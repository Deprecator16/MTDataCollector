#include "TargetManager.h"

#include "Target.h"
#include "../MTDataCollectorCharacter.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

ATargetManager::ATargetManager() :
	TargetMode(ETargetManagerMode::Moving), MaxX(1800.0), MaxY(0.0), MaxZ(700.0), MaxReaction(7.0), MinReaction(1.0),
	TargetScale(0.25), DefaultMovementSpeed(50.0), CurrentMovementSpeed(0.0), bNextIsRandLargeAngle(true)
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));

	//if (static ConstructorHelpers::FObjectFinder<USoundCue> HitSoundAsset(TEXT("/Game/Effects/pong-paddle.pong-paddle"))
	//	; HitSoundAsset.Succeeded())
	//{
	//	TargetSpawnSound = HitSoundAsset.Object;
	//}
}

bool ATargetManager::IsThisManagerValid() const
{
	return GetWorld() && SpawnType;
}

void ATargetManager::SpawnStaticTarget(const FVector& Offset)
{
	if (!IsThisManagerValid())
	{
		return;
	}

	FTransform Transform;
	Transform.SetLocation(GetActorLocation() + Offset);
	Transform.SetScale3D(FVector(TargetScale));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	CurrentTarget = GetWorld()->SpawnActor<ATarget>(SpawnType, Transform, SpawnParams);
}

void ATargetManager::SpawnMovingTarget(const FVector& StartOffset, const FVector& EndOffset, double LerpPercentage,
                                       const double MovementSpeed)
{
	CurrentMovementSpeed = MovementSpeed;
	LerpStart = StartOffset + GetActorLocation();
	LerpEnd = EndOffset + GetActorLocation();
	LerpPercentage = FMath::Clamp(LerpPercentage, 0.0, 1.0);
	const FVector StartPoint = FMath::Lerp(StartOffset, EndOffset, LerpPercentage);
	SpawnStaticTarget(StartPoint);
}

void ATargetManager::SpawnMovingTarget(const FVector& StartOffset, const FVector& EndOffset, double LerpPercentage)
{
	SpawnMovingTarget(StartOffset, EndOffset, LerpPercentage, DefaultMovementSpeed);
}

void ATargetManager::SpawnReactionTimeTarget()
{
	SpawnStaticTarget({MaxX / 2, MaxY / 2, MaxZ / 2}); // Spawn at midpoint
	OnTargetSpawn.Broadcast();
}

void ATargetManager::SpawnRandTargetOnSphere()
{
	FVector SpherePoint = FMath::VRand();
	SpherePoint *= {MaxX, MaxY, MaxZ};
	SpawnStaticTarget(SpherePoint);
	PlaySpawnSound(SpherePoint);
}

void ATargetManager::SpawnNewTarget()
{
	if (!IsThisManagerValid())
	{
		return;
	}

	switch (TargetMode)
	{
	case ETargetManagerMode::Static:
		SpawnStaticTarget(GenRandomPointInSpawnBox());
		break;
	case ETargetManagerMode::LargeAngle:
		if (bNextIsRandLargeAngle)
		{
			SpawnRandTargetOnSphere();
		}
		else
		{
			SpawnStaticTarget(FVector(1, 0, 0) * MaxX);
		}
		bNextIsRandLargeAngle = !bNextIsRandLargeAngle;
		break;
	case ETargetManagerMode::Tracking:
		SpawnMovingTarget(GenRandomPointInSpawnBox(), GenRandomPointInSpawnBox(), FMath::FRandRange(0.0, 1.0));
		GetWorldTimerManager().ClearTimer(TrackingHandler);
		GetWorldTimerManager().SetTimer(TrackingHandler, this, &ATargetManager::DestroyAndSpawnNextTarget,
		                                3.f, false);
		break;
	case ETargetManagerMode::ReactionTime:
		GetWorldTimerManager().ClearTimer(ReactionTimeHandler);
		GetWorldTimerManager().SetTimer(ReactionTimeHandler, this, &ATargetManager::SpawnReactionTimeTarget,
		                                FMath::FRandRange(MinReaction, MaxReaction), false);
		break;
	case ETargetManagerMode::Moving:
	default:
		SpawnMovingTarget(GenRandomPointInSpawnBox(), GenRandomPointInSpawnBox(), FMath::FRandRange(0.0, 1.0));
		break;
	}
}

void ATargetManager::DestroyAndSpawnNextTarget()
{
	if (TargetMode == ETargetManagerMode::Tracking)
	{
		OnTargetSpawn.Broadcast();
	}

	if (IsValid(CurrentTarget))
	{
		CurrentTarget->Destroy();
	}

	SpawnNewTarget();
}

void ATargetManager::StartTrackingMode()
{
	Character->OnUseItem.RemoveDynamic(this, &ATargetManager::StartTrackingMode);
	SpawnNewTarget();
}

void ATargetManager::StartReactionTimeMode()
{
	Character->OnUseItem.RemoveDynamic(this, &ATargetManager::StartTrackingMode);
	SpawnNewTarget();
}

void ATargetManager::PlaySpawnSound(const FVector& Offset) const
{
	if (TargetSpawnSound != nullptr)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, TargetSpawnSound, GetActorLocation() + Offset);
	}
}

FVector ATargetManager::GenRandomPointInSpawnBox() const
{
	return {FMath::FRandRange(0, MaxX), FMath::FRandRange(0, MaxY), FMath::FRandRange(0, MaxZ)};
}

void ATargetManager::BeginPlay()
{
	Super::BeginPlay();
	AActor* ActorToFind = UGameplayStatics::GetActorOfClass(GetWorld(), AMTDataCollectorCharacter::StaticClass());

	if (AMTDataCollectorCharacter* Refer = ActorToFind ? Cast<AMTDataCollectorCharacter>(ActorToFind) : nullptr)
	{
		Character = Refer;
		Character->TargetManagerMode = TargetMode;

		switch (TargetMode)
		{
		case ETargetManagerMode::ReactionTime:
			Character->OnUseItem.AddDynamic(this, &ATargetManager::ATargetManager::DestroyAndSpawnNextTarget);
			OnTargetSpawn.AddDynamic(Character, &AMTDataCollectorCharacter::WriteReactionTimeTargetSpawnedToDataFile);
			break;
		case ETargetManagerMode::Tracking:
			Character->OnUseItem.AddDynamic(this, &ATargetManager::StartTrackingMode);
			OnTargetSpawn.AddDynamic(Character, &AMTDataCollectorCharacter::WriteTrackingTargetSpawnedToDataFile);
			break;
		case ETargetManagerMode::LargeAngle:
		case ETargetManagerMode::Moving:
		case ETargetManagerMode::Static:
		default:
			Character->OnUseItem.AddDynamic(this, &ATargetManager::DestroyAndSpawnNextTarget);
			break;
		}
	}
}

void ATargetManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ((TargetMode == ETargetManagerMode::Moving || TargetMode == ETargetManagerMode::Tracking) &&
		IsValid(CurrentTarget))
	{
		const FVector NewPos = FMath::VInterpConstantTo(CurrentTarget->GetActorLocation(), LerpEnd, DeltaTime,
		                                                CurrentMovementSpeed);
		CurrentTarget->SetActorLocation(NewPos);
		if (NewPos == LerpEnd)
		{
			Swap(LerpStart, LerpEnd);
		}
	}
}
