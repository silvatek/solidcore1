#include "SolidBridge.h"

#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidRiver.h"
#include "SolidWorldChart.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"

namespace
{
	struct FRouteLine
	{
		TArray<FVector2D> Points;
	};

	float PolyLength(const FRouteLine& Poly)
	{
		float Length = 0.f;
		for (int32 Index = 1; Index < Poly.Points.Num(); ++Index)
		{
			Length += FVector2D::Distance(Poly.Points[Index - 1], Poly.Points[Index]);
		}
		return Length;
	}

	FVector2D PointAt(const FRouteLine& Poly, const float Distance)
	{
		if (Poly.Points.Num() == 0)
		{
			return FVector2D::ZeroVector;
		}
		if (Poly.Points.Num() == 1 || Distance <= 0.f)
		{
			if (Poly.Points.Num() == 1)
			{
				return Poly.Points[0];
			}
			const FVector2D Back = (Poly.Points[0] - Poly.Points[1]).GetSafeNormal();
			return Poly.Points[0] + Back * (-Distance);
		}

		float Remain = Distance;
		for (int32 Index = 1; Index < Poly.Points.Num(); ++Index)
		{
			const FVector2D Seg = Poly.Points[Index] - Poly.Points[Index - 1];
			const float Len = Seg.Size();
			if (Len <= KINDA_SMALL_NUMBER)
			{
				continue;
			}
			if (Remain <= Len || Index == Poly.Points.Num() - 1)
			{
				const FVector2D Dir = Seg / Len;
				if (Remain <= Len)
				{
					return Poly.Points[Index - 1] + Dir * Remain;
				}
				return Poly.Points[Index] + Dir * (Remain - Len);
			}
			Remain -= Len;
		}
		return Poly.Points.Last();
	}

	bool RoadPoly(const USolidWorldChart& Map, const SolidRoad::FSolidRoad& Road, const FVector2D WorldMin, const FVector2D WorldMax, FRouteLine& Out)
	{
		Out.Points.Reset();
		for (const FIntPoint& Point : Road.Points)
		{
			FVector2D World = FVector2D::ZeroVector;
			if (!Map.TerrainPointToWorld(Point, WorldMin, WorldMax, World))
			{
				continue;
			}
			if (Out.Points.Num() == 0 || !Out.Points.Last().Equals(World, 1.f))
			{
				Out.Points.Add(World);
			}
		}
		return Out.Points.Num() >= 2;
	}

	struct FMeshAccum
	{
		TArray<FVector> TimberPos;
		TArray<FVector> TimberNorm;
		TArray<int32> TimberTris;
		TArray<FVector> RoadPos;
		TArray<FVector> RoadNorm;
		TArray<int32> RoadTris;

		void Add(const bool bRoad, const FVector& A, const FVector& B, const FVector& C)
		{
			FVector Normal = FVector::CrossProduct(B - A, C - A);
			if (Normal.SizeSquared() < 0.01f)
			{
				return;
			}
			// Clockwise from the outside. The right-hand cross points inward, so flip it for lighting.
			Normal = (-Normal).GetSafeNormal();
			TArray<FVector>& Pos = bRoad ? RoadPos : TimberPos;
			TArray<FVector>& Norm = bRoad ? RoadNorm : TimberNorm;
			TArray<int32>& Tris = bRoad ? RoadTris : TimberTris;
			const int32 Base = Pos.Num();
			Pos.Add(A);
			Pos.Add(B);
			Pos.Add(C);
			Norm.Add(Normal);
			Norm.Add(Normal);
			Norm.Add(Normal);
			Tris.Add(Base);
			Tris.Add(Base + 1);
			Tris.Add(Base + 2);
		}

		void Quad(const bool bRoad, const FVector& A, const FVector& B, const FVector& C, const FVector& D)
		{
			Add(bRoad, A, B, C);
			Add(bRoad, A, C, D);
		}
	};

