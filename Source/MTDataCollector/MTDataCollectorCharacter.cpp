#include "MTDataCollectorCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"

#include "Target.h"

AMTDataCollectorCharacter::AMTDataCollectorCharacter() :
	MouseSensitivity(1.0f), bHasFired(false),
	WritePath(FPaths::ProjectConfigDir() + TEXT("/DATA/DATA") + FDateTime::Now().ToString() + TEXT("/")),
	FileName(TEXT("FLICKING.csv")), AnglePollRateHertz(60.0)
{
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
}

void AMTDataCollectorCharacter::BeginPlay()
{
	Super::BeginPlay();

	// ReSharper disable once CppExpressionWithoutSideEffects
	WriteStringToDataFile(
		TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,Fired,HitTarget\n"), FileName);
	StartTime = FDateTime::Now();
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
	if (!bHasFired)
	{
		GetWorldTimerManager().SetTimer(MousePollingHandler, this, &AMTDataCollectorCharacter::PollPlayerAngle,
		                                1.f / AnglePollRateHertz, true, 0.0f);
		bHasFired = true;
	}

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
		OutString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," +
			FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
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

	// ReSharper disable once CppExpressionWithoutSideEffects
	WriteStringToDataFile(OutString, FileName);

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

void AMTDataCollectorCharacter::PollPlayerAngle() const
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
		OutString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," +
			FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
	}
	else
	{
		OutString += "NA,NA,";
	}

	OutString += "FALSE,NA\n";

	// ReSharper disable once CppExpressionWithoutSideEffects
	WriteStringToDataFile(OutString, FileName);
}

ATarget* AMTDataCollectorCharacter::GetCurrentTarget() const
{
	AActor* CurrentTarget = UGameplayStatics::GetActorOfClass(GetWorld(), ATarget::StaticClass());
	return IsValid(CurrentTarget) ? Cast<ATarget>(CurrentTarget) : nullptr;
}

bool AMTDataCollectorCharacter::WriteStringToDataFile(const FString& Text, const FString& File) const
{
	return FFileHelper::SaveStringToFile(Text, *(WritePath + File), FFileHelper::EEncodingOptions::AutoDetect,
	                                     &IFileManager::Get(), FILEWRITE_Append);
}
