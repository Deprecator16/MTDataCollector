#include "MTDataCollectorCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"

#include "Target.h"

AMTDataCollectorCharacter::AMTDataCollectorCharacter() :
	bHasFired(false),
	WritePath(FPaths::ProjectDir() + TEXT("/DATA/Moving/")),
	FileName(TEXT("MOVING") + FDateTime::Now().ToString() + TEXT(".csv")), AnglePollRateHertz(60.0)
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
	StartTime = FDateTime::Now();

	switch (TargetManagerMode)
	{
	case ETargetManagerMode::Static:
		WritePath = FPaths::ProjectDir() + TEXT("/DATA/Static/");
		FileName = TEXT("STATIC") + FDateTime::Now().ToString() + TEXT(".csv");
		WriteStringToFile(
			TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,Fired,HitTarget\n"), FileName);
		break;
	case ETargetManagerMode::LargeAngle:
		WritePath = FPaths::ProjectDir() + TEXT("/DATA/LargeAngles/");
		FileName = TEXT("LARGEANGLE") + FDateTime::Now().ToString() + TEXT(".csv");
		WriteStringToFile(
			TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,Fired,HitTarget\n"), FileName);
		break;
	case ETargetManagerMode::Tracking:
		WritePath = FPaths::ProjectDir() + TEXT("/DATA/Tracking/");
		FileName = TEXT("TRACKING") + FDateTime::Now().ToString() + TEXT(".csv");
		WriteStringToFile(
			TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,JustSpawned,OnTarget\n"), FileName);
		break;
	case ETargetManagerMode::ReactionTime:
		bHasFired = true;
		WritePath = FPaths::ProjectDir() + TEXT("/DATA/ReactionTime/");
		FileName = TEXT("REACTIONTIME") + FDateTime::Now().ToString() + TEXT(".csv");
		WriteStringToFile(
			TEXT("Timestamp(ms),Spawned, Clicked\n"), FileName);
		break;
	case ETargetManagerMode::Moving:
	default:
		WritePath = FPaths::ProjectDir() + TEXT("/DATA/Moving/");
		FileName = TEXT("MOVING") + FDateTime::Now().ToString() + TEXT(".csv");
		WriteStringToFile(
			TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,Fired,HitTarget\n"), FileName);
		break;
	}
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

	switch (TargetManagerMode)
	{
	case ETargetManagerMode::ReactionTime:
		WritePrimaryClickReactionTimeToDataFile();
		break;
	case ETargetManagerMode::Moving:
	case ETargetManagerMode::Static:
	case ETargetManagerMode::LargeAngle:
	default:
		WritePrimaryClickStaticToDataFile();
	}
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

FRotator AMTDataCollectorCharacter::GetPlayerRotation() const
{
	return GetFirstPersonCameraComponent()->GetForwardVector().Rotation();
}

void AMTDataCollectorCharacter::PollPlayerAngle() const
{
	switch (TargetManagerMode)
	{
	case ETargetManagerMode::ReactionTime:
		break;
	case ETargetManagerMode::Tracking:
		WritePollAngleTrackingToDataFile();
		break;
	case ETargetManagerMode::Moving:
	case ETargetManagerMode::Static:
	case ETargetManagerMode::LargeAngle:
	default:
		WritePollAngleStaticToDataFile();
		break;
	}
}

ATarget* AMTDataCollectorCharacter::GetCurrentTarget() const
{
	AActor* CurrentTarget = UGameplayStatics::GetActorOfClass(GetWorld(), ATarget::StaticClass());
	return IsValid(CurrentTarget) ? Cast<ATarget>(CurrentTarget) : nullptr;
}

bool AMTDataCollectorCharacter::WriteStringToFile(const FString& Text, const FString& File) const
{
	return FFileHelper::SaveStringToFile(Text, *(WritePath + File), FFileHelper::EEncodingOptions::AutoDetect,
	                                     &IFileManager::Get(), FILEWRITE_Append);
}

void AMTDataCollectorCharacter::WritePrimaryClickStaticToDataFile() const
{
	FString OutString;
	const auto CurrentTime = FDateTime::Now() - StartTime;
	OutString += FString::SanitizeFloat(CurrentTime.GetTotalMilliseconds()) + ",";

	const FVector PlayerLocation = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FRotator PlayerRotation = GetPlayerRotation();
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
	const FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
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

	if (!WriteStringToFile(OutString, FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Data write failed in WritePrimaryClickStaticToDataFile."));
	}
}

void AMTDataCollectorCharacter::WritePrimaryClickReactionTimeToDataFile() const
{
	FString OutString;

	const FTimespan CurrentTime = FDateTime::Now() - StartTime;
	OutString += FString::SanitizeFloat(CurrentTime.GetTotalMilliseconds()) + ",";

	OutString += "FALSE,TRUE\n";

	if (!WriteStringToFile(OutString, FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Data write failed in WritePrimaryClickReactionTimeToDataFile."));
	}
}

void AMTDataCollectorCharacter::WritePollAngleStaticToDataFile() const
{
	FString OutString;

	const FTimespan CurrentTime = FDateTime::Now() - StartTime;
	OutString += FString::SanitizeFloat(CurrentTime.GetTotalMilliseconds()) + ",";

	const FRotator PlayerRotation = GetPlayerRotation();
	OutString += FString::SanitizeFloat(PlayerRotation.Pitch) + "," + FString::SanitizeFloat(PlayerRotation.Yaw) + ",";

	if (const ATarget* RefTarget = GetCurrentTarget(); IsValid(RefTarget))
	{
		const FVector RefTargetPos = RefTarget->GetActorLocation();
		const FVector VectorToRefTarget = RefTargetPos - GetFirstPersonCameraComponent()->GetComponentLocation();
		const FRotator RefTargetAngle = VectorToRefTarget.Rotation();
		OutString += FString::SanitizeFloat(RefTargetAngle.Pitch) + "," +
			FString::SanitizeFloat(RefTargetAngle.Yaw) + ",";
	}

	OutString += "\n";

	if (!WriteStringToFile(OutString, FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Data write failed in WritePollAngleStaticToDataFile."));
	}
}

void AMTDataCollectorCharacter::WritePollAngleTrackingToDataFile() const
{
	FString OutString;
	const auto CurrentTime = FDateTime::Now() - StartTime;
	const FRotator PlayerRotation = GetPlayerRotation();

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

	OutString += "FALSE,";

	FHitResult* HitResult = new FHitResult();
	const FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
	const FVector PlayerLocation = GetFirstPersonCameraComponent()->GetComponentLocation();
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

	if (!WriteStringToFile(OutString, FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Data write failed in WritePollAngleTrackingToDataFile."));
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AMTDataCollectorCharacter::WriteTrackingTargetSpawnedToDataFile()
{
	FString OutString;
	const auto CurrentTime = FDateTime::Now() - StartTime;
	const FRotator PlayerRotation = GetPlayerRotation();

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

	OutString += "TRUE,";

	FHitResult* HitResult = new FHitResult();
	const FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
	const FVector PlayerLocation = GetFirstPersonCameraComponent()->GetComponentLocation();
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

	if (!WriteStringToFile(OutString, FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Data write failed in WritePollAngleTrackingToDataFile."));
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AMTDataCollectorCharacter::WriteReactionTimeTargetSpawnedToDataFile()
{
	FString OutString;

	const FTimespan CurrentTime = FDateTime::Now() - StartTime;
	OutString += FString::SanitizeFloat(CurrentTime.GetTotalMilliseconds()) + ",";

	OutString += "TRUE,FALSE\n";

	if (!WriteStringToFile(OutString, FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Data write failed in WritePrimaryClickReactionTimeToDataFile."));
	}
}

void AMTDataCollectorCharacter::DoConfigSave()
{
	SaveConfig();
}
