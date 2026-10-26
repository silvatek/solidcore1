#include "SolidArrange.h"

namespace
{
	const SolidStrongholdFurniture::FUsed& EmptyPiece()
	{
		static const SolidStrongholdFurniture::FUsed Empty;
		return Empty;
	}
}

bool FSolidArrange::Toggle(const FContext& Context)
{
	if (Context.bOverlay || !Context.bInStronghold)
	{
		return false;
	}
	if (bOn)
	{
		PutBack();
		bOn = false;
		ClearHover();
		return true;
	}
	bOn = true;
	return true;
}

void FSolidArrange::Leave()
{
	PutBack();
	bOn = false;
	ClearHover();
}

void FSolidArrange::Open(
	const TArray<SolidStrongholdFurniture::FUsed>& InPieces,
	const TArray<SolidStrongholdGrid::FHex>& InCells,
	const SolidStrongholdGrid::FCell& InEntrance,
	const bool bInHasEntrance,
	const SolidStrongholdGrid::FCell& InCaptain,
	const bool bInHasCaptain,
	const float InFloorZ)
{
	PutBack();
	LastPlaced = INDEX_NONE;
	Pieces = InPieces;
	Cells = InCells;
	Entrance = InEntrance;
	bHasEntrance = bInHasEntrance;
	Captain = InCaptain;
	bHasCaptain = bInHasCaptain;
	FloorHeight = InFloorZ;
	Held = INDEX_NONE;
	ClearHover();
}

void FSolidArrange::ClearHover()
{
	bHover = false;
}

void FSolidArrange::SetHover(const int32 X, const int32 Y)
{
	if (!bOn || !SolidStrongholdFurniture::FindCell(Cells, X, Y))
	{
		ClearHover();
		return;
	}
	bHover = true;
	HoverX = X;
	HoverY = Y;
	if (Held != INDEX_NONE)
	{
		GhostUsed.X = X;
		GhostUsed.Y = Y;
	}
}

const TCHAR* FSolidArrange::HoverName() const
{
	if (!bOn || !bHover)
	{
		return TEXT("");
	}
	const int32 Placed = PieceOnCell(HoverX, HoverY);
	if (Placed != INDEX_NONE)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Pieces[Placed].ItemIndex);
		return Item ? Item->Name : TEXT("");
	}
	if (Held != INDEX_NONE && Covers(GhostUsed, HoverX, HoverY))
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(GhostUsed.ItemIndex);
		return Item ? Item->Name : TEXT("");
	}
	return TEXT("");
}

void FSolidArrange::HighlightHexes(TArray<SolidStrongholdGrid::FHex>& Out) const
{
	Out.Reset();
	if (!bOn || !bHover)
	{
		return;
	}

	auto Add = [this, &Out](const int32 X, const int32 Y)
	{
		const SolidStrongholdGrid::FHex* Hex = SolidStrongholdFurniture::FindCell(Cells, X, Y);
		if (!Hex)
		{
			return;
		}
		for (const SolidStrongholdGrid::FHex& Have : Out)
		{
			if (Have.Col == X && Have.Row == Y)
			{
				return;
			}
		}
		Out.Add(*Hex);
	};

	if (Held != INDEX_NONE)
	{
		Add(GhostUsed.X, GhostUsed.Y);
		if (const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(Cells, GhostUsed))
		{
			Add(Span->Col, Span->Row);
		}
		return;
	}

	const int32 Placed = PieceOnCell(HoverX, HoverY);
	if (Placed != INDEX_NONE)
	{
		Add(Pieces[Placed].X, Pieces[Placed].Y);
		if (const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(Cells, Pieces[Placed]))
		{
			Add(Span->Col, Span->Row);
		}
		return;
	}
	Add(HoverX, HoverY);
}

FSolidArrange::EConfirm FSolidArrange::Confirm()
{
	LastPlaced = INDEX_NONE;
	if (!bOn || !bHover)
	{
		return EConfirm::None;
	}
	if (Held != INDEX_NONE)
	{
		if (!CanPlace(GhostUsed))
		{
			return EConfirm::Refused;
		}
		Pieces[Held] = GhostUsed;
		LastPlaced = Held;
		Held = INDEX_NONE;
		return EConfirm::Placed;
	}

	const int32 Index = PieceOnCell(HoverX, HoverY);
	if (Index == INDEX_NONE)
	{
		return EConfirm::None;
	}
	Held = Index;
	Home = Pieces[Index];
	GhostUsed = Home;
	GhostUsed.X = HoverX;
	GhostUsed.Y = HoverY;
	return EConfirm::Lifted;
}

void FSolidArrange::Turn(const int32 Steps)
{
	if (!bOn || Held == INDEX_NONE || Steps == 0)
	{
		return;
	}
	GhostUsed.RotationDeg = FMath::UnwindDegrees(
		GhostUsed.RotationDeg + static_cast<float>(Steps) * FacingDeg);
}

