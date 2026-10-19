#include "SolidDungeonExterior.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"

namespace SolidDungeonExteriorPrivate
{
	constexpr int32 ProfileCount = 33;

	void BuryGround(TArray<FVector>& Positions, const float BurialCm)
	{
		if (BurialCm <= 0.f)
		{
			return;
		}
		for (FVector& Position : Positions)
		{
			if (Position.Z <= 1.f)
			{
				Position.Z -= BurialCm;
			}
		}
	}
	constexpr int32 RingCount = 12;
	constexpr float SuperellipseN = 4.f;

	float SurfaceScale(const float Theta, const float U)
	{
		const float Fall = FMath::Cos(U * PI * 0.5f);
		const float Side = FMath::Pow(FMath::Abs(FMath::Cos(Theta)), 8.f);
		const float Crown = FMath::Pow(FMath::Abs(FMath::Sin(Theta)), 12.f);
		const float FrontPeak = (1.f - U) * FMath::Max(Side, Crown);
		const float Wave = 0.5f + 0.5f * FMath::Sin(Theta * 5.f + 0.6f) * FMath::Cos(U * PI * 2.f + Theta);
		const float Lump = 0.07f * (1.f - FrontPeak) * (0.4f + 0.6f * Wave);
		return Fall * (1.f - Lump);
	}

	void ProfileYZ(const float Theta, const float Scale, const float FaceWidth, const float FaceHeight, float& OutY, float& OutZ)
	{
		const float Exponent = 2.f / SuperellipseN;
		const float CosT = FMath::Cos(Theta);
		const float SinT = FMath::Max(FMath::Sin(Theta), 0.f);
		OutY = (FaceWidth * 0.5f) * FMath::Sign(CosT) * FMath::Pow(FMath::Abs(CosT), Exponent) * Scale;
		OutZ = FaceHeight * FMath::Pow(SinT, Exponent) * Scale;
	}

	/**
	 * Opening edge. An ellipse stands on the ground inside the opening size,
	 * widest through the middle and narrower at the ground, with a lumpy rim.
	 */
	FVector2D OpeningRim(const float Theta, const float OpeningWidth, const float OpeningHeight)
	{
		const float RadiusY = OpeningWidth * 0.5f;
		const float RadiusZ = OpeningHeight / 1.7f;
		const float CenterZ = OpeningHeight - RadiusZ;
		const float GroundSin = FMath::Clamp(-CenterZ / RadiusZ, -1.f, 1.f);
		const float GroundAngle = FMath::Asin(GroundSin);
		const float Along = Theta / PI;
		const float Angle = FMath::Lerp(GroundAngle, PI - GroundAngle, Along);
		const FVector2D Center(0.f, CenterZ);
		FVector2D Rim(
			RadiusY * FMath::Cos(Angle),
			CenterZ + RadiusZ * FMath::Sin(Angle));
		FVector2D Inward = (Rim - Center).GetSafeNormal();
		if (Inward.IsNearlyZero())
		{
			Inward = FVector2D(0.f, 1.f);
		}
		const FVector2D Tangent(-Inward.Y, Inward.X);
		const float Pull = 16.f * (0.55f + 0.45f * FMath::Sin(Theta * 3.f + 0.6f));
		const float Slide = 10.f * FMath::Sin(Theta * 5.f + 1.1f);
		Rim += -Inward * Pull + Tangent * Slide;
		Rim.X = FMath::Clamp(Rim.X, -RadiusY, RadiusY);
		Rim.Y = FMath::Clamp(Rim.Y, 0.f, OpeningHeight);
		if (Along < 0.02f || Along > 0.98f)
		{
			Rim.Y = 0.f;
		}
		return Rim;
	}

	void AddFace(
		const TArray<FVector>& Positions,
		TArray<FVector>& Normals,
		TArray<int32>& Triangles,
		const int32 A,
		const int32 B,
		const int32 C,
		const FVector& Interior)
	{
		const FVector Cross = FVector::CrossProduct(Positions[B] - Positions[A], Positions[C] - Positions[A]);
		if (Cross.SizeSquared() < 0.01f)
		{
			return;
		}
		// The tower winds clockwise from the outside. That cross product points inward.
		// The stored normal still points outward, so the lit side is the stone.
		const bool bCrossOutward = FVector::DotProduct(Cross, Positions[A] - Interior) >= 0.f;
		const int32 Second = bCrossOutward ? C : B;
		const int32 Third = bCrossOutward ? B : C;
		Triangles.Add(A);
		Triangles.Add(Second);
		Triangles.Add(Third);
		const FVector Normal = (bCrossOutward ? Cross : -Cross).GetSafeNormal();
		Normals[A] += Normal;
		Normals[Second] += Normal;
		Normals[Third] += Normal;
	}

