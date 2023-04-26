#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetManager.generated.h"

class AMTDataCollectorCharacter;
class ATarget;

UENUM(BlueprintType)
enum class ETargetManagerMode : uint8
{
	Static UMETA(DisplayName = "Static"),
	Moving UMETA(DisplayName = "Moving"),
	Tracking UMETA(DisplayName = "Tracking"),
	LargeAngle UMETA(DisplayName = "LargeAngle"),
	ReactionTime UMETA(DisplayName = "ReactionTime"),
};

// Declaration of delegate to be called when Primary Action triggered.
// Declared dynamic so it can be accessed in Blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetSpawn);

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
	void SpawnReactionTimeTarget();
	void SpawnRandTargetOnSphere();
	void SpawnNewTarget();

	UFUNCTION()
	void DestroyAndSpawnNextTarget();

	UFUNCTION()
	void StartTrackingMode();

	UFUNCTION()
	void StartReactionTimeMode();

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
	double MaxReaction;

	UPROPERTY(EditAnywhere)
	double MinReaction;

	UPROPERTY(EditAnywhere)
	double TargetScale;

	UPROPERTY(EditAnywhere)
	double DefaultMovementSpeed;

	UPROPERTY(EditAnywhere)
	double CurrentMovementSpeed;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnTargetSpawn OnTargetSpawn;

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle TrackingHandler;
	FTimerHandle ReactionTimeHandler;
	FVector LerpStart;
	FVector LerpEnd;
};