int32 FSolidArrange::WheelSteps(const float Axis)
{
	const float Clamped = FMath::Clamp(Axis, -3.f, 3.f);
	return FMath::Clamp(FMath::RoundToInt(Clamped), -3, 3);
}

bool FSolidArrange::PutBack()
{
	if (Held == INDEX_NONE)
	{
		return false;
	}
	Pieces[Held] = Home;
	Held = INDEX_NONE;
	LastPlaced = INDEX_NONE;
	return true;
}

bool FSolidArrange::WritePlaced(TArray<SolidStrongholdFurniture::FUsed>& Furnishings) const
{
	if (!Pieces.IsValidIndex(LastPlaced))
	{
		return false;
	}

	const SolidStrongholdFurniture::FUsed& Next = Pieces[LastPlaced];
	for (SolidStrongholdFurniture::FUsed& Have : Furnishings)
	{
		if (Have.ItemIndex != Home.ItemIndex
			|| Have.Level != Home.Level
			|| Have.X != Home.X
			|| Have.Y != Home.Y
			|| !FMath::IsNearlyEqual(Have.RotationDeg, Home.RotationDeg, 0.01f))
		{
			continue;
		}
		Have = Next;
		return true;
	}
	return false;
}

const SolidStrongholdFurniture::FUsed& FSolidArrange::PieceAt(const int32 Index) const
{
	return Pieces.IsValidIndex(Index) ? Pieces[Index] : EmptyPiece();
}

bool FSolidArrange::DrawOf(const int32 Index, FDraw& Out) const
{
	if (!Pieces.IsValidIndex(Index))
	{
		return false;
	}
	const SolidStrongholdFurniture::FUsed& Used = Index == Held ? GhostUsed : Pieces[Index];
	const SolidStrongholdGrid::FHex* Anchor = SolidStrongholdFurniture::FindCell(Cells, Used.X, Used.Y);
	if (!Anchor)
	{
		return false;
	}
	const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(Cells, Used);
	const FVector2D Mid = Span ? (Anchor->Center + Span->Center) * 0.5f : Anchor->Center;
	Out.Local = FVector(Mid.X, Mid.Y, FloorHeight);
	Out.Yaw = Used.RotationDeg;
	Out.bGhost = Index == Held;
	return true;
}

bool FSolidArrange::CanPlace(const SolidStrongholdFurniture::FUsed& Candidate) const
{
	const SolidStrongholdGrid::FHex* Anchor = SolidStrongholdFurniture::FindCell(Cells, Candidate.X, Candidate.Y);
	const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Candidate.ItemIndex);
	if (!Anchor || !Item)
	{
		return false;
	}
	const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(Cells, Candidate);
	if (!Span)
	{
		return false;
	}
	if (Blocked(Anchor->Col, Anchor->Row) || Blocked(Span->Col, Span->Row))
	{
		return false;
	}
	if (PieceOnCell(Anchor->Col, Anchor->Row) != INDEX_NONE)
	{
		return false;
	}
	if ((Span->Col != Anchor->Col || Span->Row != Anchor->Row)
		&& PieceOnCell(Span->Col, Span->Row) != INDEX_NONE)
	{
		return false;
	}
	return true;
}

bool FSolidArrange::Trace(
	const FVector& LocalOrigin,
	const FVector& LocalDirection,
	const float MaxDistanceCm,
	FTrace& Out) const
{
	Out = FTrace();
	if (LocalDirection.IsNearlyZero())
	{
		return false;
	}
	const FVector Direction = LocalDirection.GetSafeNormal();
	if (FMath::Abs(Direction.Z) < KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float Distance = (FloorHeight - LocalOrigin.Z) / Direction.Z;
	if (Distance < 0.f || Distance > MaxDistanceCm)
	{
		return false;
	}
	Out.bPlane = true;
	Out.Point = LocalOrigin + Direction * Distance;
	Out.Point.Z = FloorHeight;
	const FVector2D Plan(Out.Point.X, Out.Point.Y);
	for (const SolidStrongholdGrid::FHex& Hex : Cells)
	{
		if (SolidStrongholdGrid::PointInHex(Plan, Hex.Center))
		{
			Out.bCell = true;
			Out.X = Hex.Col;
			Out.Y = Hex.Row;
			break;
		}
	}
	return true;
}

bool FSolidArrange::Blocked(const int32 X, const int32 Y) const
{
	if (bHasEntrance && Entrance.Col == X && Entrance.Row == Y)
	{
		return true;
	}
	return bHasCaptain && Captain.Col == X && Captain.Row == Y;
}

bool FSolidArrange::Covers(const SolidStrongholdFurniture::FUsed& Used, const int32 X, const int32 Y) const
{
	if (Used.X == X && Used.Y == Y)
	{
		return true;
	}
	const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(Cells, Used);
	return Span && Span->Col == X && Span->Row == Y;
}

int32 FSolidArrange::PieceOnCell(const int32 X, const int32 Y) const
{
	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		if (Index == Held)
		{
			continue;
		}
		if (Covers(Pieces[Index], X, Y))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}