	UStaticMesh* BuildRock(
		UObject* Outer,
		const float FaceWidth,
		const float FaceHeight,
		const float SlopeRun,
		const float OpeningWidth,
		const float OpeningHeight,
		const float BurialCm)
	{
		if (!Outer || FaceWidth <= KINDA_SMALL_NUMBER || FaceHeight <= KINDA_SMALL_NUMBER || SlopeRun <= KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		const int32 Apex = RingCount * ProfileCount;
		const int32 InnerBase = Apex + 1;
		const int32 VertexCount = InnerBase + ProfileCount;
		TArray<FVector> Positions;
		TArray<FVector> Normals;
		Positions.SetNum(VertexCount);
		Normals.SetNumZeroed(VertexCount);

		for (int32 Ring = 0; Ring < RingCount; ++Ring)
		{
			const float U = static_cast<float>(Ring) / static_cast<float>(RingCount);
			const float X = -U * SlopeRun;
			for (int32 Step = 0; Step < ProfileCount; ++Step)
			{
				const float Theta = PI * static_cast<float>(Step) / static_cast<float>(ProfileCount - 1);
				float Y = 0.f;
				float Z = 0.f;
				ProfileYZ(Theta, SurfaceScale(Theta, U), FaceWidth, FaceHeight, Y, Z);
				Positions[Ring * ProfileCount + Step] = FVector(X, Y, Z);
			}
		}
		Positions[Apex] = FVector(-SlopeRun, 0.f, 0.f);
		for (int32 Step = 0; Step < ProfileCount; ++Step)
		{
			const float Theta = PI * static_cast<float>(Step) / static_cast<float>(ProfileCount - 1);
			const FVector2D Rim = OpeningRim(Theta, OpeningWidth, OpeningHeight);
			Positions[InnerBase + Step] = FVector(0.f, Rim.X, Rim.Y);
		}
		BuryGround(Positions, BurialCm);

		TArray<int32> Triangles;
		Triangles.Reserve(ProfileCount * (RingCount + 2) * 6);
		const FVector Interior(-SlopeRun * 0.35f, 0.f, FaceHeight * 0.2f);
		for (int32 Ring = 0; Ring < RingCount - 1; ++Ring)
		{
			for (int32 Step = 0; Step < ProfileCount - 1; ++Step)
			{
				const int32 A = Ring * ProfileCount + Step;
				const int32 B = A + 1;
				const int32 C = A + ProfileCount;
				const int32 D = C + 1;
				AddFace(Positions, Normals, Triangles, A, B, C, Interior);
				AddFace(Positions, Normals, Triangles, B, D, C, Interior);
			}
		}
		const int32 LastRing = (RingCount - 1) * ProfileCount;
		for (int32 Step = 0; Step < ProfileCount - 1; ++Step)
		{
			AddFace(Positions, Normals, Triangles, LastRing + Step, LastRing + Step + 1, Apex, Interior);
		}
		for (int32 Step = 0; Step < ProfileCount - 1; ++Step)
		{
			const int32 OuterA = Step;
			const int32 OuterB = Step + 1;
			const int32 InnerA = InnerBase + Step;
			const int32 InnerB = InnerBase + Step + 1;
			AddFace(Positions, Normals, Triangles, OuterA, InnerA, OuterB, Interior);
			AddFace(Positions, Normals, Triangles, InnerA, InnerB, OuterB, Interior);
		}

		for (FVector& Normal : Normals)
		{
			if (Normal.SizeSquared() < KINDA_SMALL_NUMBER)
			{
				Normal = FVector::UpVector;
			}
			else
			{
				Normal.Normalize();
			}
		}

		FMeshDescription MeshDescription;
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();

		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> InstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> InstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> InstanceColors = Attributes.GetVertexInstanceColors();
		TPolygonGroupAttributesRef<FName> PolygonGroupNames = Attributes.GetPolygonGroupMaterialSlotNames();
		InstanceUVs.SetNumChannels(1);

		const FName SlotName(TEXT("Stone"));
		const FPolygonGroupID PolygonGroup = MeshDescription.CreatePolygonGroup();
		PolygonGroupNames[PolygonGroup] = SlotName;

		TArray<FVertexID> VertexIDs;
		VertexIDs.Reserve(Positions.Num());
		for (const FVector& Position : Positions)
		{
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = FVector3f(Position);
			VertexIDs.Add(VertexID);
		}

		for (int32 Tri = 0; Tri + 2 < Triangles.Num(); Tri += 3)
		{
			TArray<FVertexInstanceID, TInlineAllocator<3>> Corners;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 Vert = Triangles[Tri + Corner];
				const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(VertexIDs[Vert]);
				const FVector Normal = Normals[Vert];
				InstanceNormals[InstanceID] = FVector3f(Normal);
				FVector Tangent = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
				if (Tangent.IsNearlyZero())
				{
					Tangent = FVector::ForwardVector;
				}
				InstanceTangents[InstanceID] = FVector3f(Tangent);
				InstanceBinormalSigns[InstanceID] = 1.f;
				InstanceUVs.Set(InstanceID, 0, FVector2f(0.f, 0.f));
				InstanceColors[InstanceID] = FVector4f(1.f, 1.f, 1.f, 1.f);
				Corners.Add(InstanceID);
			}
			MeshDescription.CreatePolygon(PolygonGroup, Corners);
		}

		UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Transient);
		Mesh->bAllowCPUAccess = true;
		Mesh->NeverStream = true;
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}
		Mesh->SetStaticMaterials({ FStaticMaterial(nullptr, SlotName, SlotName) });

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bFastBuild = false;
		BuildParams.bAllowCpuAccess = true;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = false;

		const TArray<const FMeshDescription*> Descriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(Descriptions, BuildParams))
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonExterior: failed to build the rock."));
			return nullptr;
		}
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}

		if (!Mesh->GetBodySetup())
		{
			Mesh->CreateBodySetup();
		}
		if (UBodySetup* BodySetup = Mesh->GetBodySetup())
		{
			BodySetup->AggGeom.BoxElems.Empty();
			BodySetup->AggGeom.ConvexElems.Empty();
			BodySetup->AggGeom.SphereElems.Empty();
			BodySetup->AggGeom.SphylElems.Empty();
			BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
			BodySetup->bDoubleSidedGeometry = true;
			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
		}
		return Mesh;
	}

	UStaticMesh* BuildMouth(
		UObject* Outer,
		const float OpeningWidth,
		const float OpeningHeight,
		const float Depth,
		const float BurialCm)
	{
		if (!Outer || OpeningWidth <= KINDA_SMALL_NUMBER || OpeningHeight <= KINDA_SMALL_NUMBER || Depth <= KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		const int32 FrontCenter = 0;
		const int32 BackCenter = 1;
		const int32 FrontBase = 2;
		const int32 BackBase = FrontBase + ProfileCount;
		const int32 VertexCount = BackBase + ProfileCount;
		TArray<FVector> Positions;
		TArray<FVector> Normals;
		Positions.SetNum(VertexCount);
		Normals.SetNumZeroed(VertexCount);
		Positions[FrontCenter] = FVector(0.f, 0.f, OpeningHeight * 0.42f);
		Positions[BackCenter] = FVector(-Depth, 0.f, OpeningHeight * 0.42f);
		for (int32 Step = 0; Step < ProfileCount; ++Step)
		{
			const float Theta = PI * static_cast<float>(Step) / static_cast<float>(ProfileCount - 1);
			const FVector2D Rim = OpeningRim(Theta, OpeningWidth, OpeningHeight);
			Positions[FrontBase + Step] = FVector(0.f, Rim.X, Rim.Y);
			Positions[BackBase + Step] = FVector(-Depth, Rim.X, Rim.Y);
		}
		BuryGround(Positions, BurialCm);

		TArray<int32> Triangles;
		Triangles.Reserve(ProfileCount * 12);
		const FVector Interior(-Depth * 0.5f, 0.f, OpeningHeight * 0.42f);
		for (int32 Step = 0; Step < ProfileCount - 1; ++Step)
		{
			const int32 FrontA = FrontBase + Step;
			const int32 FrontB = FrontA + 1;
			const int32 BackA = BackBase + Step;
			const int32 BackB = BackA + 1;
			AddFace(Positions, Normals, Triangles, FrontCenter, FrontA, FrontB, Interior);
			AddFace(Positions, Normals, Triangles, BackCenter, BackB, BackA, Interior);
			AddFace(Positions, Normals, Triangles, FrontA, BackA, FrontB, Interior);
			AddFace(Positions, Normals, Triangles, BackA, BackB, FrontB, Interior);
		}

		for (FVector& Normal : Normals)
		{
			if (Normal.SizeSquared() < KINDA_SMALL_NUMBER)
			{
				Normal = FVector::ForwardVector;
			}
			else
			{
				Normal.Normalize();
			}
		}

		FMeshDescription MeshDescription;
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();

		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> InstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> InstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> InstanceColors = Attributes.GetVertexInstanceColors();
		TPolygonGroupAttributesRef<FName> PolygonGroupNames = Attributes.GetPolygonGroupMaterialSlotNames();
		InstanceUVs.SetNumChannels(1);

		const FName SlotName(TEXT("Mouth"));
		const FPolygonGroupID PolygonGroup = MeshDescription.CreatePolygonGroup();
		PolygonGroupNames[PolygonGroup] = SlotName;

		TArray<FVertexID> VertexIDs;
		VertexIDs.Reserve(Positions.Num());
		for (const FVector& Position : Positions)
		{
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = FVector3f(Position);
			VertexIDs.Add(VertexID);
		}

		for (int32 Tri = 0; Tri + 2 < Triangles.Num(); Tri += 3)
		{
			TArray<FVertexInstanceID, TInlineAllocator<3>> Corners;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 Vert = Triangles[Tri + Corner];
				const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(VertexIDs[Vert]);
				const FVector Normal = Normals[Vert];
				InstanceNormals[InstanceID] = FVector3f(Normal);
				FVector Tangent = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
				if (Tangent.IsNearlyZero())
				{
					Tangent = FVector::ForwardVector;
				}
				InstanceTangents[InstanceID] = FVector3f(Tangent);
				InstanceBinormalSigns[InstanceID] = 1.f;
				InstanceUVs.Set(InstanceID, 0, FVector2f(0.f, 0.f));
				InstanceColors[InstanceID] = FVector4f(1.f, 1.f, 1.f, 1.f);
				Corners.Add(InstanceID);
			}
			MeshDescription.CreatePolygon(PolygonGroup, Corners);
		}

		UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Transient);
		Mesh->bAllowCPUAccess = true;
		Mesh->NeverStream = true;
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}
		Mesh->SetStaticMaterials({ FStaticMaterial(nullptr, SlotName, SlotName) });

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bFastBuild = false;
		BuildParams.bAllowCpuAccess = true;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = false;

		const TArray<const FMeshDescription*> Descriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(Descriptions, BuildParams))
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonExterior: failed to build the mouth."));
			return nullptr;
		}
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}

		if (!Mesh->GetBodySetup())
		{
			Mesh->CreateBodySetup();
		}
		if (UBodySetup* BodySetup = Mesh->GetBodySetup())
		{
			BodySetup->AggGeom.BoxElems.Empty();
			BodySetup->AggGeom.ConvexElems.Empty();
			BodySetup->AggGeom.SphereElems.Empty();
			BodySetup->AggGeom.SphylElems.Empty();
			BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
			BodySetup->bDoubleSidedGeometry = true;
			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
		}
		return Mesh;
	}
}

