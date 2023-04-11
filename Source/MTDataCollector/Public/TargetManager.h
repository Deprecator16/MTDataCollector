#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetManager.generated.h"

class AMTDataCollectorCharacter;
class ATarget;

UENUM()
enum class ETargetManagerMode
{
	Static,
	MovingX,
	MovingZ,
	MovingXZ,
	LargeAngle,
	ReactionTime,
};

UCLASS()
class MTDATACOLLECTOR_API ATargetManager : public AActor
{
	GENERATED_BODY()

public:
	ATargetManager();

	virtual void Tick(float DeltaTime) override;

	bool IsThisManagerValid() const;
	void SpawnStaticTarget(const FVector& Offset);
	void SpawnMovingTarget(const FVector& StartOffset, const FVector& EndOffset, double LerpPercentage,
	                       const double MovementSpeed);
	void SpawnMovingTarget(const FVector& StartOffset, const FVector& EndOffset, double LerpPercentage);
	void SpawnNewTarget();
	void DestroyAndSpawnNextTarget();
	FVector GenRandomPointInSpawnBox() const;

	UPROPERTY(EditAnywhere)
	ETargetManagerMode TargetMode;

	UPROPERTY()
	AMTDataCollectorCharacter* Character;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ATarget> SpawnType;

	UPROPERTY()
	ATarget* CurrentTarget;

	UPROPERTY(EditAnywhere)
	double MaxX;

	UPROPERTY(EditAnywhere)
	double MaxY;

	UPROPERTY(EditAnywhere)
	double MaxZ;

	UPROPERTY(EditAnywhere)
	double TargetScale;

	UPROPERTY(EditAnywhere)
	double DefaultMovementSpeed;

	UPROPERTY(EditAnywhere)
	double CurrentMovementSpeed;

protected:
	virtual void BeginPlay() override;

private:
	FVector LerpStart;
	FVector LerpEnd;
};
