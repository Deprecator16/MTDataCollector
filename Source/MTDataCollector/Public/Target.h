#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Target.generated.h"

class AMTDataCollectorCharacter;

UCLASS()
class MTDATACOLLECTOR_API ATarget : public AActor
{
	GENERATED_BODY()

public:
	ATarget();

protected:
	virtual void BeginPlay() override;

private:
	// The character that will be shooting the targets
	UPROPERTY()
		AMTDataCollectorCharacter* Character;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* TargetMesh;

	virtual void Tick(float DeltaTime) override;
	void DestroyTarget();
};
