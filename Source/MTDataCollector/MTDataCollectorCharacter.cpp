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
#include "MTNetwork.h"


//////////////////////////////////////////////////////////////////////////
// AMTDataCollectorCharacter

AMTDataCollectorCharacter::AMTDataCollectorCharacter()
{
	hasFired = false;
	NeuralNetworkIsReady = false;
	usingNeuralNetwork = true;
	NeuralNetIndex = 0;
	MouseSensitivity = 1.0f;
	WritePath = FPaths::ProjectConfigDir() + TEXT("DATA") + FDateTime::Now().ToString() + TEXT(".csv");
	ModelPath = TEXT("D:/UE5 Projects/MTDataCollector/Content/Models/model.onnx");

	FFileHelper::SaveStringToFile(TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,Fired,HitTarget\n"),
		*WritePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);

	//const UGameDeveloperSettings* GameSettings = GetDefault<UGameDeveloperSettings>();
	//UE_LOG(LogTemp, Warning, TEXT("%f"), GameSettings->MouseSensitivity);
	//MouseSensitivity = GameSettings->MouseSensitivity;

	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	Network = NewObject<UMTNetwork>((UObject*)GetTransientPackage(), UMTNetwork::StaticClass());
}

void AMTDataCollectorCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartTime = FDateTime::Now();
}

void AMTDataCollectorCharacter::PollTrajectory()
{
	FString outString;
	auto currentTime = FDateTime::Now() - StartTime;
	FRotator playerRotation = GetFirstPersonCameraComponent()->GetForwardVector().Rotation();

	outString += FString::SanitizeFloat(currentTime.GetTotalMilliseconds()) + ",";
	outString += FString::SanitizeFloat(playerRotation.Pitch) + "," + FString::SanitizeFloat(playerRotation.Yaw) + ",";

	ATarget* refTarget = GetCurrentTarget();
	if (IsValid(refTarget))
	{
		FVector RefTargetPos = refTarget->GetActorLocation();
		FVector VectorToRefTarget = RefTargetPos - GetFirstPersonCameraComponent()->GetComponentLocation();
		FRotator RefTargetAngle = VectorToRefTarget.Rotation();
		outString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," + FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
	}
	else
	{
		outString += "NA,NA,";
	}

	outString += "FALSE,NA\n";
	FFileHelper::SaveStringToFile(outString, *WritePath, FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}

void AMTDataCollectorCharacter::DoNeuralNetMovement()
{
	FRotator delta(trajectory[NeuralNetIndex * 2], trajectory[(NeuralNetIndex * 2) + 1], 0.f);
	FRotator newRotation = GetController()->GetControlRotation() + delta;
	GetController()->SetControlRotation(newRotation);

	if (NeuralNetIndex++ == 63)
		GetWorldTimerManager().ClearTimer(NeuralNetHandler);
}

void AMTDataCollectorCharacter::CheckIfTargetUp()
{
	ATarget* refTarget = GetCurrentTarget();
	if (IsValid(refTarget))
	{
		FVector VectorToRefTarget = refTarget->GetActorLocation() - GetFirstPersonCameraComponent()->GetComponentLocation();
		FRotator Normalized = VectorToRefTarget.Rotation() - GetController()->GetControlRotation();

		float dx = Normalized.Pitch / 64.f;
		float dy = Normalized.Yaw / 64.f;
		inArr.Reset(NUM_NNI_STEPS * 2);
		for (int i = 0; i < NUM_NNI_STEPS; i++)
		{
			inArr.Add(dx * i);
			inArr.Add(dy * i);
		}

		trajectory = Network->URunModel(inArr, ModelPath);
		while (trajectory.Num() == 0)
		{
			Network = NewObject<UMTNetwork>((UObject*)GetTransientPackage(), UMTNetwork::StaticClass());
			trajectory = Network->URunModel(inArr, ModelPath);
		}
		GetWorldTimerManager().SetTimer(NeuralNetHandler, this, &AMTDataCollectorCharacter::DoNeuralNetMovement, 1.f / 60.f, true, 0.0f);
	}
	else
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AMTDataCollectorCharacter::CheckIfTargetUp);
	}
}

ATarget* AMTDataCollectorCharacter::GetCurrentTarget()
{
	AActor* currentTarget = UGameplayStatics::GetActorOfClass(GetWorld(), ATarget::StaticClass());
	return currentTarget ? Cast<ATarget>(currentTarget) : nullptr;
}

void AMTDataCollectorCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AMTDataCollectorCharacter::OnPrimaryAction);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AMTDataCollectorCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AMTDataCollectorCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &AMTDataCollectorCharacter::TurnWithMouse);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &AMTDataCollectorCharacter::LookUpWithMouse);
}

void AMTDataCollectorCharacter::OnPrimaryAction()
{
	if (usingNeuralNetwork)
	{
		NeuralNetIndex = 0;
		GetWorldTimerManager().SetTimerForNextTick(this, &AMTDataCollectorCharacter::CheckIfTargetUp);
	}
	else
	{
		GetWorldTimerManager().SetTimer(MousePollingHandler, this, &AMTDataCollectorCharacter::PollTrajectory, 1.f / 60.f, true, 0.0f);

		FString outString;
		auto currentTime = FDateTime::Now() - StartTime;
		outString += FString::SanitizeFloat(currentTime.GetTotalMilliseconds()) + ",";

		FVector PlayerLocation = GetFirstPersonCameraComponent()->GetComponentLocation();
		FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
		FRotator playerRotation = ForwardVector.Rotation();
		outString += FString::SanitizeFloat(playerRotation.Pitch) + "," + FString::SanitizeFloat(playerRotation.Yaw) + ",";

		ATarget* refTarget = GetCurrentTarget();
		if (IsValid(refTarget))
		{
			FVector VectorToRefTarget = refTarget->GetActorLocation() - PlayerLocation;
			FRotator RefTargetAngle = VectorToRefTarget.Rotation();
			outString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," + FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
		}
		else
		{
			outString += "NA,NA,";
		}

		outString += "TRUE,";

		FHitResult* HitResult = new FHitResult();
		FVector EndTrace = ForwardVector * 10000.f + PlayerLocation;
		FCollisionQueryParams* TraceParams = new FCollisionQueryParams();
		if (GetWorld()->LineTraceSingleByChannel(*HitResult, PlayerLocation, EndTrace, ECC_Visibility, *TraceParams))
		{
			ATarget* target = Cast<ATarget>(HitResult->GetActor());
			outString += IsValid(target) ? "TRUE\n" : "FALSE\n";
		}
		else
		{
			outString += "FALSE\n";
		}

		FFileHelper::SaveStringToFile(outString, *WritePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
	}

	OnUseItem.Broadcast();
}

void AMTDataCollectorCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMTDataCollectorCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMTDataCollectorCharacter::TurnWithMouse(float Value)
{
	AddControllerYawInput(Value * MouseSensitivity);
}

void AMTDataCollectorCharacter::LookUpWithMouse(float Value)
{
	AddControllerPitchInput(Value * MouseSensitivity);
}
