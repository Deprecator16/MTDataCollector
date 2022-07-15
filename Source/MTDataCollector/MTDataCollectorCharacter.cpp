// Copyright Epic Games, Inc. All Rights Reserved.

#include "MTDataCollectorCharacter.h"
#include "MTDataCollectorProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"

#include "Target.h"


//////////////////////////////////////////////////////////////////////////
// AMTDataCollectorCharacter

AMTDataCollectorCharacter::AMTDataCollectorCharacter()
{
	hasFired = false;
	MouseSensitivity = 1.0f;
	WritePath = FPaths::ProjectConfigDir();
	WritePath += TEXT("DATA") + FDateTime::Now().ToString() + TEXT(".csv");

	FFileHelper::SaveStringToFile(TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,Fired,HitTarget\n"),
		*WritePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	//const UGameDeveloperSettings* GameSettings = GetDefault<UGameDeveloperSettings>();
	//UE_LOG(LogTemp, Warning, TEXT("%f"), GameSettings->MouseSensitivity);
	//MouseSensitivity = GameSettings->MouseSensitivity;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));
}

void AMTDataCollectorCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	StartTime = FDateTime::Now();
}

void AMTDataCollectorCharacter::PollTrajectory()
{
	FString outString;
	auto currentTime = FDateTime::Now() - StartTime;
	double currentTimeMS = currentTime.GetTotalMilliseconds();

	outString += FString::SanitizeFloat(currentTimeMS) + ",";

	FVector PlayerLoc = GetFirstPersonCameraComponent()->GetComponentLocation();
	FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
	FRotator playerRotation = ForwardVector.Rotation();

	outString += FString::SanitizeFloat(playerRotation.Pitch) + "," + FString::SanitizeFloat(playerRotation.Yaw) + ",";

	AActor* currentTarget = UGameplayStatics::GetActorOfClass(GetWorld(), ATarget::StaticClass());
	if (currentTarget)
	{
		ATarget* refTarget = Cast<ATarget>(currentTarget);
		if (refTarget)
		{
			FVector RefTargetPos = refTarget->GetActorLocation();
			FVector VectorToRefTarget = RefTargetPos - PlayerLoc;
			FRotator RefTargetAngle = VectorToRefTarget.Rotation();

			outString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," + FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
		}
		else
		{
			outString += "NA,NA,";
		}
	}
	else
	{
		outString += "NA,NA,";
	}

	outString += "FALSE,NA,\n";

	FFileHelper::SaveStringToFile(outString, *WritePath, FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}

//////////////////////////////////////////////////////////////////////////// Input

void AMTDataCollectorCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AMTDataCollectorCharacter::OnPrimaryAction);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AMTDataCollectorCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AMTDataCollectorCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "Mouse" versions handle devices that provide an absolute delta, such as a mouse.
	// "Gamepad" versions are for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AMTDataCollectorCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &AMTDataCollectorCharacter::TurnWithMouse);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AMTDataCollectorCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &AMTDataCollectorCharacter::LookUpWithMouse);
}

void AMTDataCollectorCharacter::OnPrimaryAction()
{
	//const UGameDeveloperSettings* GameSettings = GetDefault<UGameDeveloperSettings>();
	//UE_LOG(LogTemp, Warning, TEXT("%f"), GameSettings->MouseSensitivity);

	if (!hasFired)
	{
		GetWorldTimerManager().SetTimer(MousePollingHandler, this, &AMTDataCollectorCharacter::PollTrajectory, 1.f / 60.f, true, 0.0f);
		hasFired = true;
	}

	FString outString;
	auto currentTime = FDateTime::Now() - StartTime;
	double currentTimeMS = currentTime.GetTotalMilliseconds();

	// Write to file
	outString += FString::SanitizeFloat(currentTimeMS) + ",";

	FHitResult* HitResult = new FHitResult();
	FVector StartTrace = GetFirstPersonCameraComponent()->GetComponentLocation();
	FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
	FVector EndTrace = ForwardVector * 10000.f + StartTrace;
	FCollisionQueryParams* TraceParams = new FCollisionQueryParams();

	FRotator playerRotation = ForwardVector.Rotation();

	outString += FString::SanitizeFloat(playerRotation.Pitch) + "," + FString::SanitizeFloat(playerRotation.Yaw) + ",";

	AActor* currentTarget = UGameplayStatics::GetActorOfClass(GetWorld(), ATarget::StaticClass());
	if (currentTarget)
	{
		ATarget* refTarget = Cast<ATarget>(currentTarget);
		if (refTarget)
		{
			FVector RefTargetPos = refTarget->GetActorLocation();
			FVector VectorToRefTarget = RefTargetPos - StartTrace;
			FRotator RefTargetAngle = VectorToRefTarget.Rotation();

			outString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," + FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
		}
		else
		{
			outString += "NA,NA,";
		}
	}
	else
	{
		outString += "NA,NA,";
	}

	outString += "TRUE,";

	if (GetWorld()->LineTraceSingleByChannel(*HitResult, StartTrace, EndTrace, ECC_Visibility, *TraceParams))
	{
		//DrawDebugLine(GetWorld(), StartTrace, ForwardVector * dist + StartTrace, FColor::Green, false, 5.f);
		ATarget* target = Cast<ATarget>(HitResult->GetActor());

		if (target && IsValid(target))
		{
			outString += "TRUE\n";
			//target->DamageTarget(100.f);
		}
		else
		{
			outString += "FALSE\n";
		}
	}
	else
	{
		outString += "FALSE\n";
	}

	FFileHelper::SaveStringToFile(outString, *WritePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);

	// Trigger the OnItemUsed Event
	OnUseItem.Broadcast();
}

void AMTDataCollectorCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnPrimaryAction();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AMTDataCollectorCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AMTDataCollectorCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMTDataCollectorCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMTDataCollectorCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMTDataCollectorCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMTDataCollectorCharacter::TurnWithMouse(float Value)
{
	UE_LOG(LogTemp, Warning, TEXT("%f"), Value);
	AddControllerYawInput(Value * MouseSensitivity);
}

void AMTDataCollectorCharacter::LookUpWithMouse(float Value)
{
	UE_LOG(LogTemp, Warning, TEXT("%f"), Value);
	AddControllerPitchInput(Value * MouseSensitivity);
}

bool AMTDataCollectorCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AMTDataCollectorCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AMTDataCollectorCharacter::EndTouch);

		return true;
	}
	
	return false;
}