	void SidesAt(const TArray<FVector>& Line, const int32 Index, const float HalfWidth, FVector2D& OutLeft, FVector2D& OutRight)
	{
		FVector2D Dir = FVector2D::ZeroVector;
		if (Index + 1 < Line.Num())
		{
			Dir += FVector2D(Line[Index + 1].X - Line[Index].X, Line[Index + 1].Y - Line[Index].Y).GetSafeNormal();
		}
		if (Index > 0)
		{
			Dir += FVector2D(Line[Index].X - Line[Index - 1].X, Line[Index].Y - Line[Index - 1].Y).GetSafeNormal();
		}
		Dir = Dir.GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			Dir = FVector2D(1.f, 0.f);
		}

		FVector2D Right(Dir.Y, -Dir.X);
		if (Index > 0 && Index + 1 < Line.Num())
		{
			const FVector2D OutDir = FVector2D(Line[Index + 1].X - Line[Index].X, Line[Index + 1].Y - Line[Index].Y).GetSafeNormal();
			const FVector2D OutRight(OutDir.Y, -OutDir.X);
			const float Dot = FVector2D::DotProduct(Right, OutRight);
			if (Dot > 0.25f)
			{
				Right /= Dot;
			}
		}
		Right *= HalfWidth;
		OutRight = Right;
		OutLeft = -Right;
	}

	UStaticMesh* FinishMesh(UObject* Outer, const FMeshAccum& Accum, const TCHAR* Name)
	{
		if (Accum.TimberTris.Num() < 3 && Accum.RoadTris.Num() < 3)
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

		const FName TimberName(TEXT("Timber"));
		const FName RoadName(TEXT("Road"));
		const FPolygonGroupID TimberGroup = MeshDescription.CreatePolygonGroup();
		const FPolygonGroupID RoadGroup = MeshDescription.CreatePolygonGroup();
		PolygonGroupNames[TimberGroup] = TimberName;
		PolygonGroupNames[RoadGroup] = RoadName;

		auto Emit = [&](const FPolygonGroupID Group, const TArray<FVector>& Pos, const TArray<FVector>& Norm, const TArray<int32>& Tris)
		{
			TArray<FVertexID> Ids;
			Ids.Reserve(Pos.Num());
			for (const FVector& Position : Pos)
			{
				const FVertexID VertexID = MeshDescription.CreateVertex();
				VertexPositions[VertexID] = FVector3f(Position);
				Ids.Add(VertexID);
			}
			for (int32 Tri = 0; Tri + 2 < Tris.Num(); Tri += 3)
			{
				TArray<FVertexInstanceID, TInlineAllocator<3>> Corners;
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					const int32 Vert = Tris[Tri + Corner];
					const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(Ids[Vert]);
					const FVector Normal = Norm[Vert];
					InstanceNormals[InstanceID] = FVector3f(Normal);
					FVector Tangent = FVector::CrossProduct(Normal, FVector::UpVector).GetSafeNormal();
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
				MeshDescription.CreatePolygon(Group, Corners);
			}
		};

		Emit(TimberGroup, Accum.TimberPos, Accum.TimberNorm, Accum.TimberTris);
		Emit(RoadGroup, Accum.RoadPos, Accum.RoadNorm, Accum.RoadTris);

		UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Transient);
		Mesh->bAllowCPUAccess = true;
		Mesh->NeverStream = true;
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}
		Mesh->SetStaticMaterials({
			FStaticMaterial(nullptr, TimberName, TimberName),
			FStaticMaterial(nullptr, RoadName, RoadName),
		});

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		// A box around a ramp is a wall. Walk the triangles, the same way terrain does.
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bFastBuild = false;
		BuildParams.bAllowCpuAccess = true;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = false;

		const TArray<const FMeshDescription*> Descriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(Descriptions, BuildParams))
		{
			UE_LOG(LogSolid, Error, TEXT("SolidBridge: failed to build %s mesh."), Name);
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

	/** Strip along a centerline. The deck is capped at both ends. A ramp is always capped at the buried end. */
	UStaticMesh* BuildStrip(UObject* Outer, const TArray<FVector>& WorldLine, const float WidthCm, const FVector& Origin, const bool bCapStart, const bool bCapEnd, const TCHAR* Name)
	{
		if (WorldLine.Num() < 2)
		{
			return nullptr;
		}

		const float Half = WidthCm * 0.5f;
		TArray<FVector> Left;
		TArray<FVector> Right;
		Left.Reserve(WorldLine.Num());
		Right.Reserve(WorldLine.Num());
		for (int32 Index = 0; Index < WorldLine.Num(); ++Index)
		{
			FVector2D SideL;
			FVector2D SideR;
			SidesAt(WorldLine, Index, Half, SideL, SideR);
			const FVector P = WorldLine[Index] - Origin;
			Left.Add(P + FVector(SideL.X, SideL.Y, 0.f));
			Right.Add(P + FVector(SideR.X, SideR.Y, 0.f));
		}

		FMeshAccum Accum;
		for (int32 Index = 1; Index < WorldLine.Num(); ++Index)
		{
			const FVector L0 = Left[Index - 1];
			const FVector R0 = Right[Index - 1];
			const FVector L1 = Left[Index];
			const FVector R1 = Right[Index];
			const FVector Down(0.f, 0.f, -SolidBridge::ThicknessCm);
			const FVector L0b = L0 + Down;
			const FVector R0b = R0 + Down;
			const FVector L1b = L1 + Down;
			const FVector R1b = R1 + Down;

			Accum.Quad(true, L0, L1, R1, R0);
			Accum.Quad(false, L0b, R0b, R1b, L1b);
			Accum.Quad(false, R0, R1, R1b, R0b);
			Accum.Quad(false, L1, L0, L0b, L1b);
		}

		const FVector Down(0.f, 0.f, -SolidBridge::ThicknessCm);
		if (bCapStart)
		{
			Accum.Quad(false, Left[0], Right[0], Right[0] + Down, Left[0] + Down);
		}
		if (bCapEnd)
		{
			const int32 Last = Left.Num() - 1;
			Accum.Quad(false, Left[Last], Left[Last] + Down, Right[Last] + Down, Right[Last]);
		}
		return FinishMesh(Outer, Accum, Name);
	}

	void Paint(UStaticMeshComponent* Mesh, UObject* Outer, const TCHAR* TimberName, const TCHAR* RoadName)
	{
		if (!Mesh || !Mesh->GetStaticMesh())
		{
			return;
		}
		if (UMaterialInterface* Timber = SolidMaterials::CreateSolidColor(Outer, SolidBridge::TimberColor(), TimberName, 0.92f))
		{
			Mesh->SetMaterial(0, Timber);
		}
		if (UMaterialInterface* Road = SolidMaterials::CreateSolidColor(
			Outer, USolidWorldChart::DefaultColorForBiome(ESolidBiome::Road), RoadName, 0.95f))
		{
			Mesh->SetMaterial(1, Road);
		}
	}

	bool IsIglinRelionRoad(const SolidRoad::FSolidRoad& Road)
	{
		auto TownId = [](const FSolidWorldChartMark& Mark) -> int32
		{
			if (Mark.Kind != ESolidWorldChartMarkKind::Town)
			{
				return INDEX_NONE;
			}
			const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Mark.Name);
			return Town != nullptr ? Town->Id : INDEX_NONE;
		};
		const int32 From = TownId(Road.From);
		const int32 To = TownId(Road.To);
		return (From == SolidTowns::Iglin && To == SolidTowns::Relion)
			|| (From == SolidTowns::Relion && To == SolidTowns::Iglin);
	}

	/** One cross-axis value, the middle of the deck's values on that axis. */
	float DeckCrossAxis(const TArray<FVector>& Deck, const bool bEastWest)
	{
		TArray<float> Values;
		Values.Reserve(Deck.Num());
		for (const FVector& Point : Deck)
		{
			Values.Add(bEastWest ? Point.Y : Point.X);
		}
		Values.Sort();
		return Values[Values.Num() / 2];
	}

	/** Drop a later point that sits on the same plan position. */
	void DropStacked(TArray<FVector>& Line)
	{
		if (Line.Num() < 2)
		{
			return;
		}
		int32 Write = 1;
		for (int32 Index = 1; Index < Line.Num(); ++Index)
		{
			const FVector& Prev = Line[Write - 1];
			if (FMath::Abs(Line[Index].X - Prev.X) < 1.f && FMath::Abs(Line[Index].Y - Prev.Y) < 1.f)
			{
				continue;
			}
			Line[Write] = Line[Index];
			++Write;
		}
		Line.SetNum(Write);
	}

	/**
	 * A routed centerline follows the road, and that road may bend onto the crossing.
	 * The deck's longer span is the bridge axis. Every point keeps its place along that
	 * axis. The cross-axis is the road cell at the unshifted deck middle, plus the
	 * half-quad paint shift, so the straight run sits on the painted strip.
	 */
	void Straighten(
		const USolidWorldChart& Map,
		const FVector2D WorldMin,
		const FVector2D WorldMax,
		const FVector& UnshiftedMid,
		SolidBridge::FBridge& Bridge)
	{
		if (Bridge.Deck.Num() < 2)
		{
			return;
		}
		float MinX = Bridge.Deck[0].X;
		float MaxX = MinX;
		float MinY = Bridge.Deck[0].Y;
		float MaxY = MinY;
		for (const FVector& Point : Bridge.Deck)
		{
			MinX = FMath::Min(MinX, Point.X);
			MaxX = FMath::Max(MaxX, Point.X);
			MinY = FMath::Min(MinY, Point.Y);
			MaxY = FMath::Max(MaxY, Point.Y);
		}
		const bool bEastWest = (MaxX - MinX) >= (MaxY - MinY);
		float Lock = DeckCrossAxis(Bridge.Deck, bEastWest);
		FIntPoint Cell = FIntPoint::ZeroValue;
		FVector2D Center = FVector2D::ZeroVector;
		if (Map.CellAtWorld(UnshiftedMid.X, UnshiftedMid.Y, WorldMin, WorldMax, Cell)
			&& Map.CellToWorldXY(Cell, WorldMin, WorldMax, Center))
		{
			const FVector2D Paint = SolidRoad::StripPaintOffset();
			Lock = bEastWest ? Center.Y + Paint.Y : Center.X + Paint.X;
		}
		auto Snap = [bEastWest, Lock](TArray<FVector>& Line)
		{
			for (FVector& Point : Line)
			{
				if (bEastWest)
				{
					Point.Y = Lock;
				}
				else
				{
					Point.X = Lock;
				}
			}
			DropStacked(Line);
		};
		Snap(Bridge.Deck);
		Snap(Bridge.RampFrom);
		Snap(Bridge.RampTo);
	}

	/** Intact center. A broken bridge does not build this mesh. */
	UStaticMesh* BuildIntactDeckMesh(UObject* Outer, const SolidBridge::FBridge& Bridge, const FVector& Origin)
	{
		return BuildStrip(Outer, Bridge.Deck, Bridge.WidthCm, Origin, true, true, TEXT("deck"));
	}

	UStaticMesh* BuildRampMesh(
		UObject* Outer,
		const TArray<FVector>& Centerline,
		const float WidthCm,
		const FVector& Origin,
		const bool bCapLip,
		const TCHAR* Name)
	{
		// The first point meets the deck. Cap it when that deck is missing.
		// The last point is the buried end.
		return BuildStrip(Outer, Centerline, WidthCm, Origin, bCapLip, true, Name);
	}
}

