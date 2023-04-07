#include "MTDataCollectorCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"

#include "Target.h"

AMTDataCollectorCharacter::AMTDataCollectorCharacter()
{
	MouseSensitivity = 1.0f;
	bHasFired = false;
	WritePath = FPaths::ProjectConfigDir() + TEXT("DATA") + FDateTime::Now().ToString() + TEXT(".csv");

	FFileHelper::SaveStringToFile(TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,Fired,HitTarget\n"),
		*WritePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);

	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
}

void AMTDataCollectorCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartTime = FDateTime::Now();
}

void AMTDataCollectorCharacter::PollTrajectory() const
{
	FString OutString;
	const auto CurrentTime = FDateTime::Now() - StartTime;
	const FRotator PlayerRotation = GetFirstPersonCameraComponent()->GetForwardVector().Rotation();

	OutString += FString::SanitizeFloat(CurrentTime.GetTotalMilliseconds()) + ",";
	OutString += FString::SanitizeFloat(PlayerRotation.Pitch) + "," + FString::SanitizeFloat(PlayerRotation.Yaw) + ",";

	if (const ATarget* RefTarget = GetCurrentTarget(); IsValid(RefTarget))
	{
		const FVector RefTargetPos = RefTarget->GetActorLocation();
		const FVector VectorToRefTarget = RefTargetPos - GetFirstPersonCameraComponent()->GetComponentLocation();
		const FRotator RefTargetAngle = VectorToRefTarget.Rotation();
		OutString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," + FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
	}
	else
	{
		OutString += "NA,NA,";
	}

	OutString += "FALSE,NA\n";
	FFileHelper::SaveStringToFile(OutString, *WritePath, FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(), FILEWRITE_Append);
}

ATarget* AMTDataCollectorCharacter::GetCurrentTarget() const
{
	AActor* CurrentTarget = UGameplayStatics::GetActorOfClass(GetWorld(), ATarget::StaticClass());
	return CurrentTarget ? Cast<ATarget>(CurrentTarget) : nullptr;
}

void AMTDataCollectorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AMTDataCollectorCharacter::OnPrimaryAction);
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &AMTDataCollectorCharacter::TurnWithMouse);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &AMTDataCollectorCharacter::LookUpWithMouse);
}

void AMTDataCollectorCharacter::OnPrimaryAction()
{
	GetWorldTimerManager().SetTimer(MousePollingHandler, this, &AMTDataCollectorCharacter::PollTrajectory,
	                                1.f / 60.f, true, 0.0f);

	FString OutString;
	const auto CurrentTime = FDateTime::Now() - StartTime;
	OutString += FString::SanitizeFloat(CurrentTime.GetTotalMilliseconds()) + ",";

	const FVector PlayerLocation = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
	const FRotator PlayerRotation = ForwardVector.Rotation();
	OutString += FString::SanitizeFloat(PlayerRotation.Pitch) + "," + FString::SanitizeFloat(PlayerRotation.Yaw) + ",";

	if (const ATarget* RefTarget = GetCurrentTarget(); IsValid(RefTarget))
	{
		const FVector VectorToRefTarget = RefTarget->GetActorLocation() - PlayerLocation;
		const FRotator RefTargetAngle = VectorToRefTarget.Rotation();
		OutString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," + FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
	}
	else
	{
		OutString += "NA,NA,";
	}

	OutString += "TRUE,";

	FHitResult* HitResult = new FHitResult();
	const FVector EndTrace = ForwardVector * 10000.f + PlayerLocation;
	if (const FCollisionQueryParams* TraceParams = new FCollisionQueryParams(); GetWorld()->
		LineTraceSingleByChannel(*HitResult, PlayerLocation, EndTrace, ECC_Visibility, *TraceParams))
	{
		const ATarget* Target = Cast<ATarget>(HitResult->GetActor());
		OutString += IsValid(Target) ? "TRUE\n" : "FALSE\n";
	}
	else
	{
		OutString += "FALSE\n";
	}

	FFileHelper::SaveStringToFile(OutString, *WritePath, FFileHelper::EEncodingOptions::AutoDetect,
	                              &IFileManager::Get(), FILEWRITE_Append);

	OnUseItem.Broadcast();
}

void AMTDataCollectorCharacter::TurnWithMouse(const float Value)
{
	AddControllerYawInput(Value * MouseSensitivity);
}

void AMTDataCollectorCharacter::LookUpWithMouse(const float Value)
{
	AddControllerPitchInput(Value * MouseSensitivity);
}