ASolidDungeonExterior::ASolidDungeonExterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolidDungeonExterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidDungeonExterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

float ASolidDungeonExterior::GroundDropCm(
	const USolidTerrainMap* Terrain,
	const float SurfaceBias,
	const FVector& ActorLocation)
{
	if (!Terrain || !Terrain->IsBuilt())
	{
		return 0.f;
	}
	const USolidWorldMap* Map = Terrain->GetWorldMap();
	if (!Map || !Map->IsLoaded())
	{
		return 0.f;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	FIntPoint Cell = FIntPoint::ZeroValue;
	if (!Map->CellAtWorld(ActorLocation.X, ActorLocation.Y, WorldMin, WorldMax, Cell))
	{
		return 0.f;
	}

	const float ExtentX = FMath::Max(WorldMax.X - WorldMin.X, 1.f);
	const float ExtentY = FMath::Max(WorldMax.Y - WorldMin.Y, 1.f);
	const float CellW = ExtentX / static_cast<float>(USolidWorldMap::MapWidth);
	const float CellH = ExtentY / static_cast<float>(USolidWorldMap::MapHeight);
	const float XMax = WorldMax.X - static_cast<float>(Cell.X) * CellW;
	const float XMin = XMax - CellW;
	const float YMax = WorldMax.Y - static_cast<float>(Cell.Y) * CellH;
	const float YMin = YMax - CellH;
	const float Spacing = FMath::Max(Terrain->PointSpacing, 1.f);
	const int32 X0 = FMath::Clamp(FMath::FloorToInt((XMin - WorldMin.X) / Spacing), 0, Terrain->GridWidth - 1);
	const int32 X1 = FMath::Clamp(FMath::CeilToInt((XMax - WorldMin.X) / Spacing), 0, Terrain->GridWidth - 1);
	const int32 Y0 = FMath::Clamp(FMath::FloorToInt((YMin - WorldMin.Y) / Spacing), 0, Terrain->GridHeight - 1);
	const int32 Y1 = FMath::Clamp(FMath::CeilToInt((YMax - WorldMin.Y) / Spacing), 0, Terrain->GridHeight - 1);

	float Lowest = TNumericLimits<float>::Max();
	int32 Count = 0;
	for (int32 IndexY = Y0; IndexY <= Y1; ++IndexY)
	{
		for (int32 IndexX = X0; IndexX <= X1; ++IndexX)
		{
			const FSolidTerrainPoint& Point = Terrain->GetPoint(IndexX, IndexY);
			FIntPoint PointCell = FIntPoint::ZeroValue;
			if (!Map->CellAtWorld(Point.X, Point.Y, WorldMin, WorldMax, PointCell) || PointCell != Cell)
			{
				continue;
			}
			Lowest = FMath::Min(Lowest, Point.Height);
			++Count;
		}
	}
	if (Count == 0)
	{
		return 0.f;
	}

	const float Target = Lowest + SurfaceBias - GroundBurialCm;
	return FMath::Max(0.f, ActorLocation.Z - Target);
}

void ASolidDungeonExterior::SetGroundDrop(const float InGroundDropCm)
{
	ChosenDropCm = InGroundDropCm;
}

void ASolidDungeonExterior::SetFace(
	const float InFaceWidthCm,
	const float InFaceHeightCm,
	const float InSlopeRunCm,
	const float InFaceForwardCm)
{
	FaceWidthCm = InFaceWidthCm;
	FaceHeightCm = InFaceHeightCm;
	SlopeRunCm = InSlopeRunCm;
	FaceForwardCm = InFaceForwardCm;
}

void ASolidDungeonExterior::DiscardParts()
{
	auto Drop = [](TObjectPtr<UStaticMeshComponent>& Part)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
			Part = nullptr;
		}
	};
	Drop(RockMesh);
	Drop(MouthMesh);
}