int32 SolidBridge::Collect(
	const USolidWorldChart& Map,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	const TFunction<float(float X, float Y)>& HeightAt,
	TArray<FBridge>& Out)
{
	Out.Reset();
	if (!HeightAt)
	{
		return 0;
	}

	struct FCrossing
	{
		FRouteLine Poly;
		SolidRoad::EType Type = SolidRoad::EType::Trail;
		bool bIglinRelion = false;
	};
	TArray<FCrossing> Crossings;
	for (const SolidRoad::FSolidRoad& Road : Map.GetRoutedRoads())
	{
		FCrossing Crossing;
		if (!RoadPoly(Map, Road, WorldMinXY, WorldMaxXY, Crossing.Poly))
		{
			continue;
		}
		Crossing.Type = Road.Type;
		Crossing.bIglinRelion = IsIglinRelionRoad(Road);
		Crossings.Add(MoveTemp(Crossing));
	}

	for (const FCrossing& Crossing : Crossings)
	{
		const FRouteLine& Poly = Crossing.Poly;

		const float Total = PolyLength(Poly);
		TArray<float> Dist;
		Dist.Add(0.f);
		for (float D = SampleStepCm; D < Total; D += SampleStepCm)
		{
			Dist.Add(D);
		}
		if (Total > 1.f)
		{
			Dist.Add(Total);
		}
		float Along = 0.f;
		for (int32 Index = 1; Index < Poly.Points.Num(); ++Index)
		{
			Along += FVector2D::Distance(Poly.Points[Index - 1], Poly.Points[Index]);
			Dist.Add(Along);
		}
		Dist.Sort();

		TArray<float> Unique;
		Unique.Reserve(Dist.Num());
		for (const float D : Dist)
		{
			if (Unique.Num() == 0 || D - Unique.Last() > 1.f)
			{
				Unique.Add(D);
			}
		}
		Dist = MoveTemp(Unique);

		auto IsWet = [&Map, &Poly, WorldMinXY, WorldMaxXY](const float D)
		{
			const FVector2D P = PointAt(Poly, D);
			return Map.SampleBiome(P.X, P.Y, WorldMinXY, WorldMaxXY) == ESolidBiome::River;
		};

		TArray<uint8> Wet;
		Wet.Reserve(Dist.Num());
		for (const float D : Dist)
		{
			Wet.Add(IsWet(D) ? 1 : 0);
		}

		struct FRun
		{
			int32 First = 0;
			int32 Last = 0;
		};
		TArray<FRun> Runs;
		for (int32 Index = 0; Index < Wet.Num();)
		{
			if (Wet[Index] == 0)
			{
				++Index;
				continue;
			}
			int32 Last = Index;
			while (Last + 1 < Wet.Num() && Wet[Last + 1] != 0)
			{
				++Last;
			}
			if (Runs.Num() > 0 && Dist[Index] - Dist[Runs.Last().Last] < SolidRiver::WidthCm)
			{
				Runs.Last().Last = Last;
			}
			else
			{
				Runs.Add({ Index, Last });
			}
			Index = Last + 1;
		}

		auto RefineWet = [&IsWet](float DryAt, float WetAt)
		{
			for (int32 Refine = 0; Refine < 16; ++Refine)
			{
				const float Mid = (DryAt + WetAt) * 0.5f;
				if (IsWet(Mid))
				{
					WetAt = Mid;
				}
				else
				{
					DryAt = Mid;
				}
			}
			return WetAt;
		};

		auto EdgeBefore = [&IsWet, &RefineWet](const float WetD)
		{
			float WetAt = WetD;
			for (int32 Step = 0; Step < 8000; ++Step)
			{
				const float DryAt = WetAt - SampleStepCm;
				if (!IsWet(DryAt))
				{
					return RefineWet(DryAt, WetAt);
				}
				WetAt = DryAt;
				if (WetD - WetAt > MaxRampCm)
				{
					return WetAt;
				}
			}
			return WetAt;
		};

		auto EdgeAfter = [&IsWet, &RefineWet](const float WetD)
		{
			float WetAt = WetD;
			for (int32 Step = 0; Step < 8000; ++Step)
			{
				const float DryAt = WetAt + SampleStepCm;
				if (!IsWet(DryAt))
				{
					return RefineWet(DryAt, WetAt);
				}
				WetAt = DryAt;
				if (WetAt - WetD > MaxRampCm)
				{
					return WetAt;
				}
			}
			return WetAt;
		};

		const float Width = SolidRoad::WidthCm(Crossing.Type);
		for (const FRun& Run : Runs)
		{
			float Water0 = Dist[Run.First];
			if (Run.First > 0)
			{
				Water0 = RefineWet(Dist[Run.First - 1], Dist[Run.First]);
			}
			else
			{
				Water0 = EdgeBefore(Dist[Run.First]);
			}
			float Water1 = Dist[Run.Last];
			if (Run.Last + 1 < Dist.Num())
			{
				Water1 = RefineWet(Dist[Run.Last + 1], Dist[Run.Last]);
			}
			else
			{
				Water1 = EdgeAfter(Dist[Run.Last]);
			}

			const float Deck0 = Water0 - BankRestCm;
			const float Deck1 = Water1 + BankRestCm;
			if (Deck1 - Deck0 < 10.f)
			{
				continue;
			}

			TArray<float> DeckD;
			DeckD.Add(Deck0);
			float VertD = 0.f;
			for (int32 Index = 1; Index < Poly.Points.Num(); ++Index)
			{
				VertD += FVector2D::Distance(Poly.Points[Index - 1], Poly.Points[Index]);
				if (VertD > Deck0 + 1.f && VertD < Deck1 - 1.f)
				{
					DeckD.Add(VertD);
				}
			}
			DeckD.Add(Deck1);

			float Highest = -TNumericLimits<float>::Max();
			float HighestWater = -TNumericLimits<float>::Max();
			bool bSawWater = false;
			for (float D = Deck0; D <= Deck1 + 0.1f; D += SampleStepCm)
			{
				const float Clamped = FMath::Min(D, Deck1);
				const FVector2D P = PointAt(Poly, Clamped);
				const float H = HeightAt(P.X, P.Y);
				Highest = FMath::Max(Highest, H);
				if (IsWet(Clamped))
				{
					bSawWater = true;
					HighestWater = FMath::Max(HighestWater, H);
				}
			}
			float DeckTop = Highest + RiseCm;
			if (bSawWater)
			{
				DeckTop = FMath::Max(DeckTop, HighestWater + MinClearanceCm + ThicknessCm);
			}

			FBridge Bridge;
			Bridge.WidthCm = Width;
			Bridge.bIglinRelion = Crossing.bIglinRelion;
			Bridge.bBroken = Bridge.bIglinRelion;
			Bridge.Deck.Reserve(DeckD.Num());
			for (const float D : DeckD)
			{
				const FVector2D P = PointAt(Poly, D);
				Bridge.Deck.Add(FVector(P.X, P.Y, DeckTop));
			}

			auto BuildRamp = [&](const float EndD, const float Sign, TArray<FVector>& Ramp)
			{
				float D = EndD;
				float OutDist = 0.f;
				bool bMet = false;
				float MetAt = 0.f;
				auto Push = [&](const float AtD, const float Travel)
				{
					const FVector2D P = PointAt(Poly, AtD);
					Ramp.Add(FVector(P.X, P.Y, DeckTop - Grade * Travel));
				};
				Push(D, 0.f);
				while (OutDist < MaxRampCm)
				{
					OutDist += SampleStepCm;
					D += Sign * SampleStepCm;
					Push(D, OutDist);
					const FVector& Last = Ramp.Last();
					if (!bMet && Last.Z <= HeightAt(Last.X, Last.Y))
					{
						bMet = true;
						MetAt = OutDist;
					}
					if (bMet && OutDist >= MetAt + BurialCm)
					{
						break;
					}
				}
			};

			BuildRamp(Deck0, -1.f, Bridge.RampFrom);
			BuildRamp(Deck1, 1.f, Bridge.RampTo);
			if (Bridge.Deck.Num() >= 2 && Bridge.RampFrom.Num() >= 2 && Bridge.RampTo.Num() >= 2)
			{
				const FVector UnshiftedMid = (Bridge.Deck[0] + Bridge.Deck.Last()) * 0.5f;
				const FVector2D Paint = SolidRoad::StripPaintOffset();
				auto Shift = [Paint](TArray<FVector>& Line)
				{
					for (FVector& Point : Line)
					{
						Point.X += Paint.X;
						Point.Y += Paint.Y;
					}
				};
				Shift(Bridge.Deck);
				Shift(Bridge.RampFrom);
				Shift(Bridge.RampTo);
				Straighten(Map, WorldMinXY, WorldMaxXY, UnshiftedMid, Bridge);
				if (Bridge.Deck.Num() >= 2 && Bridge.RampFrom.Num() >= 2 && Bridge.RampTo.Num() >= 2)
				{
					Out.Add(MoveTemp(Bridge));
				}
			}
		}
	}
	return Out.Num();
}

