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
class UMTNetwork;
class ATarget;

// Declaration of delegate to be called when Primary Action triggered.
// Declared dynamic so it can be accessed in Blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUseItem);

UCLASS(config=Game)
class AMTDataCollectorCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

public:
	AMTDataCollectorCharacter();

	void PollTrajectory();
	void DoNeuralNetMovement();
	void CheckIfTargetUp();
	ATarget* GetCurrentTarget();

	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:
	virtual void BeginPlay();

	void OnPrimaryAction();

	void TurnWithMouse(float Value);
	void LookUpWithMouse(float Value);

	void MoveForward(float Val);
	void MoveRight(float Val);

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MouseSensitivity;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnUseItem OnUseItem;

	const int NUM_NNI_STEPS = 64;
private:
	UMTNetwork* Network;
	FTimerHandle MousePollingHandler;
	FTimerHandle NeuralNetHandler;
	FDateTime StartTime;
	FString WritePath;
	FString ModelPath;
	bool hasFired;
	bool usingNeuralNetwork;
	bool NeuralNetworkIsReady;
	int NeuralNetIndex;
	TArray<float> trajectory;
	TArray<float> inArr;
};

