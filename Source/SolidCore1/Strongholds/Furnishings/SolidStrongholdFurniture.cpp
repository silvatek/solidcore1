#include "SolidStrongholdFurniture.h"
#include "Actors/SolidBed.h"
#include "Actors/SolidChest.h"
#include "Actors/SolidFireplace.h"
#include "Actors/SolidFurnishing.h"
#include "Actors/SolidSmallTable.h"
#include "Inventory/SolidStorage.h"
#include "SolidCore1.h"
#include "Strongholds/SolidStronghold.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace
{
	SolidStrongholdFurniture::FItem MakeItem(
		const TCHAR* Name,
		UClass* ActorClass,
		const int32 Hexes,
		const int32 Limit,
		const bool bEnabled)
	{
		return SolidStrongholdFurniture::FItem(Name, ActorClass, Hexes, Limit, bEnabled, 0);
	}

	SolidStrongholdFurniture::FItem MakeStorage(
		const TCHAR* Name,
		UClass* ActorClass,
		const int32 Hexes,
		const int32 Limit,
		const bool bEnabled,
		const int32 StorageSlots)
	{
		return SolidStrongholdFurniture::FItem(Name, ActorClass, Hexes, Limit, bEnabled, StorageSlots);
	}
}

SolidStrongholdFurniture::FItem::FItem(
	const TCHAR* InName,
	UClass* InActorClass,
	const int32 InHexes,
	const int32 InLimit,
	const bool bInEnabled,
	const int32 InStorageSlots)
	: Name(InName)
	, ActorClass(InActorClass)
	, StorageSlots(InStorageSlots)
	, Limit(InLimit)
	, bEnabled(bInEnabled)
	, Hexes(InHexes)
{
}

int32 SolidStrongholdFurniture::FItem::HexCount() const
{
	return Hexes;
}

bool SolidStrongholdFurniture::FItem::IsStorage() const
{
	return StorageSlots > 0;
}

const TArray<SolidStrongholdFurniture::FItem>& SolidStrongholdFurniture::FurnitureCatalogue()
{
	static const TArray<FItem> Catalogue = {
		MakeItem(TEXT("Fireplace"), ASolidFireplace::StaticClass(), 1, 1, true),
		MakeItem(TEXT("Bed"), ASolidBed::StaticClass(), 2, 3, true),
		MakeStorage(TEXT("Chest"), ASolidChest::StaticClass(), 1, 3, true, SolidStorage::Count),
		MakeItem(TEXT("Small Table"), ASolidSmallTable::StaticClass(), 1, 10, true),
	};
	return Catalogue;
}

TArray<SolidStrongholdFurniture::FItem> SolidStrongholdFurniture::StartingCatalog()
{
	return FurnitureCatalogue();
}

const SolidStrongholdFurniture::FItem* SolidStrongholdFurniture::ItemAt(const int32 Index)
{
	const TArray<FItem>& Catalogue = FurnitureCatalogue();
	return Catalogue.IsValidIndex(Index) ? &Catalogue[Index] : nullptr;
}