ASolidBridge::ASolidBridge()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	RampFromMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RampFrom"));
	RampFromMesh->SetupAttachment(SceneRoot);
	RampFromMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RampFromMesh->SetCollisionResponseToAllChannels(ECR_Block);
	RampFromMesh->SetCastShadow(true);

	DeckMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Deck"));
	DeckMesh->SetupAttachment(SceneRoot);
	DeckMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DeckMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DeckMesh->SetCastShadow(true);

	RampToMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RampTo"));
	RampToMesh->SetupAttachment(SceneRoot);
	RampToMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RampToMesh->SetCollisionResponseToAllChannels(ECR_Block);
	RampToMesh->SetCastShadow(true);
}

void ASolidBridge::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bHasBridge)
	{
		BuildVisuals();
	}
}

void ASolidBridge::BeginPlay()
{
	Super::BeginPlay();
	if (bHasBridge)
	{
		BuildVisuals();
	}
}

void ASolidBridge::SetBridge(const SolidBridge::FBridge& InBridge)
{
	Bridge = InBridge;
	bHasBridge = Bridge.Deck.Num() >= 2;
	if (!bHasBridge)
	{
		return;
	}

	FVector Mid = FVector::ZeroVector;
	for (const FVector& P : Bridge.Deck)
	{
		Mid.X += P.X;
		Mid.Y += P.Y;
	}
	Mid /= static_cast<float>(Bridge.Deck.Num());
	SetActorLocation(FVector(Mid.X, Mid.Y, 0.f));
	BuildVisuals();
}

