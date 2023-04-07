#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "MTDataCollectorCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;
class ATarget;

#define USE_SHARED_NNI_MEMORY 0

// Declaration of delegate to be called when Primary Action triggered.
// Declared dynamic so it can be accessed in Blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUseItem);

UCLASS(config=Game)
class AMTDataCollectorCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* FirstPersonCameraComponent;

public:
	AMTDataCollectorCharacter();

	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MouseSensitivity;
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
		FOnUseItem OnUseItem;

	const int Points_Per_Trajectory = 64;

protected:
	virtual void BeginPlay() override;

	void OnPrimaryAction();
	void TurnWithMouse(float Value);
	void LookUpWithMouse(float Value);

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void PollTrajectory() const;
	ATarget* GetCurrentTarget() const;
	
	bool bHasFired;
	FString WritePath;
	FTimerHandle MousePollingHandler;
	FDateTime StartTime;
};