UStaticMeshComponent* ASolidDungeonExterior::AddPart(
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

void ASolidDungeonExterior::BuildVisuals()
{
	DiscardParts();

	if (FaceWidthCm <= 0.f || FaceHeightCm <= 0.f || SlopeRunCm <= 0.f)
	{
		return;
	}

	float Drop = ChosenDropCm;
	if (Drop < 0.f)
	{
		const USolidTerrainMap* Terrain = nullptr;
		float SurfaceBias = 0.f;
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ASolidTerrainStreamer> It(World); It; ++It)
			{
				Terrain = It->GetTerrainMap();
				SurfaceBias = It->CollisionHeightBias;
				break;
			}
		}
		Drop = GroundDropCm(Terrain, SurfaceBias, GetActorLocation());
	}

	UStaticMesh* Rock = SolidDungeonExteriorPrivate::BuildRock(
		this,
		FaceWidthCm,
		FaceHeightCm,
		SlopeRunCm,
		SolidStrongholds::OpeningWidthCm,
		SolidStrongholds::OpeningHeightCm,
		Drop);
	const FVector Face(FaceForwardCm, 0.f, 0.f);
	RockMesh = AddPart(Rock, Face, FVector::OneVector, SolidStrongholds::RockColor(), TEXT("DungeonExterior"));

	const float Depth = SolidStrongholds::FrameThicknessCm;
	UStaticMesh* Mouth = SolidDungeonExteriorPrivate::BuildMouth(
		this,
		SolidStrongholds::OpeningWidthCm,
		SolidStrongholds::OpeningHeightCm,
		Depth,
		Drop);
	MouthMesh = AddPart(
		Mouth,
		Face + FVector(-SolidStrongholds::MouthRecessCm, 0.f, 0.f),
		FVector::OneVector,
		SolidStrongholds::MouthColor(),
		TEXT("DungeonMouth"));
}
