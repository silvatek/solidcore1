#include "SolidMonsterActor.h"
#include "SolidMonster.h"
#include "SolidRaptor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

ASolidMonster::ASolidMonster()
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

void ASolidMonster::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidMonster::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidMonster::SetChoice(const int32 InChoiceIndex)
{
	ChoiceIndex = InChoiceIndex;
}

void ASolidMonster::BuildVisuals()
{
	const FSolidRaptor Raptor;
	Raptor.ApplyVisuals(Body, ActiveIdle);
}