void ASolidBridge::SetBroken(const bool bInBroken)
{
	if (!bHasBridge || Bridge.bBroken == bInBroken)
	{
		return;
	}
	Bridge.bBroken = bInBroken;
	BuildVisuals();
}

void ASolidBridge::BuildVisuals()
{
	if (!bHasBridge || !DeckMesh || !RampFromMesh || !RampToMesh)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	if (Bridge.bBroken)
	{
		DeckMesh->SetStaticMesh(nullptr);
		DeckMesh->SetVisibility(false);
		DeckMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else if (UStaticMesh* Deck = BuildIntactDeckMesh(this, Bridge, Origin))
	{
		DeckMesh->SetStaticMesh(Deck);
		DeckMesh->SetVisibility(true);
		DeckMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Paint(DeckMesh, this, TEXT("BridgeDeckTimber"), TEXT("BridgeDeckRoad"));
	}
	if (UStaticMesh* Ramp = BuildRampMesh(this, Bridge.RampFrom, Bridge.WidthCm, Origin, Bridge.bBroken, TEXT("ramp from")))
	{
		RampFromMesh->SetStaticMesh(Ramp);
		Paint(RampFromMesh, this, TEXT("BridgeRampFromTimber"), TEXT("BridgeRampFromRoad"));
	}
	if (UStaticMesh* Ramp = BuildRampMesh(this, Bridge.RampTo, Bridge.WidthCm, Origin, Bridge.bBroken, TEXT("ramp to")))
	{
		RampToMesh->SetStaticMesh(Ramp);
		Paint(RampToMesh, this, TEXT("BridgeRampToTimber"), TEXT("BridgeRampToRoad"));
	}
}
