#include "SolidStrongholdInterior.h"
#include "SolidMaterials.h"
#include "Strongholds/Furnishings/Actors/SolidFurnishing.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ASolidStrongholdInterior::ASolidStrongholdInterior()
{
	PrimaryActorTick.bCanEverTick = false;
}

int32 ASolidStrongholdInterior::ArrangeFloorCount() const
{
	return 0;
}

bool ASolidStrongholdInterior::CollectArrangeFloor(
	const int32 Floor,
	TArray<SolidStrongholdGrid::FHex>& OutCells) const
{
	(void)Floor;
	OutCells.Reset();
	return false;
}

bool ASolidStrongholdInterior::ArrangeFloorZ(const int32 Floor, float& OutZ) const
{
	(void)Floor;
	OutZ = 0.f;
	return false;
}

bool ASolidStrongholdInterior::ArrangeEntrance(const int32 Floor, int32& OutX, int32& OutY) const
{
	(void)Floor;
	OutX = 0;
	OutY = 0;
	return false;
}

int32 ASolidStrongholdInterior::ArrangeFloorUnderFeet(const FVector& WorldFeet) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldFeet);
	int32 Best = INDEX_NONE;
	float BestZ = -TNumericLimits<float>::Max();
	constexpr float SlackCm = 40.f;
	for (int32 Floor = 0; Floor < ArrangeFloorCount(); ++Floor)
	{
		float Z = 0.f;
		if (!ArrangeFloorZ(Floor, Z))
		{
			continue;
		}
		if (Local.Z + SlackCm >= Z && Z >= BestZ)
		{
			Best = Floor;
			BestZ = Z;
		}
	}
	return Best;
}

void ASolidStrongholdInterior::ShowArrangeHighlight(
	const TArray<SolidStrongholdGrid::FHex>& Hexes,
	const float FloorZ)
{
	if (Hexes.Num() == 0)
	{
		HideArrangeHighlight();
		return;
	}
	if (!ArrangeHighlight)
	{
		ArrangeHighlight = NewObject<UInstancedStaticMeshComponent>(this, TEXT("ArrangeHighlight"));
		if (!ArrangeHighlight)
		{
			return;
		}
		ArrangeHighlight->SetupAttachment(GetRootComponent());
		ArrangeHighlight->SetMobility(EComponentMobility::Movable);
		ArrangeHighlight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ArrangeHighlight->SetCastShadow(false);
		if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			ArrangeHighlight->SetStaticMesh(Cube);
		}
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			this, FLinearColor(1.f, 0.95f, 0.45f), TEXT("ArrangeHighlight"), 0.45f))
		{
			ArrangeHighlight->SetMaterial(0, Material);
		}
		if (GetWorld())
		{
			ArrangeHighlight->RegisterComponent();
		}
	}

	ArrangeHighlight->SetVisibility(true);
	ArrangeHighlight->ClearInstances();
	constexpr float ShapeCm = 100.f;
	const float Thickness = SolidStrongholdGrid::LineThicknessCm * 2.2f;
	constexpr float Height = 4.f;
	const float Z = FloorZ + SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm + Height * 0.5f;
	for (const SolidStrongholdGrid::FHex& Hex : Hexes)
	{
		FVector2D Vertices[6];
		SolidStrongholdGrid::HexVertices(Hex.Center, Vertices);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector2D& A = Vertices[Index];
			const FVector2D& B = Vertices[(Index + 1) % 6];
			const FVector2D Delta = B - A;
			const float Length = Delta.Size();
			if (Length < 1.f)
			{
				continue;
			}
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			const FVector Mid((A.X + B.X) * 0.5f, (A.Y + B.Y) * 0.5f, Z);
			ArrangeHighlight->AddInstance(FTransform(
				FRotator(0.f, Yaw, 0.f),
				Mid,
				FVector(Length / ShapeCm, Thickness / ShapeCm, Height / ShapeCm)));
		}
	}
}

void ASolidStrongholdInterior::HideArrangeHighlight()
{
	if (!ArrangeHighlight)
	{
		return;
	}
	ArrangeHighlight->ClearInstances();
	ArrangeHighlight->SetVisibility(false);
}

void ASolidStrongholdInterior::MoveFurnishingTo(
	ASolidFurnishing* Piece,
	const FVector& LocalPoint,
	const float LocalYaw) const
{
	if (!Piece)
	{
		return;
	}
	const FTransform ActorXf = GetActorTransform();
	const FVector WorldPos = ActorXf.TransformPosition(LocalPoint);
	const FRotator WorldRot = ActorXf.TransformRotation(
		FRotator(0.f, LocalYaw, 0.f).Quaternion()).Rotator();
	Piece->SetActorLocationAndRotation(WorldPos, WorldRot, false, nullptr, ETeleportType::TeleportPhysics);
}
