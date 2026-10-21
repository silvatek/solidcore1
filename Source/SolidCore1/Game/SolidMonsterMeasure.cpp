#include "SolidMonsterMeasure.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace SolidMonsterMeasurePrivate
{
	/** Engine cylinder and cube are 100 cm, with the pivot at the centre. The cylinder's radius is 50 cm. */
	constexpr float ShapeHalfWidthCm = 50.f;
	constexpr float ShapeHeightCm = 100.f;

	void Prepare(UStaticMeshComponent* Part, const bool bCastShadow)
	{
		if (!Part)
		{
			return;
		}
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(bCastShadow);
		Part->SetCanEverAffectNavigation(false);
	}

	void Place(
		UStaticMeshComponent* Part,
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		const TCHAR* DebugName,
		ASolidMonsterMeasure* Owner)
	{
		if (!Part || !Mesh || !Owner)
		{
			return;
		}
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(Owner, Color, DebugName, 0.6f))
		{
			Part->SetMaterial(0, Material);
		}
	}
}

ASolidMonsterMeasure::ASolidMonsterMeasure()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	RedPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RedPost"));
	RedPost->SetupAttachment(SceneRoot);
	BluePost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BluePost"));
	BluePost->SetupAttachment(SceneRoot);
	SolidMonsterMeasurePrivate::Prepare(RedPost, true);
	SolidMonsterMeasurePrivate::Prepare(BluePost, true);

	for (int32 Index = 0; Index < SolidMonsterMeasure::GridLineCount; ++Index)
	{
		UStaticMeshComponent* Line = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Grid%d"), Index));
		Line->SetupAttachment(SceneRoot);
		SolidMonsterMeasurePrivate::Prepare(Line, false);
		GridLines[Index] = Line;
	}
}

void ASolidMonsterMeasure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidMonsterMeasure::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

const UStaticMeshComponent* ASolidMonsterMeasure::GetGridLine(const int32 Index) const
{
	if (Index < 0 || Index >= SolidMonsterMeasure::GridLineCount)
	{
		return nullptr;
	}
	return GridLines[Index];
}

void ASolidMonsterMeasure::BuildVisuals()
{
	using namespace SolidMonsterMeasure;
	using namespace SolidMonsterMeasurePrivate;

	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cylinder || !Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidMonsterMeasure: missing Engine BasicShapes."));
		return;
	}

	const FLinearColor Red(0.85f, 0.08f, 0.05f);
	const FLinearColor Blue(0.10f, 0.28f, 0.95f);
	const FLinearColor Yellow(0.95f, 0.75f, 0.05f);

	Place(
		RedPost,
		Cylinder,
		FVector(0.f, RedPostSideCm, RedPostCm * 0.5f),
		FVector(PostRadiusCm / ShapeHalfWidthCm, PostRadiusCm / ShapeHalfWidthCm, RedPostCm / ShapeHeightCm),
		Red,
		TEXT("MonsterPost100"),
		this);
	Place(
		BluePost,
		Cylinder,
		FVector(0.f, BluePostSideCm, BluePostCm * 0.5f),
		FVector(PostRadiusCm / ShapeHalfWidthCm, PostRadiusCm / ShapeHalfWidthCm, BluePostCm / ShapeHeightCm),
		Blue,
		TEXT("MonsterPost200"),
		this);

	const float SpanCm = GridHalfCm * 2.f;
	const float LineZ = LineThicknessCm * 0.5f;
	const FVector AlongX(SpanCm / ShapeHeightCm, LineWidthCm / ShapeHeightCm, LineThicknessCm / ShapeHeightCm);
	const FVector AlongY(LineWidthCm / ShapeHeightCm, SpanCm / ShapeHeightCm, LineThicknessCm / ShapeHeightCm);

	for (int32 Index = 0; Index < GridLinesPerAxis; ++Index)
	{
		const float Offset = -GridHalfCm + static_cast<float>(Index) * GridStepCm;
		Place(
			GridLines[Index],
			Cube,
			FVector(0.f, Offset, LineZ),
			AlongX,
			Yellow,
			TEXT("MonsterGrid"),
			this);
		Place(
			GridLines[GridLinesPerAxis + Index],
			Cube,
			FVector(Offset, 0.f, LineZ),
			AlongY,
			Yellow,
			TEXT("MonsterGrid"),
			this);
	}
}
