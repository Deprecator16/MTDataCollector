#include "Target.h"

ATarget::ATarget()
{
	PrimaryActorTick.bCanEverTick = false;
	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup Mesh"));
	RootComponent = TargetMesh;
}

void ATarget::BeginPlay()
{
	Super::BeginPlay();
}

void ATarget::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATarget::DestroyTarget()
{
	Destroy();
}
