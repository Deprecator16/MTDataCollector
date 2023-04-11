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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* TargetMesh;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	AMTDataCollectorCharacter* Character;
};
