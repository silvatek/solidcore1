#include "SolidDungeonRock.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "SolidStrongholdType.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"

namespace SolidDungeonRockPrivate
{
	constexpr float CubeSizeCm = 100.f;
	constexpr int32 ProfileCount = 33;
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

	/** Opening edge, pulled a little into the stone so the rim is irregular and the hole stays filled. */
	FVector2D OpeningRim(const float Theta, const float OpeningWidth, const float OpeningHeight)
	{
		const float HalfWidth = OpeningWidth * 0.5f;
		const float YDir = FMath::Cos(Theta);
		const float ZDir = FMath::Sin(Theta);
		float AlongY = TNumericLimits<float>::Max();
		if (FMath::Abs(YDir) > KINDA_SMALL_NUMBER)
		{
			AlongY = HalfWidth / FMath::Abs(YDir);
		}
		float AlongZ = TNumericLimits<float>::Max();
		if (ZDir > KINDA_SMALL_NUMBER)
		{
			AlongZ = OpeningHeight / ZDir;
		}
		const float HitT = FMath::Min(AlongY, AlongZ);
		const FVector2D Hit(YDir * HitT, ZDir * HitT);
		const FVector2D Center(0.f, OpeningHeight * 0.5f);
		FVector2D Inward = (Hit - Center).GetSafeNormal();
		if (Inward.IsNearlyZero())
		{
			Inward = FVector2D(0.f, 1.f);
		}
		const FVector2D Tangent(-Inward.Y, Inward.X);
		const float Pull = 8.f * (0.5f + 0.5f * FMath::Sin(Theta * 6.f + 0.4f));
		const float Slide = 6.f * FMath::Sin(Theta * 8.f + 1.1f);
		FVector2D Rim = Hit - Inward * Pull + Tangent * Slide;
		Rim.X = FMath::Clamp(Rim.X, -HalfWidth, HalfWidth);
		Rim.Y = FMath::Clamp(Rim.Y, 0.f, OpeningHeight);
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
		const bool bOutward = FVector::DotProduct(Cross, Positions[A] - Interior) >= 0.f;
		const int32 Second = bOutward ? B : C;
		const int32 Third = bOutward ? C : B;
		Triangles.Add(A);
		Triangles.Add(Second);
		Triangles.Add(Third);
		const FVector Normal = (bOutward ? Cross : -Cross).GetSafeNormal();
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
		const float OpeningHeight)
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
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonRock: failed to build the rock."));
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

ASolidDungeonRock::ASolidDungeonRock()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolidDungeonRock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidDungeonRock::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidDungeonRock::DiscardParts()
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

UStaticMeshComponent* ASolidDungeonRock::AddPart(
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

void ASolidDungeonRock::BuildVisuals()
{
	DiscardParts();

	const SolidStronghold::FSolidDungeon& Dungeon = SolidStronghold::FSolidDungeon::Instance();
	UStaticMesh* Rock = SolidDungeonRockPrivate::BuildRock(
		this,
		Dungeon.FaceWidthCm(),
		Dungeon.FaceHeightCm(),
		Dungeon.SlopeRunCm(),
		SolidStrongholds::OpeningWidthCm,
		SolidStrongholds::OpeningHeightCm);
	RockMesh = AddPart(Rock, FVector::ZeroVector, FVector::OneVector, SolidStrongholds::RockColor(), TEXT("DungeonRock"));

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidDungeonRock: missing Engine BasicShapes Cube."));
		return;
	}

	const float Width = SolidStrongholds::OpeningWidthCm;
	const float Height = SolidStrongholds::OpeningHeightCm;
	const float Depth = SolidStrongholds::FrameThicknessCm;
	const float Recess = SolidStrongholds::MouthRecessCm;
	MouthMesh = AddPart(
		Cube,
		FVector(-Recess - Depth * 0.5f, 0.f, Height * 0.5f),
		FVector(Depth / SolidDungeonRockPrivate::CubeSizeCm, Width / SolidDungeonRockPrivate::CubeSizeCm, Height / SolidDungeonRockPrivate::CubeSizeCm),
		SolidStrongholds::MouthColor(),
		TEXT("DungeonMouth"));
}