int32 SolidStrongholdFurniture::IndexOfActor(const UClass* ActorClass)
{
	const TArray<FItem>& Catalogue = FurnitureCatalogue();
	for (int32 Index = 0; Index < Catalogue.Num(); ++Index)
	{
		if (Catalogue[Index].ActorClass == ActorClass)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

const SolidStrongholdGrid::FHex* SolidStrongholdFurniture::FPlacedFurnishing::SpanCell(
	const TArray<SolidStrongholdGrid::FHex>& Hexes) const
{
	const FItem* Item = ItemAt(ItemIndex);
	const SolidStrongholdGrid::FHex* Anchor = FindCell(Hexes, X, Y);
	if (!Item || !Anchor)
	{
		return nullptr;
	}
	if (Item->HexCount() < 2)
	{
		return Anchor;
	}

	const FVector Forward = FRotator(0.f, RotationDeg, 0.f).Vector();
	const FVector2D Target = Anchor->Center
		+ FVector2D(Forward.X, Forward.Y) * SolidStrongholdGrid::FlatToFlatCm;
	const SolidStrongholdGrid::FHex* Best = nullptr;
	float BestDist = SolidStrongholdGrid::FlatToFlatCm * 0.45f;
	for (const SolidStrongholdGrid::FHex& Hex : Hexes)
	{
		const float Dist = FVector2D::Distance(Hex.Center, Target);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = &Hex;
		}
	}
	return Best;
}

bool SolidStrongholdFurniture::FPlacedFurnishing::Covers(
	const TArray<SolidStrongholdGrid::FHex>& Hexes,
	const int32 Col,
	const int32 Row) const
{
	if (X == Col && Y == Row)
	{
		return true;
	}
	const SolidStrongholdGrid::FHex* Span = SpanCell(Hexes);
	return Span && Span->Col == Col && Span->Row == Row;
}

void SolidStrongholdFurniture::FPlacedFurnishing::GiveSlots()
{
	const FItem* Kind = ItemAt(ItemIndex);
	Slots.SetNum(Kind ? Kind->StorageSlots : 0);
}

SolidStorage::FContainer SolidStrongholdFurniture::FPlacedFurnishing::Container() const
{
	SolidStorage::FContainer Out;
	const int32 N = FMath::Min(SolidStorage::Count, Slots.Num());
	for (int32 Index = 0; Index < N; ++Index)
	{
		Out.Slots[Index] = Slots[Index];
	}
	return Out;
}

void SolidStrongholdFurniture::FPlacedFurnishing::SetContainer(const SolidStorage::FContainer& Container)
{
	const FItem* Kind = ItemAt(ItemIndex);
	if (!Kind || !Kind->IsStorage())
	{
		Slots.Reset();
		return;
	}
	Slots.SetNum(Kind->StorageSlots);
	const int32 N = FMath::Min(Kind->StorageSlots, SolidStorage::Count);
	for (int32 Index = 0; Index < N; ++Index)
	{
		Slots[Index] = Container.Slots[Index];
	}
}

void SolidStrongholdFurniture::UsedFurnishings(const FSolidStronghold& Stronghold, TArray<FPlacedFurnishing>& OutUsed)
{
	OutUsed.Reset();
	if (Stronghold.Type != ESolidStrongholdType::Dungeon
		|| Stronghold.Size != ESolidStrongholdSize::Meagre)
	{
		return;
	}

	OutUsed.Add({ IndexOfActor(ASolidFireplace::StaticClass()), 0, 2, 0, 0.f });
	OutUsed.Add({ IndexOfActor(ASolidBed::StaticClass()), 0, 0, -1, 0.f });
	OutUsed.Add({ IndexOfActor(ASolidChest::StaticClass()), 0, 2, 1, 0.f });
	bool bFilledChest = false;
	for (FPlacedFurnishing& Piece : OutUsed)
	{
		Piece.GiveSlots();
		const FItem* Kind = ItemAt(Piece.ItemIndex);
		if (!bFilledChest && Kind && Kind->IsStorage())
		{
			Piece.SetContainer(SolidStorage::StartingCaveChest());
			bFilledChest = true;
		}
	}
}

TArray<SolidStrongholdFurniture::FPlacedFurnishing> SolidStrongholdFurniture::StartingFurnishings()
{
	TArray<FPlacedFurnishing> Used;
	UsedFurnishings(
		FSolidStronghold(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre),
		Used);
	return Used;
}

namespace
{
	bool SameFurnishing(
		const SolidStrongholdFurniture::FPlacedFurnishing& A,
		const SolidStrongholdFurniture::FPlacedFurnishing& B)
	{
		return A.ItemIndex == B.ItemIndex
			&& A.Level == B.Level
			&& A.X == B.X
			&& A.Y == B.Y
			&& FMath::IsNearlyEqual(A.RotationDeg, B.RotationDeg, 0.01f);
	}

	bool FurnishingsMatch(
		const TArray<ASolidFurnishing*>& Owned,
		const TArray<SolidStrongholdFurniture::FPlacedFurnishing>& Used)
	{
		if (Owned.Num() != Used.Num())
		{
			return false;
		}
		TArray<int32> Taken;
		for (const SolidStrongholdFurniture::FPlacedFurnishing& Want : Used)
		{
			int32 Found = INDEX_NONE;
			for (int32 Index = 0; Index < Owned.Num(); ++Index)
			{
				if (Taken.Contains(Index) || !Owned[Index] || !Owned[Index]->HasPlacement())
				{
					continue;
				}
				if (SameFurnishing(Owned[Index]->GetPlacement(), Want))
				{
					Found = Index;
					break;
				}
			}
			if (Found == INDEX_NONE)
			{
				return false;
			}
			Taken.Add(Found);
		}
		return true;
	}
}

FTransform SolidStrongholdFurniture::FloorLocal(const FVector2D& Mid, const float YawDeg)
{
	return FTransform(FRotator(0.f, YawDeg, 0.f), FVector(Mid.X, Mid.Y, 0.f));
}

int32 SolidStrongholdFurniture::PlaceFurnishings(
	AActor* Owner,
	const TArray<FPlacedFurnishing>& Used,
	const TArray<SolidStrongholdGrid::FHex>& Hexes,
	TFunctionRef<FTransform(const FVector2D& Mid, float YawDeg)> ToLocal)
{
	if (!Owner)
	{
		return 0;
	}
	UWorld* World = Owner->GetWorld();
	if (!World || !World->IsGameWorld() || World->bIsTearingDown || !Owner->IsActorInitialized())
	{
		return 0;
	}

	TArray<ASolidFurnishing*> Owned;
	for (TActorIterator<ASolidFurnishing> It(World); It; ++It)
	{
		ASolidFurnishing* Piece = *It;
		if (Piece && Piece->GetOwner() == Owner && !Piece->IsActorBeingDestroyed())
		{
			Owned.Add(Piece);
		}
	}
	if (FurnishingsMatch(Owned, Used))
	{
		return Owned.Num();
	}
	for (ASolidFurnishing* Piece : Owned)
	{
		Piece->Destroy();
	}

	int32 Placed = 0;
	for (const FPlacedFurnishing& UsedPiece : Used)
	{
		const FItem* Item = ItemAt(UsedPiece.ItemIndex);
		if (!Item || UsedPiece.Level != 0)
		{
			continue;
		}

		const SolidStrongholdGrid::FHex* Anchor = FindCell(Hexes, UsedPiece.X, UsedPiece.Y);
		const SolidStrongholdGrid::FHex* Span = UsedPiece.SpanCell(Hexes);
		if (!Anchor || !Span)
		{
			UE_LOG(LogSolid, Error, TEXT("%s: furnishing hex (%d,%d) is not on the floor."),
				*Owner->GetName(), UsedPiece.X, UsedPiece.Y);
			continue;
		}

		const FVector2D Mid = (Anchor->Center + Span->Center) * 0.5f;
		const FTransform Local = ToLocal(Mid, UsedPiece.RotationDeg);
		const FTransform ActorXf = Owner->GetActorTransform();
		const FVector WorldPos = ActorXf.TransformPosition(Local.GetLocation());
		const FRotator WorldRot = ActorXf.TransformRotation(Local.GetRotation()).Rotator();
		const FTransform SpawnXf(WorldRot, WorldPos);

		ASolidFurnishing* Piece = World->SpawnActorDeferred<ASolidFurnishing>(
			Item->ActorClass,
			SpawnXf,
			Owner,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Piece)
		{
			UE_LOG(LogSolid, Error, TEXT("%s: failed to spawn %s."), *Owner->GetName(), Item->Name);
			continue;
		}

		Piece->FinishSpawning(SpawnXf);
		Piece->SetPlacement(UsedPiece);
		Piece->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
		++Placed;
	}

	UE_LOG(LogSolid, Warning, TEXT("%s furnishings placed: %d."), *Owner->GetName(), Placed);
	return Placed;
}
