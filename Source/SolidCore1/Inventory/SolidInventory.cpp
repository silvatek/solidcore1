#include "SolidInventory.h"

namespace SolidInventory
{
	/** Gap between worn boxes. */
	inline constexpr float Gap = 12.f;
	/** Gap between the figure and the pack. */
	inline constexpr float SideGap = 64.f;

	inline constexpr float HeadW = 148.f;
	inline constexpr float HeadH = 80.f;
	inline constexpr float TorsoW = 176.f;
	inline constexpr float BodyH = 148.f;
	inline constexpr float BeltH = 48.f;
	inline constexpr float BootsH = 120.f;
	inline constexpr float HandW = 132.f;
	inline constexpr float HandH = 104.f;

	inline constexpr float PackCell = 112.f;
	inline constexpr float PackGap = 10.f;
	/** Height of a Pack, Chest, or worn-slot heading. The box starts one Gap below it. */
	inline constexpr float PackLabelH = 22.f;

	/** Belt is narrower than the body so the waist reads as a belt. */
	inline constexpr float BeltW = 128.f;

	FBox MakeWorn(
		const FLoadout& Loadout,
		const ESlot Slot,
		const float X,
		const float Y,
		const float W,
		const float H,
		const float LabelY)
	{
		FBox Box;
		Box.X = X;
		Box.Y = Y;
		Box.W = W;
		Box.H = H;
		Box.Slot = Slot;
		Box.LabelY = LabelY;
		Box.Label = SlotName(Slot);
		const int32 Index = WornIndex(Slot);
		if (Index != INDEX_NONE)
		{
			Box.Item = SolidItemType::PageLabel(Loadout.Worn[Index], Loadout.WornStack[Index]);
		}
		return Box;
	}

	void Layout(const FLoadout& Loadout, FPage& OutPage, const SolidStorage::FContainer* Chest)
	{
		OutPage = FPage();

		const float TorsoX = HandW + Gap;
		const float HeadX = TorsoX + (TorsoW - HeadW) * 0.5f;
		const float Heading = PackLabelH + Gap;
		const float HeadLabelY = 0.f;
		const float HeadY = Heading;
		const float BodyLabelY = HeadY + HeadH + Gap;
		const float BodyY = BodyLabelY + Heading;
		const float HandY = BodyY + (BodyH - HandH) * 0.5f;
		const float HandLabelY = HandY - Heading;
		const float RightX = TorsoX + TorsoW + Gap;
		const float BeltX = TorsoX + (TorsoW - BeltW) * 0.5f;
		const float BeltLabelY = BodyY + BodyH + Gap;
		const float BeltY = BeltLabelY + Heading;
		const float BootsLabelY = BeltY + BeltH + Gap;
		const float BootsY = BootsLabelY + Heading;

		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::Head, HeadX, HeadY, HeadW, HeadH, HeadLabelY));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::Body, TorsoX, BodyY, TorsoW, BodyH, BodyLabelY));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::OffHand, 0.f, HandY, HandW, HandH, HandLabelY));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::MainHand, RightX, HandY, HandW, HandH, HandLabelY));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::Belt, BeltX, BeltY, BeltW, BeltH, BeltLabelY));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::Boots, TorsoX, BootsY, TorsoW, BootsH, BootsLabelY));

		const float DollRight = RightX + HandW;
		const float DollBottom = BootsY + BootsH;
		const float PackX = DollRight + SideGap;
		const float GridY = PackLabelH + Gap;
		const float GridW = PackColumns * PackCell + (PackColumns - 1) * PackGap;
		const float GridH = PackRows * PackCell + (PackRows - 1) * PackGap;

		for (int32 Row = 0; Row < PackRows; ++Row)
		{
			for (int32 Column = 0; Column < PackColumns; ++Column)
			{
				FBox Box;
				Box.bPack = true;
				Box.Column = Column;
				Box.Row = Row;
				Box.X = PackX + Column * (PackCell + PackGap);
				Box.Y = GridY + Row * (PackCell + PackGap);
				Box.W = PackCell;
				Box.H = PackCell;
				const int32 Index = PackIndex(Column, Row);
				if (Index != INDEX_NONE)
				{
					Box.Item = SolidItemType::PageLabel(Loadout.Pack[Index], Loadout.PackStack[Index]);
				}
				OutPage.Boxes.Add(Box);
			}
		}

		OutPage.PackX = PackX;
		OutPage.PackLabelY = 0.f;
		OutPage.PackGridW = GridW;
		float ContentRight = PackX + GridW;
		float ContentBottom = FMath::Max(DollBottom, GridY + GridH);

		if (Chest)
		{
			const float ChestX = ContentRight + SideGap;
			const float ChestGridW = SolidStorage::Columns * PackCell + (SolidStorage::Columns - 1) * PackGap;
			const float ChestGridH = SolidStorage::Rows * PackCell + (SolidStorage::Rows - 1) * PackGap;
			for (int32 Row = 0; Row < SolidStorage::Rows; ++Row)
			{
				for (int32 Column = 0; Column < SolidStorage::Columns; ++Column)
				{
					FBox Box;
					Box.bChest = true;
					Box.Column = Column;
					Box.Row = Row;
					Box.X = ChestX + Column * (PackCell + PackGap);
					Box.Y = GridY + Row * (PackCell + PackGap);
					Box.W = PackCell;
					Box.H = PackCell;
					const int32 Index = SolidStorage::Index(Column, Row);
					if (Index != INDEX_NONE)
					{
						Box.Item = SolidItemType::PageLabel(Chest->Slots[Index], Chest->Counts[Index]);
					}
					OutPage.Boxes.Add(Box);
				}
			}
			OutPage.bShowChest = true;
			OutPage.ChestX = ChestX;
			OutPage.ChestLabelY = 0.f;
			OutPage.ChestGridW = ChestGridW;
			ContentRight = ChestX + ChestGridW;
			ContentBottom = FMath::Max(ContentBottom, GridY + ChestGridH);
		}

		// A short bar, centered on the row, smaller than a slot in both directions.
		const float CraftButtonW = 96.f;
		const float CraftButtonH = 36.f;
		const float RowW = CraftCount * PackCell + (CraftCount - 1) * PackGap + PackGap + CraftButtonW;
		if (RowW > ContentRight)
		{
			ContentRight = RowW;
		}
		const float RowX = ContentRight - RowW;
		const float RowY = ContentBottom + Gap;
		for (int32 Index = 0; Index < CraftCount; ++Index)
		{
			FBox Box;
			Box.bCraft = true;
			Box.Slot = ESlot::Count;
			Box.Column = Index;
			Box.X = RowX + Index * (PackCell + PackGap);
			Box.Y = RowY;
			Box.W = PackCell;
			Box.H = PackCell;
			Box.Item = SolidItemType::PageLabel(Loadout.Craft[Index], Loadout.CraftStack[Index]);
			OutPage.Boxes.Add(Box);
		}
		OutPage.CraftButtonX = RowX + CraftCount * (PackCell + PackGap);
		OutPage.CraftButtonY = RowY + (PackCell - CraftButtonH) * 0.5f;
		OutPage.CraftButtonW = CraftButtonW;
		OutPage.CraftButtonH = CraftButtonH;
		OutPage.bCraftEnabled = false;
		ContentBottom = RowY + PackCell;

		OutPage.ContentW = ContentRight;
		OutPage.ContentH = ContentBottom;
	}
}
