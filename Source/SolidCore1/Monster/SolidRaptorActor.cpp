#include "SolidRaptorActor.h"
#include "SolidMonster.h"
#include "SolidRaptor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

ASolidRaptor::ASolidRaptor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(SceneRoot);
	Capsule->InitCapsuleSize(FSolidMonster::CapsuleRadiusCm, FSolidMonster::CapsuleHalfHeightCm);
	Capsule->SetRelativeLocation(FVector(0.f, 0.f, FSolidMonster::CapsuleHalfHeightCm));
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionObjectType(ECC_WorldDynamic);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Capsule->SetGenerateOverlapEvents(false);

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(SceneRoot);
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body->SetRelativeRotation(FRotator(0.f, FSolidMonster::MeshFacingYawDeg, 0.f));
}

void ASolidRaptor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidRaptor::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidRaptor::SetRaptor(const FSolidRaptor& InRaptor)
{
	Raptor = InRaptor;
	if (Capsule)
	{
		const float Scale = Raptor.DrawnBodyFactor() / FSolidRaptor::SmallDrawnBodyFactor;
		const float Radius = FSolidMonster::CapsuleRadiusCm * Scale;
		const float HalfHeight = FSolidMonster::CapsuleHalfHeightCm * Scale;
		Capsule->SetCapsuleSize(Radius, HalfHeight, false);
		// The centre stays at the half height so the bottom is the actor origin.
		Capsule->SetRelativeLocation(FVector(0.f, 0.f, HalfHeight));
	}
	BuildVisuals();
}

void ASolidRaptor::BuildVisuals()
{
	Raptor.ApplyVisuals(Body, ActiveIdle);
}
