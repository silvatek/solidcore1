#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "SolidStrongholdInterior.generated.h"

class ASolidFurnishing;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * An actor drawn for one stronghold visit. Dungeon and tower interiors are the kinds.
 * The stronghold names which kind, and fills it.
 */
UCLASS()
class SOLIDCORE1_API ASolidStrongholdInterior : public AActor
{
	GENERATED_BODY()

public:
	ASolidStrongholdInterior();

	/** Feet position where the captain arrives. */
	virtual FVector GetCaptainSpawnWorld() const { return FVector::ZeroVector; }

	/** View on arrival. */
	virtual FRotator GetCaptainView() const { return FRotator::ZeroRotator; }

	/** Dark opening a click uses to leave. Null when this interior has none. */
	virtual UStaticMeshComponent* GetPassageMesh() const { return nullptr; }

	/** Arrange furniture plates on the interior wall. None until a kind builds them. */
	virtual int32 ArrangeSignCount() const { return 0; }
	/** Done arranging while the mode is on. The resting words are Arrange furniture. */
	virtual void SetArrangeSignDone(bool bDone);
	virtual UStaticMeshComponent* GetArrangeSignPlate(int32 Index) const
	{
		(void)Index;
		return nullptr;
	}

	void SetStrongholdCell(FIntPoint Cell) { StrongholdCell = Cell; }
	FIntPoint GetStrongholdCell() const { return StrongholdCell; }

	/** Floors this interior can arrange. The captain's floor is the one under his feet. */
	virtual int32 ArrangeFloorCount() const;
	virtual bool CollectArrangeFloor(int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutCells) const;
	virtual bool ArrangeFloorZ(int32 Floor, float& OutZ) const;
	/** Ground-floor entrance, when that hex is still a furniture cell. */
	virtual bool ArrangeEntrance(int32 Floor, int32& OutX, int32& OutY) const;

	/** A hex-grid point, placed in this room. The cave and the tower already store hexes that way. */
	virtual FVector HexPointInRoom(const FVector& HexPoint) const { return HexPoint; }
	/** A point in the room, back on the hex grid. */
	virtual FVector RoomPointOnHexGrid(const FVector& RoomPoint) const { return RoomPoint; }
	/** A hex-grid direction, turned into the room. */
	virtual FVector HexDirectionInRoom(const FVector& HexDirection) const { return HexDirection; }
	/** A direction in the room, turned back onto the hex grid. */
	virtual FVector RoomDirectionOnHexGrid(const FVector& RoomDirection) const { return RoomDirection; }
	/** A hex-grid facing, turned to face the same way in the room. */
	virtual float HexFacingInRoom(float HexFacingDeg) const { return HexFacingDeg; }

	/** INDEX_NONE when the feet are below every floor. */
	int32 ArrangeFloorUnderFeet(const FVector& WorldFeet) const;

	/** Bright edges on the hexes under the pointer. An empty list clears them. */
	void ShowArrangeHighlight(const TArray<SolidStrongholdGrid::FHex>& Hexes, float FloorZ);
	void HideArrangeHighlight();

	/** Move an owned piece. The point and facing are on the hex grid. */
	void MoveFurnishingTo(ASolidFurnishing* Piece, const FVector& HexPoint, float HexFacingDeg) const;

private:
	FIntPoint StrongholdCell = FIntPoint::ZeroValue;

	/** Hex edges drawn while arrange mode is on. This interior owns the component. */
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> ArrangeHighlight;
};
