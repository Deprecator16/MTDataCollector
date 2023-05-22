#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "TargetManager.h"

#include "MTDataCollectorCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;
class ATargetManager;
class ATarget;

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

	UFUNCTION()
	bool WriteStringToFile(const FString& Text, const FString& File) const;

	UFUNCTION()
	void WritePrimaryClickStaticToDataFile() const;

	UFUNCTION()
	void WritePrimaryClickReactionTimeToDataFile() const;

	UFUNCTION()
	void WritePollAngleStaticToDataFile() const;

	UFUNCTION()
	void WritePollAngleTrackingToDataFile() const;

	UFUNCTION()
	void WriteTrackingTargetSpawnedToDataFile();

	UFUNCTION()
	void WriteReactionTimeTargetSpawnedToDataFile();

	UFUNCTION(BlueprintCallable)
	void DoConfigSave();

	UFUNCTION(BlueprintCallable)
	ATarget* GetCurrentTarget() const;

	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite)
	float MouseSensitivity;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnUseItem OnUseItem;

	UPROPERTY(BlueprintReadOnly)
	ETargetManagerMode TargetManagerMode;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void OnPrimaryAction();
	void TurnWithMouse(float Value);
	void LookUpWithMouse(float Value);

private:
	FRotator GetPlayerRotation() const;
	void PollPlayerAngle() const;

	bool bHasFired;
	FString WritePath;
	FString FileName;
	double AnglePollRateHertz;
	FTimerHandle MousePollingHandler;
	FDateTime StartTime;
};
