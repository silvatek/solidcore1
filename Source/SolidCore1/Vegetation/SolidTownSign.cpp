#include "SolidTownSign.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidTownSigns.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	void SetupSignText(UTextRenderComponent* Text, const FString& Label, const FColor& Color)
	{
		if (!Text)
		{
			return;
		}

		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(SolidTownSigns::TextWorldSize);
		Text->SetTextRenderColor(Color);
		Text->SetText(FText::FromString(Label));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->SetCastShadow(false);
		Text->SetRelativeRotation(FRotator::ZeroRotator);
		Text->SetVisibility(true);
	}
}

ASolidTownSign::ASolidTownSign()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PoleMesh"));
	PoleMesh->SetupAttachment(SceneRoot);
	PoleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PoleMesh->SetCollisionResponseToAllChannels(ECR_Block);
	PoleMesh->SetCastShadow(true);

	BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardMesh"));
	BoardMesh->SetupAttachment(SceneRoot);
	BoardMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoardMesh->SetCollisionResponseToAllChannels(ECR_Block);
	BoardMesh->SetCastShadow(true);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(SceneRoot);

	OutlineUp = CreateDefaultSubobject<UTextRenderComponent>(TEXT("OutlineUp"));
	OutlineUp->SetupAttachment(SceneRoot);
	OutlineDown = CreateDefaultSubobject<UTextRenderComponent>(TEXT("OutlineDown"));
	OutlineDown->SetupAttachment(SceneRoot);
	OutlineLeft = CreateDefaultSubobject<UTextRenderComponent>(TEXT("OutlineLeft"));
	OutlineLeft->SetupAttachment(SceneRoot);
	OutlineRight = CreateDefaultSubobject<UTextRenderComponent>(TEXT("OutlineRight"));
	OutlineRight->SetupAttachment(SceneRoot);
}

void ASolidTownSign::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidTownSign::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidTownSign::SetTownName(const FString& InName)
{
	TownName = InName;
}

FString ASolidTownSign::GetLabel() const
{
	return SolidTownSigns::MakeLabel(TownName);
}

UTextRenderComponent* ASolidTownSign::GetOutlineText(const int32 Index) const
{
	switch (Index)
	{
	case 0: return OutlineUp;
	case 1: return OutlineDown;
	case 2: return OutlineLeft;
	case 3: return OutlineRight;
	default: return nullptr;
	}
}

void ASolidTownSign::BuildVisuals()
{
	const FString Label = GetLabel();
	const int32 CharCount = FMath::Max(Label.Len(), 1);
	const float TextWidth = SolidTownSigns::TextWorldSize * 0.55f * static_cast<float>(CharCount);
	const float BoardWidth = TextWidth + SolidTownSigns::TextWorldSize * 0.9f;
	const float BoardHeight = SolidTownSigns::TextWorldSize * 1.85f;

	constexpr float ShapeSize = 100.f;
	const float BoardCenterX = (SolidTownSigns::PoleThicknessCm + SolidTownSigns::BoardDepthCm) * 0.5f;
	const float BoardCenterZ = SolidTownSigns::PoleHeightCm - BoardHeight * 0.5f;
	const float OutlineX = BoardCenterX + SolidTownSigns::BoardDepthCm * 0.5f + 2.f;
	const float LabelX = OutlineX + 1.f;

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidTownSign: missing Engine BasicShapes Cube."));
		return;
	}

	if (PoleMesh)
	{
		PoleMesh->SetStaticMesh(Cube);
		PoleMesh->SetRelativeLocation(FVector(0.f, 0.f, SolidTownSigns::PoleHeightCm * 0.5f));
		PoleMesh->SetRelativeRotation(FRotator::ZeroRotator);
		PoleMesh->SetRelativeScale3D(FVector(
			SolidTownSigns::PoleThicknessCm / ShapeSize,
			SolidTownSigns::PoleThicknessCm / ShapeSize,
			SolidTownSigns::PoleHeightCm / ShapeSize));
		if (UMaterialInterface* PoleMat = SolidMaterials::CreateSolidColor(
				this, SolidTownSigns::PoleColor(), TEXT("TownSignPole"), 0.92f))
		{
			PoleMesh->SetMaterial(0, PoleMat);
		}
	}

	if (BoardMesh)
	{
		BoardMesh->SetStaticMesh(Cube);
		BoardMesh->SetRelativeLocation(FVector(BoardCenterX, 0.f, BoardCenterZ));
		BoardMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BoardMesh->SetRelativeScale3D(FVector(
			SolidTownSigns::BoardDepthCm / ShapeSize,
			BoardWidth / ShapeSize,
			BoardHeight / ShapeSize));
		if (UMaterialInterface* BoardMat = SolidMaterials::CreateSolidColor(
				this, SolidTownSigns::BoardColor(), TEXT("TownSignBoard"), 0.88f))
		{
			BoardMesh->SetMaterial(0, BoardMat);
		}
	}

	SetupSignText(LabelText, Label, FColor::White);
	if (LabelText)
	{
		LabelText->SetRelativeLocation(FVector(LabelX, 0.f, BoardCenterZ));
	}

	const float Offset = SolidTownSigns::OutlineOffsetCm;
	UTextRenderComponent* Outlines[] = { OutlineUp, OutlineDown, OutlineLeft, OutlineRight };
	const FVector OutlineOffsets[] = {
		FVector(OutlineX, 0.f, BoardCenterZ + Offset),
		FVector(OutlineX, 0.f, BoardCenterZ - Offset),
		FVector(OutlineX, -Offset, BoardCenterZ),
		FVector(OutlineX, Offset, BoardCenterZ),
	};
	for (int32 Index = 0; Index < OutlineCount; ++Index)
	{
		SetupSignText(Outlines[Index], Label, FColor::Black);
		if (Outlines[Index])
		{
			Outlines[Index]->SetRelativeLocation(OutlineOffsets[Index]);
		}
	}
}
