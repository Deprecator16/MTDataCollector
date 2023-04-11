#include "TargetManager.h"

#include "Target.h"
#include "../MTDataCollectorCharacter.h"
#include "Kismet/GameplayStatics.h"

ATargetManager::ATargetManager() :
	TargetMode(ETargetManagerMode::MovingXZ), MaxX(1800.0), MaxY(0.0), MaxZ(700.0), TargetScale(0.25),
	DefaultMovementSpeed(50.0), CurrentMovementSpeed(0.0)
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
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
		break;
	case ETargetManagerMode::MovingX:
		break;
	case ETargetManagerMode::MovingZ:
		break;
	case ETargetManagerMode::MovingXZ:
		SpawnMovingTarget(GenRandomPointInSpawnBox(), GenRandomPointInSpawnBox(), FMath::FRandRange(0.0, 1.0));
		break;
	case ETargetManagerMode::ReactionTime:
	default:
		SpawnStaticTarget({MaxX / 2, MaxY / 2, MaxZ / 2}); // Spawn at midpoint
		break;
	}
}

void ATargetManager::DestroyAndSpawnNextTarget()
{
	if (IsValid(CurrentTarget))
	{
		CurrentTarget->Destroy();
	}

	SpawnNewTarget();
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
		Character->OnUseItem.AddDynamic(this, &ATargetManager::DestroyAndSpawnNextTarget);
	}
}

void ATargetManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetMode == ETargetManagerMode::MovingX ||
		TargetMode == ETargetManagerMode::MovingZ ||
		TargetMode == ETargetManagerMode::MovingXZ &&
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
