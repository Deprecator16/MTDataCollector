#include "MTDataCollectorCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"

#include "Target.h"
#include "MTNetwork.h"

AMTDataCollectorCharacter::AMTDataCollectorCharacter()
{
	MouseSensitivity = 1.0f;
	bUsingNeuralNetwork = false;
	NeuralNetworkIsReady = false;
	bHasFired = false;
	NeuralNetIndex = 0;
	WritePath = FPaths::ProjectConfigDir() + TEXT("DATA") + FDateTime::Now().ToString() + TEXT(".csv");
	ModelPath = TEXT("E:/SSD Repos/UE5 SSD Projects/MTDataCollector/Content/Models/model.onnx");

	FFileHelper::SaveStringToFile(TEXT("Timestamp(ms),PlayerRotX,PlayerRotY,TargetRotX,TargetRotY,Fired,HitTarget\n"),
		*WritePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);

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

#if USE_SHARED_NNI_MEMORY
	SharedTrajectoryBlock = MakeUnique<FSharedMemory>("TrajectoryMemBlock", sizeof(float) * Points_Per_Trajectory * 2);
#endif
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

void AMTDataCollectorCharacter::DoNeuralNetMovement()
{
	const FRotator Delta(Trajectory[NeuralNetIndex * 2], Trajectory[NeuralNetIndex * 2 + 1], 0.f);
	const FRotator NewRotation = GetController()->GetControlRotation() + Delta;
	GetController()->SetControlRotation(NewRotation);

	if (NeuralNetIndex++ == 63)
		GetWorldTimerManager().ClearTimer(NeuralNetHandler);
}

void AMTDataCollectorCharacter::CheckIfTargetUp()
{
	if (const ATarget* RefTarget = GetCurrentTarget(); IsValid(RefTarget))
	{
		const FVector VectorToRefTarget = RefTarget->GetActorLocation() - GetFirstPersonCameraComponent()->
			GetComponentLocation();
		const FRotator Normalized = VectorToRefTarget.Rotation() - GetController()->GetControlRotation();

		// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
		const float dx = Normalized.Pitch / 64.f;
		// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
		const float dy = Normalized.Yaw / 64.f;
		InArr.Reset(Points_Per_Trajectory * 2);
		for (int i = 0; i < Points_Per_Trajectory; i++)
		{
			InArr.Add(dx * i);
			InArr.Add(dy * i);
		}

		if (bUsingNeuralNetwork && !IsValid(Network))
		{
			Network = NewObject<UMTNetwork>(GetTransientPackage(), UMTNetwork::StaticClass());
			Network->InitModel(ModelPath);
		}

		Trajectory = Network->URunModel(InArr);
		GetWorldTimerManager().SetTimer(NeuralNetHandler, this, &AMTDataCollectorCharacter::DoNeuralNetMovement,
		                                1.f / 60.f, true, 0.0f);
	}
	else
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AMTDataCollectorCharacter::CheckIfTargetUp);
	}
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
	if (bUsingNeuralNetwork)
	{
		NeuralNetIndex = 0;
		GetWorldTimerManager().SetTimerForNextTick(this, &AMTDataCollectorCharacter::CheckIfTargetUp);
	}
	else
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
