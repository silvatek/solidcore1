#include "SolidFortressExterior.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Strongholds/SolidStronghold.h"
#include "Terrain/SolidTerrainSeat.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace SolidFortressExteriorPrivate
{
	constexpr float CubeSizeCm = 100.f;

	/** Plan +X is east and plan +Y is north. Actor -X is that north, so the gate faces the grass. */
	FVector ActorCenter(const float Width, const float Depth, const float PlanX, const float PlanY, const float Z)
	{
		return FVector(Depth * 0.5f - PlanY, PlanX - Width * 0.5f, Z);
	}
}

ASolidFortressExterior::ASolidFortressExterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolidFortressExterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidFortressExterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidFortressExterior::DiscardParts()
{
	auto Drop = [](TObjectPtr<UStaticMeshComponent>& Part)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
			Part = nullptr;
		}
	};

	for (TObjectPtr<UStaticMeshComponent>& Part : Curtain)
	{
		Drop(Part);
	}
	Curtain.Reset();
	for (TObjectPtr<UStaticMeshComponent>& Part : Buildings)
	{
		Drop(Part);
	}
	Buildings.Reset();
	Drop(GateMesh);
}

UStaticMeshComponent* ASolidFortressExterior::AddPart(
	UStaticMesh* Mesh,
	const FVector& Location,
	const FVector& Scale,
	const FLinearColor& Color,
	const TCHAR* DebugName)
{
	if (!Mesh || !SceneRoot)
	{
		return nullptr;
	}

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
	if (!Part)
	{
		return nullptr;
	}

	Part->SetupAttachment(SceneRoot);
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(FRotator::ZeroRotator);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Part->SetCollisionResponseToAllChannels(ECR_Block);
	Part->SetCastShadow(true);
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Color, DebugName, 0.94f))
	{
		Part->SetMaterial(0, Material);
	}
	if (GetWorld())
	{
		Part->RegisterComponent();
	}
	return Part;
}

