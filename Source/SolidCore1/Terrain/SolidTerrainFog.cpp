#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidCore1.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#if WITH_EDITOR
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#endif

namespace SolidTerrainFog
{
	float FogFromDistanceMeters(float DistM)
	{
		if (DistM > FullFogStartMeters)
		{
			return 1.f;
		}
		if (DistM >= HalfFogStartMeters)
		{
			return 0.5f;
		}
		return 0.f;
	}

	static UMaterialInterface* CreateSolidColorMaterial(
		UObject* Outer,
		const FLinearColor& Color,
		const TCHAR* DebugName)
	{
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
		if (!Parent)
		{
			Parent = LoadObject<UMaterialInterface>(
				nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_DefaultColorway.MI_DefaultColorway"));
		}
		if (!Parent)
		{
			UE_LOG(LogSolid, Error, TEXT("Fog solid material %s: no FlatCol parent."), DebugName);
			return nullptr;
		}

		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
		if (!MID)
		{
			return Parent;
		}

		MID->SetVectorParameterValue(TEXT("Base Color"), Color);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
		MID->SetScalarParameterValue(TEXT("Roughness"), 1.f);
		UE_LOG(LogSolid, Warning, TEXT("Fog solid material %s from %s"), DebugName, *Parent->GetName());
		return MID;
	}

#if WITH_EDITOR
	/** Real translucent mist — BlendMode on a new UMaterial (MIDs cannot change blend mode). */
	static UMaterialInterface* CreateProgrammaticTranslucentMist(
		UObject* Outer,
		const FLinearColor& Color,
		float Opacity,
		const TCHAR* DebugName)
	{
		Opacity = FMath::Clamp(Opacity, 0.f, 1.f);

		UMaterial* Material = NewObject<UMaterial>(Outer, NAME_None, RF_Transient);
		if (!Material)
		{
			return nullptr;
		}

		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->TwoSided = true;
		Material->TranslucencyLightingMode = TLM_VolumetricNonDirectional;
		Material->bScreenSpaceReflections = false;
		Material->SetShadingModel(MSM_Unlit);

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			UE_LOG(LogSolid, Error, TEXT("Fog %s: GetEditorOnlyData() null — cannot build translucent mat."), DebugName);
			return nullptr;
		}

		UMaterialExpressionVectorParameter* ColorParam =
			NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParam->ParameterName = TEXT("MistColor");
		ColorParam->DefaultValue = Color;
		ColorParam->MaterialExpressionEditorX = -380;
		ColorParam->MaterialExpressionEditorY = 0;

		UMaterialExpressionScalarParameter* OpacityParam =
			NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParam->ParameterName = TEXT("Opacity");
		OpacityParam->DefaultValue = Opacity;
		OpacityParam->MaterialExpressionEditorX = -380;
		OpacityParam->MaterialExpressionEditorY = 160;

		EditorData->ExpressionCollection.Expressions.Add(ColorParam);
		EditorData->ExpressionCollection.Expressions.Add(OpacityParam);
		EditorData->EmissiveColor.Expression = ColorParam;
		EditorData->Opacity.Expression = OpacityParam;

		bool bNeedsRecompile = false;
		Material->SetMaterialUsage(bNeedsRecompile, MATUSAGE_StaticMesh);

		Material->PreEditChange(nullptr);
		Material->PostEditChange();

		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, Outer);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("MistColor"), Color);
			MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
			UE_LOG(LogSolid, Warning,
				TEXT("Fog translucent material %s (programmatic, opacity=%.2f)"), DebugName, Opacity);
			return MID;
		}

		UE_LOG(LogSolid, Warning,
			TEXT("Fog translucent material %s (programmatic parent, opacity=%.2f)"), DebugName, Opacity);
		return Material;
	}
#endif // WITH_EDITOR

	UMaterialInterface* CreateVolumeMaterial(
		UObject* Outer,
		const FLinearColor& Color,
		float Opacity,
		const TCHAR* DebugName)
	{
		Opacity = FMath::Clamp(Opacity, 0.f, 1.f);

#if WITH_EDITOR
		if (UMaterialInterface* Programmatic = CreateProgrammaticTranslucentMist(Outer, Color, Opacity, DebugName))
		{
			return Programmatic;
		}
#endif

		// Glow parents often ignore Opacity and read as solid white panels — last resort only.
		UE_LOG(LogSolid, Error,
			TEXT("Fog %s: programmatic translucent failed; FlatCol fallback is OPAQUE (opacity=%.2f unused)."),
			DebugName, Opacity);
		return CreateSolidColorMaterial(Outer, Color, DebugName);
	}

	UMaterialInterface* CreateHalfMaterial(UObject* Outer)
	{
		return CreateVolumeMaterial(
			Outer, FLinearColor(0.55f, 0.62f, 0.68f), 0.40f, TEXT("ExplorationFogHalf"));
	}

	UMaterialInterface* CreateFullMaterial(UObject* Outer)
	{
		return CreateVolumeMaterial(
			Outer, FLinearColor(0.48f, 0.54f, 0.60f), 0.70f, TEXT("ExplorationFogFull"));
	}

	UMaterialInterface* CreateOuterMaterial(UObject* Outer)
	{
		return CreateVolumeMaterial(
			Outer, FLinearColor(0.92f, 0.94f, 0.96f), 0.82f, TEXT("ExplorationFogOuter"));
	}

	void ConfigureOverlayComponent(UStaticMeshComponent* Mesh)
	{
		if (!Mesh)
		{
			return;
		}

		// Fog must never block pawn/camera traces (spring arm ProbeChannel = ECC_Camera).
		Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Mesh->SetCollisionObjectType(ECC_WorldDynamic);
		Mesh->BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->BodyInstance.SetResponseToAllChannels(ECR_Ignore);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetNotifyRigidBodyCollision(false);
		Mesh->CanCharacterStepUpOn = ECB_No;
		Mesh->SetCastShadow(false);
		Mesh->SetVisibility(true);
		Mesh->SetHiddenInGame(false);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->bUseAsOccluder = false;
		Mesh->SetCanEverAffectNavigation(false);
	}

	static UStaticMesh* BuildRuntimeFogStaticMesh(
		UObject* Outer,
		const TArray<FVector>& Positions,
		const TArray<FVector>& Normals,
		const TArray<FVector>& Tangents,
		const TArray<FVector2D>& UVs,
		const TArray<FLinearColor>& Colors,
		const TArray<int32>& Triangles,
		const TArray<UMaterialInterface*>& Materials,
		const TArray<int32>& TriMaterialIndices)
	{
		if (Positions.Num() == 0 || Triangles.Num() < 3 || Materials.Num() == 0)
		{
			return nullptr;
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

		TArray<FPolygonGroupID> PolygonGroups;
		PolygonGroups.Reserve(Materials.Num());
		for (int32 MatIndex = 0; MatIndex < Materials.Num(); ++MatIndex)
		{
			const FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
			PolygonGroupNames[PolygonGroupID] = FName(*FString::Printf(TEXT("Slot%d"), MatIndex));
			PolygonGroups.Add(PolygonGroupID);
		}

		TArray<FVertexID> VertexIDs;
		VertexIDs.Reserve(Positions.Num());
		for (const FVector& Position : Positions)
		{
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = FVector3f(Position);
			VertexIDs.Add(VertexID);
		}

		const int32 TriCount = Triangles.Num() / 3;
		for (int32 TriIndex = 0; TriIndex < TriCount; ++TriIndex)
		{
			const int32 I0 = Triangles[TriIndex * 3 + 0];
			const int32 I1 = Triangles[TriIndex * 3 + 1];
			const int32 I2 = Triangles[TriIndex * 3 + 2];
			const int32 MatIndex = TriMaterialIndices.IsValidIndex(TriIndex)
				? FMath::Clamp(TriMaterialIndices[TriIndex], 0, PolygonGroups.Num() - 1)
				: 0;

			TArray<FVertexInstanceID, TInlineAllocator<3>> InstanceIDs;
			const int32 CornerIndices[3] = { I0, I1, I2 };
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 VertIndex = CornerIndices[Corner];
				const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(VertexIDs[VertIndex]);
				InstanceNormals[InstanceID] = FVector3f(Normals[VertIndex]);
				InstanceTangents[InstanceID] = FVector3f(Tangents[VertIndex]);
				InstanceBinormalSigns[InstanceID] = 1.f;
				InstanceUVs.Set(InstanceID, 0, FVector2f(UVs.IsValidIndex(VertIndex) ? UVs[VertIndex] : FVector2D::ZeroVector));
				InstanceColors[InstanceID] = FVector4f(
					Colors.IsValidIndex(VertIndex) ? Colors[VertIndex] : FLinearColor::White);
				InstanceIDs.Add(InstanceID);
			}

			MeshDescription.CreatePolygon(PolygonGroups[MatIndex], InstanceIDs);
		}

		UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Transient);
		Mesh->bAllowCPUAccess = true;
		Mesh->NeverStream = true;
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}

		TArray<FStaticMaterial> StaticMaterials;
		StaticMaterials.Reserve(Materials.Num());
		for (int32 MatIndex = 0; MatIndex < Materials.Num(); ++MatIndex)
		{
			const FName SlotName(*FString::Printf(TEXT("Slot%d"), MatIndex));
			StaticMaterials.Add(FStaticMaterial(Materials[MatIndex], SlotName, SlotName));
		}
		Mesh->SetStaticMaterials(StaticMaterials);

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bFastBuild = true;
		BuildParams.bAllowCpuAccess = false;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = false;

		const TArray<const FMeshDescription*> Descriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(Descriptions, BuildParams))
		{
			return nullptr;
		}

		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}

		return Mesh;
	}

	UStaticMesh* BuildChunkFogMesh(
		UObject* Outer,
		const USolidTerrainMap* TerrainMap,
		const FMeshBuildParams& Params,
		UMaterialInterface* HalfMaterial,
		UMaterialInterface* FullMaterial,
		UMaterialInterface* OuterMaterial)
	{
		const bool bHaveHalf = HalfMaterial != nullptr;
		const bool bHaveFull = FullMaterial != nullptr;
		const bool bHaveOuter = OuterMaterial != nullptr;
		if (!Outer || (!bHaveHalf && !bHaveFull && !bHaveOuter))
		{
			return nullptr;
		}

		const float ChunkSize = FMath::Max(Params.ChunkWorldSize, 100.f);
		const int32 FogQuads = FMath::Clamp(Params.FogQuadsPerSide, 1, 64);
		const int32 FogVerts = FogQuads + 1;
		const float Step = ChunkSize / static_cast<float>(FogQuads);
		const float OriginX = static_cast<float>(Params.ChunkCoord.X) * ChunkSize;
		const float OriginY = static_cast<float>(Params.ChunkCoord.Y) * ChunkSize;
		const float HeightFull = FMath::Max(Params.VolumeHeightCm, 200.f);
		const float HeightHalf = FMath::Max(Params.VolumeHeightHalfCm, 200.f);

		TArray<float> SurfaceZ;
		TArray<float> FogAmounts;
		SurfaceZ.SetNumUninitialized(FogVerts * FogVerts);
		FogAmounts.SetNumUninitialized(FogVerts * FogVerts);
		for (int32 Y = 0; Y < FogVerts; ++Y)
		{
			for (int32 X = 0; X < FogVerts; ++X)
			{
				const float WorldX = OriginX + static_cast<float>(X) * Step;
				const float WorldY = OriginY + static_cast<float>(Y) * Step;
				const float Height = (TerrainMap && TerrainMap->IsBuilt())
					? TerrainMap->SampleHeight(WorldX, WorldY)
					: 0.f;
				SurfaceZ[Y * FogVerts + X] = Height + Params.CollisionHeightBias;
				FogAmounts[Y * FogVerts + X] = (TerrainMap && TerrainMap->IsBuilt())
					? FMath::Clamp(TerrainMap->SamplePoint(WorldX, WorldY).Fog, 0.f, 1.f)
					: 1.f;
			}
		}

		TArray<FVector> Positions;
		TArray<FVector> Normals;
		TArray<FVector> Tangents;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<int32> Triangles;
		TArray<int32> TriMaterials;

		// Two contours: clear|fogged (~25m) and half|full (~50m).
		const int32 EdgeBudget = FogQuads * FogQuads * 4;
		Positions.Reserve(EdgeBudget * 4);
		Normals.Reserve(EdgeBudget * 4);
		Tangents.Reserve(EdgeBudget * 4);
		UVs.Reserve(EdgeBudget * 4);
		Colors.Reserve(EdgeBudget * 4);
		Triangles.Reserve(EdgeBudget * 6);
		TriMaterials.Reserve(EdgeBudget * 2);

		// Material slots: 0=half, 1=full, 2=outer (white half→full curtain).
		int32 SlotHalf = INDEX_NONE;
		int32 SlotFull = INDEX_NONE;
		int32 SlotOuter = INDEX_NONE;
		TArray<UMaterialInterface*> Materials;
		if (bHaveHalf)
		{
			SlotHalf = Materials.Add(HalfMaterial);
		}
		if (bHaveFull)
		{
			SlotFull = Materials.Add(FullMaterial);
		}
		if (bHaveOuter)
		{
			SlotOuter = Materials.Add(OuterMaterial);
		}
		else if (SlotFull != INDEX_NONE)
		{
			SlotOuter = SlotFull;
		}
		else
		{
			SlotOuter = SlotHalf;
		}
		if (SlotHalf == INDEX_NONE)
		{
			SlotHalf = (SlotFull != INDEX_NONE) ? SlotFull : SlotOuter;
		}
		if (SlotFull == INDEX_NONE)
		{
			SlotFull = (SlotOuter != INDEX_NONE) ? SlotOuter : SlotHalf;
		}

		auto AppendVert = [&](const FVector& Pos, const FVector& Normal) -> int32
		{
			const int32 Index = Positions.Num();
			Positions.Add(Pos);
			Normals.Add(Normal);
			FVector Tangent = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
			if (Tangent.IsNearlyZero())
			{
				Tangent = FVector::RightVector;
			}
			Tangents.Add(Tangent);
			UVs.Add(FVector2D(Pos.X * 0.01f, Pos.Y * 0.01f));
			Colors.Add(FLinearColor::White);
			return Index;
		};

		auto AppendQuad = [&](int32 I0, int32 I1, int32 I2, int32 I3, int32 Slot)
		{
			Triangles.Add(I0);
			Triangles.Add(I1);
			Triangles.Add(I2);
			Triangles.Add(I0);
			Triangles.Add(I2);
			Triangles.Add(I3);
			TriMaterials.Add(Slot);
			TriMaterials.Add(Slot);
		};

		struct FContourPoint
		{
			float X = 0.f;
			float Y = 0.f;
			float Z = 0.f;
		};

		auto CrossingOnEdge = [&](int32 IX0, int32 IY0, int32 IX1, int32 IY1, float Iso) -> FContourPoint
		{
			const int32 I0 = IY0 * FogVerts + IX0;
			const int32 I1 = IY1 * FogVerts + IX1;
			const float F0 = FogAmounts[I0];
			const float F1 = FogAmounts[I1];
			float T = 0.5f;
			if (FMath::Abs(F1 - F0) > KINDA_SMALL_NUMBER)
			{
				T = FMath::Clamp((Iso - F0) / (F1 - F0), 0.f, 1.f);
			}
			FContourPoint P;
			P.X = FMath::Lerp(static_cast<float>(IX0) * Step, static_cast<float>(IX1) * Step, T);
			P.Y = FMath::Lerp(static_cast<float>(IY0) * Step, static_cast<float>(IY1) * Step, T);
			P.Z = FMath::Lerp(SurfaceZ[I0], SurfaceZ[I1], T);
			return P;
		};

		auto AppendContourWall = [&](
			const FContourPoint& A, const FContourPoint& B, int32 Slot, float VolumeHeight)
		{
			FVector2D Dir2(B.X - A.X, B.Y - A.Y);
			const float Len = Dir2.Size();
			if (Len < 1.f || Slot < 0)
			{
				return;
			}
			Dir2 /= Len;

			const float Pad = FMath::Min(Step * 0.08f, Len * 0.25f);
			const float AX = A.X - Dir2.X * Pad;
			const float AY = A.Y - Dir2.Y * Pad;
			const float BX = B.X + Dir2.X * Pad;
			const float BY = B.Y + Dir2.Y * Pad;
			constexpr float SkirtCm = 20.f;
			const FVector Normal(-Dir2.Y, Dir2.X, 0.f);

			const int32 V0 = AppendVert(FVector(AX, AY, A.Z + SkirtCm), Normal);
			const int32 V1 = AppendVert(FVector(BX, BY, B.Z + SkirtCm), Normal);
			const int32 V2 = AppendVert(FVector(BX, BY, B.Z + VolumeHeight), Normal);
			const int32 V3 = AppendVert(FVector(AX, AY, A.Z + VolumeHeight), Normal);
			AppendQuad(V0, V1, V2, V3, Slot);
		};

		static const int8 MSSegments[16][4] = {
			{-1, -1, -1, -1},
			{3, 0, -1, -1},
			{0, 1, -1, -1},
			{3, 1, -1, -1},
			{1, 2, -1, -1},
			{3, 0, 1, 2},
			{0, 2, -1, -1},
			{3, 2, -1, -1},
			{2, 3, -1, -1},
			{0, 2, -1, -1},
			{0, 1, 2, 3},
			{1, 2, -1, -1},
			{1, 3, -1, -1},
			{0, 1, -1, -1},
			{0, 3, -1, -1},
			{-1, -1, -1, -1},
		};

		auto EmitIsocontour = [&](float Iso, auto&& InsidePred, auto&& SlotForCell, float VolumeHeight)
		{
			for (int32 Y = 0; Y < FogQuads; ++Y)
			{
				for (int32 X = 0; X < FogQuads; ++X)
				{
					const int32 I00 = Y * FogVerts + X;
					const int32 I10 = I00 + 1;
					const int32 I01 = I00 + FogVerts;
					const int32 I11 = I01 + 1;

					const float F00 = FogAmounts[I00];
					const float F10 = FogAmounts[I10];
					const float F01 = FogAmounts[I01];
					const float F11 = FogAmounts[I11];

					const uint8 Case =
						(InsidePred(F00) ? 1u : 0u) |
						(InsidePred(F10) ? 2u : 0u) |
						(InsidePred(F11) ? 4u : 0u) |
						(InsidePred(F01) ? 8u : 0u);

					if (Case == 0 || Case == 15)
					{
						continue;
					}

					FContourPoint EdgePt[4];
					EdgePt[0] = CrossingOnEdge(X, Y, X + 1, Y, Iso);
					EdgePt[1] = CrossingOnEdge(X + 1, Y, X + 1, Y + 1, Iso);
					EdgePt[2] = CrossingOnEdge(X + 1, Y + 1, X, Y + 1, Iso);
					EdgePt[3] = CrossingOnEdge(X, Y + 1, X, Y, Iso);

					const int32 Slot = SlotForCell(F00, F10, F01, F11);
					const int8* Seg = MSSegments[Case];
					for (int32 S = 0; S < 4 && Seg[S] >= 0; S += 2)
					{
						const int32 E0 = Seg[S];
						const int32 E1 = Seg[S + 1];
						if (E0 < 0 || E1 < 0)
						{
							break;
						}
						AppendContourWall(EdgePt[E0], EdgePt[E1], Slot, VolumeHeight);
					}
				}
			}
		};

		// Inner ring: clear (fog≈0) | any fog.
		EmitIsocontour(
			ClearFogEpsilon,
			[](float F) { return F > ClearFogEpsilon; },
			[&](float F00, float F10, float F01, float F11) -> int32
			{
				float FoggedAmount = 0.f;
				if (F00 > ClearFogEpsilon) { FoggedAmount = FMath::Max(FoggedAmount, F00); }
				if (F10 > ClearFogEpsilon) { FoggedAmount = FMath::Max(FoggedAmount, F10); }
				if (F01 > ClearFogEpsilon) { FoggedAmount = FMath::Max(FoggedAmount, F01); }
				if (F11 > ClearFogEpsilon) { FoggedAmount = FMath::Max(FoggedAmount, F11); }
				return (FoggedAmount >= HalfFullFogThreshold) ? SlotFull : SlotHalf;
			},
			HeightHalf);

		// Outer ring: half fog | full fog (white curtain ~50m).
		EmitIsocontour(
			HalfFullFogThreshold,
			[](float F) { return F >= HalfFullFogThreshold; },
			[&](float, float, float, float) -> int32 { return SlotOuter; },
			HeightFull);

		return BuildRuntimeFogStaticMesh(
			Outer, Positions, Normals, Tangents, UVs, Colors, Triangles, Materials, TriMaterials);
	}

	float SampleMistAmountAround(const USolidTerrainMap* Map, const FVector& WorldLocation)
	{
		if (!Map || !Map->IsBuilt())
		{
			return 0.f;
		}

		float Amount = FMath::Clamp(Map->SamplePoint(WorldLocation.X, WorldLocation.Y).Fog, 0.f, 1.f);
		const float ClearCm = MetersToCm(ClearRadiusMeters);
		const float FullCm = MetersToCm(FullFogStartMeters);
		constexpr int32 NumDirs = 8;
		for (int32 DirIndex = 0; DirIndex < NumDirs; ++DirIndex)
		{
			const float Angle = (2.f * PI) * (static_cast<float>(DirIndex) / static_cast<float>(NumDirs));
			const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
			const FVector AtClear = WorldLocation + FVector(Dir.X * ClearCm, Dir.Y * ClearCm, 0.f);
			const FVector AtFull = WorldLocation + FVector(Dir.X * FullCm, Dir.Y * FullCm, 0.f);
			Amount = FMath::Max(Amount, Map->SamplePoint(AtClear.X, AtClear.Y).Fog);
			Amount = FMath::Max(Amount, Map->SamplePoint(AtFull.X, AtFull.Y).Fog);
		}
		return FMath::Clamp(Amount, 0.f, 1.f);
	}

	void SilenceHeightFog(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
		{
			AExponentialHeightFog* FogActor = *It;
			if (!FogActor)
			{
				continue;
			}
			if (UExponentialHeightFogComponent* FogComp = FogActor->GetComponent())
			{
				FogComp->SetFogDensity(0.f);
				FogComp->SetFogMaxOpacity(0.f);
				FogComp->SetStartDistance(0.f);
				FogComp->SetVolumetricFog(false);
				FogComp->VolumetricFogExtinctionScale = 0.f;
				FogComp->MarkRenderStateDirty();
			}
		}
	}

	AExponentialHeightFog* EnsureHeightFog(UWorld* World, AActor* Owner, const FLinearColor& MistColor)
	{
		if (!World)
		{
			return nullptr;
		}

		AExponentialHeightFog* HeightFogActor = nullptr;
		for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
		{
			HeightFogActor = *It;
			break;
		}

		if (!HeightFogActor)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Owner;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			HeightFogActor = World->SpawnActor<AExponentialHeightFog>(
				AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (HeightFogActor)
			{
				UE_LOG(LogSolid, Warning, TEXT("Spawned ExponentialHeightFog for TerrainPoint mist."));
			}
		}

		if (UExponentialHeightFogComponent* FogComp = HeightFogActor ? HeightFogActor->GetComponent() : nullptr)
		{
			FogComp->SetVisibility(true);
			FogComp->SetVolumetricFog(true);
			FogComp->VolumetricFogScatteringDistribution = 0.3f;
			FogComp->VolumetricFogExtinctionScale = 0.8f;
			FogComp->FogHeightFalloff = 0.02f;
			FogComp->SetFogInscatteringColor(MistColor);
			FogComp->SetVolumetricFogDistance(20000.f);
		}

		return HeightFogActor;
	}

	void ApplyHeightFogAmount(AExponentialHeightFog* FogActor, float Amount, const FHeightFogStyle& Style)
	{
		if (!FogActor)
		{
			return;
		}

		UExponentialHeightFogComponent* FogComp = FogActor->GetComponent();
		if (!FogComp)
		{
			return;
		}

		Amount = FMath::Clamp(Amount, 0.f, 1.f);

		float Density = 0.f;
		float MaxOpacity = 0.f;
		float StartDistance = 0.f;
		float ExtinctionScale = 0.f;

		if (Amount <= KINDA_SMALL_NUMBER)
		{
			Density = 0.f;
			MaxOpacity = 0.f;
			StartDistance = 0.f;
			ExtinctionScale = 0.f;
		}
		else if (Amount <= 0.5f)
		{
			const float T = Amount / 0.5f;
			Density = FMath::Lerp(0.f, Style.DensityAtHalf, T);
			MaxOpacity = FMath::Lerp(0.f, Style.MaxOpacityAtHalf, T);
			StartDistance = FMath::Lerp(800.f, 100.f, T);
			ExtinctionScale = FMath::Lerp(0.2f, 1.2f, T);
		}
		else
		{
			const float T = (Amount - 0.5f) / 0.5f;
			Density = FMath::Lerp(Style.DensityAtHalf, Style.DensityAtFull, T);
			MaxOpacity = FMath::Lerp(Style.MaxOpacityAtHalf, Style.MaxOpacityAtFull, T);
			StartDistance = FMath::Lerp(100.f, 20.f, T);
			ExtinctionScale = FMath::Lerp(1.2f, 2.2f, T);
		}

		FogComp->SetFogDensity(Density);
		FogComp->SetFogMaxOpacity(MaxOpacity);
		FogComp->SetFogInscatteringColor(Style.MistColor);
		FogComp->SetStartDistance(StartDistance);
		FogComp->SetVolumetricFog(Amount > KINDA_SMALL_NUMBER);
		FogComp->VolumetricFogExtinctionScale = ExtinctionScale;
		FogComp->MarkRenderStateDirty();
	}
}