void ASolidFortressExterior::BuildVisuals()
{
	DiscardParts();
	if (!Fortress)
	{
		return;
	}

	const float Width = Fortress->OuterWidthCm();
	const float Depth = Fortress->OuterDepthCm();
	const float Height = Fortress->CurtainHeightCm();
	const float Wall = FSolidStronghold::WallCm;
	const float Opening = FSolidStronghold::OpeningCm;
	const float Storey = FSolidStronghold::StoreyCm;
	if (Width <= Wall || Depth <= Wall || Height <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidFortressExterior: missing Engine BasicShapes Cube."));
		return;
	}

	const FLinearColor Rock = SolidStrongholds::RockColor();
	SolidTerrainSeat::FFootprint Seat;
	Seat.HalfXCm = Depth * 0.5f;
	Seat.HalfYCm = Width * 0.5f;
	const float Drop = SolidTerrainSeat::DropCm(this, Seat);

	auto Place = [&](
		const float PlanCenterX,
		const float PlanCenterY,
		const float CenterZ,
		const float PlanSizeX,
		const float PlanSizeY,
		const float SizeZ,
		const FLinearColor& Color,
		const TCHAR* DebugName,
		const bool bBury) -> UStaticMeshComponent*
	{
		if (PlanSizeX <= KINDA_SMALL_NUMBER || PlanSizeY <= KINDA_SMALL_NUMBER || SizeZ <= KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		UStaticMeshComponent* Part = AddPart(
			Cube,
			SolidFortressExteriorPrivate::ActorCenter(Width, Depth, PlanCenterX, PlanCenterY, CenterZ),
			FVector(
				PlanSizeY / SolidFortressExteriorPrivate::CubeSizeCm,
				PlanSizeX / SolidFortressExteriorPrivate::CubeSizeCm,
				SizeZ / SolidFortressExteriorPrivate::CubeSizeCm),
			Color,
			DebugName);
		if (bBury)
		{
			SolidTerrainSeat::BuryCentered(
				Part,
				CenterZ - SizeZ * 0.5f,
				CenterZ + SizeZ * 0.5f,
				Drop,
				SolidFortressExteriorPrivate::CubeSizeCm);
		}
		return Part;
	};

	auto AddCurtain = [&](
		const float PlanCenterX,
		const float PlanCenterY,
		const float CenterZ,
		const float PlanSizeX,
		const float PlanSizeY,
		const float SizeZ,
		const bool bBury)
	{
		if (UStaticMeshComponent* Part = Place(
			PlanCenterX, PlanCenterY, CenterZ, PlanSizeX, PlanSizeY, SizeZ, Rock, TEXT("FortressCurtain"), bBury))
		{
			Curtain.Add(Part);
		}
	};

	const float GateMinX = (Width - Opening) * 0.5f;
	const float GateMaxX = GateMinX + Opening;
	const float NorthY = Depth - Wall * 0.5f;

	AddCurtain(Width * 0.5f, Wall * 0.5f, Height * 0.5f, Width, Wall, Height, true);
	AddCurtain(Wall * 0.5f, Depth * 0.5f, Height * 0.5f, Wall, Depth - 2.f * Wall, Height, true);
	AddCurtain(Width - Wall * 0.5f, Depth * 0.5f, Height * 0.5f, Wall, Depth - 2.f * Wall, Height, true);
	AddCurtain(GateMinX * 0.5f, NorthY, Height * 0.5f, GateMinX, Wall, Height, true);
	AddCurtain((GateMaxX + Width) * 0.5f, NorthY, Height * 0.5f, Width - GateMaxX, Wall, Height, true);
	if (Height > Storey + KINDA_SMALL_NUMBER)
	{
		AddCurtain(
			Width * 0.5f,
			NorthY,
			(Storey + Height) * 0.5f,
			Opening,
			Wall,
			Height - Storey,
			false);
	}

	for (const SolidStronghold::FSolidFortressBuilding& One : Fortress->Buildings)
	{
		float MinX = One.InnerMinCm.X - Wall;
		float MaxX = One.InnerMinCm.X + One.WidthCm() + Wall;
		float MinY = One.InnerMinCm.Y - Wall;
		float MaxY = One.InnerMinCm.Y + One.DepthCm() + Wall;
		if (FMath::IsNearlyEqual(MinX, 0.f, 0.1f))
		{
			MinX += Wall;
		}
		if (FMath::IsNearlyEqual(MaxX, Width, 0.1f))
		{
			MaxX -= Wall;
		}
		if (FMath::IsNearlyEqual(MinY, 0.f, 0.1f))
		{
			MinY += Wall;
		}
		if (FMath::IsNearlyEqual(MaxY, Depth, 0.1f))
		{
			MaxY -= Wall;
		}

		const float SizeX = MaxX - MinX;
		const float SizeY = MaxY - MinY;
		const float BuildingHeight = One.HeightCm(Fortress->ParapetCm);
		const float Parapet = Fortress->ParapetCm;
		const bool bMerlons = One.Floors.Num() >= 2 && Parapet > KINDA_SMALL_NUMBER;
		const float BodyHeight = bMerlons ? BuildingHeight - Parapet : BuildingHeight;
		if (UStaticMeshComponent* Part = Place(
			(MinX + MaxX) * 0.5f,
			(MinY + MaxY) * 0.5f,
			BodyHeight * 0.5f,
			SizeX,
			SizeY,
			BodyHeight,
			Rock,
			TEXT("FortressBuilding"),
			true))
		{
			Buildings.Add(Part);
		}
		if (!bMerlons)
		{
			continue;
		}

		const float MerlonZ = BodyHeight + Parapet * 0.5f;
		const float LowHeight = Parapet * 0.5f;
		const float LowZ = BodyHeight + LowHeight * 0.5f;
		auto AddPiece = [&](
			const float CenterAlong,
			const float SizeAlong,
			const bool bAlongX,
			const float Fixed,
			const float PieceHeight,
			const float CenterZ)
		{
			UStaticMeshComponent* Part = bAlongX
				? Place(CenterAlong, Fixed, CenterZ, SizeAlong, Parapet, PieceHeight, Rock, TEXT("FortressMerlon"), false)
				: Place(Fixed, CenterAlong, CenterZ, Parapet, SizeAlong, PieceHeight, Rock, TEXT("FortressMerlon"), false);
			if (Part)
			{
				Buildings.Add(Part);
			}
		};
		auto AddCorner = [&](const float PlanX, const float PlanY)
		{
			if (UStaticMeshComponent* Part = Place(
				PlanX, PlanY, MerlonZ, Parapet, Parapet, Parapet, Rock, TEXT("FortressMerlon"), false))
			{
				Buildings.Add(Part);
			}
		};
		AddCorner(MinX + Parapet * 0.5f, MinY + Parapet * 0.5f);
		AddCorner(MaxX - Parapet * 0.5f, MinY + Parapet * 0.5f);
		AddCorner(MinX + Parapet * 0.5f, MaxY - Parapet * 0.5f);
		AddCorner(MaxX - Parapet * 0.5f, MaxY - Parapet * 0.5f);

		auto FillBetween = [&](const float Start, const float End, const bool bAlongX, const float Fixed)
		{
			const float Span = End - Start;
			if (Span <= KINDA_SMALL_NUMBER)
			{
				return;
			}
			const int32 LowCount = FMath::FloorToInt(Span / Parapet);
			if (LowCount <= 0)
			{
				AddPiece((Start + End) * 0.5f, Span, bAlongX, Fixed, Parapet, MerlonZ);
				return;
			}
			const float HighEach = (Span - static_cast<float>(LowCount) * Parapet) / static_cast<float>(LowCount + 1);
			float Cursor = Start;
			for (int32 Index = 0; Index < LowCount; ++Index)
			{
				if (HighEach > KINDA_SMALL_NUMBER)
				{
					AddPiece(Cursor + HighEach * 0.5f, HighEach, bAlongX, Fixed, Parapet, MerlonZ);
					Cursor += HighEach;
				}
				AddPiece(Cursor + Parapet * 0.5f, Parapet, bAlongX, Fixed, LowHeight, LowZ);
				Cursor += Parapet;
			}
			if (HighEach > KINDA_SMALL_NUMBER)
			{
				AddPiece(Cursor + HighEach * 0.5f, HighEach, bAlongX, Fixed, Parapet, MerlonZ);
			}
		};
		FillBetween(MinX + Parapet, MaxX - Parapet, true, MinY + Parapet * 0.5f);
		FillBetween(MinX + Parapet, MaxX - Parapet, true, MaxY - Parapet * 0.5f);
		FillBetween(MinY + Parapet, MaxY - Parapet, false, MinX + Parapet * 0.5f);
		FillBetween(MinY + Parapet, MaxY - Parapet, false, MaxX - Parapet * 0.5f);
	}

	GateMesh = Place(
		Width * 0.5f,
		NorthY,
		Storey * 0.5f,
		Opening,
		Wall,
		Storey,
		SolidStrongholds::MouthColor(),
		TEXT("FortressGate"),
		true);
}
