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
	inline constexpr float LegsH = 120.f;
	inline constexpr float HandW = 132.f;
	inline constexpr float HandH = 104.f;

	inline constexpr float PackCell = 112.f;
	inline constexpr float PackGap = 10.f;
	inline constexpr float PackLabelH = 22.f;

	/** Belt is narrower than the body so the waist reads as a belt. */
	inline constexpr float BeltW = 128.f;

	FBox MakeWorn(const FLoadout& Loadout, const ESlot Slot, const float X, const float Y, const float W, const float H)
	{
		FBox Box;
		Box.X = X;
		Box.Y = Y;
		Box.W = W;
		Box.H = H;
		Box.Slot = Slot;
		Box.Label = SlotName(Slot);
		const int32 Index = WornIndex(Slot);
		if (Index != INDEX_NONE)
		{
			Box.Item = Loadout.Worn[Index];
		}
		return Box;
	}

	void Layout(const FLoadout& Loadout, FPage& OutPage)
	{
		OutPage = FPage();

		const float TorsoX = HandW + Gap;
		const float HeadX = TorsoX + (TorsoW - HeadW) * 0.5f;
		const float HeadY = 0.f;
		const float BodyY = HeadY + HeadH + Gap;
		const float HandY = BodyY + (BodyH - HandH) * 0.5f;
		const float RightX = TorsoX + TorsoW + Gap;
		const float BeltX = TorsoX + (TorsoW - BeltW) * 0.5f;
		const float BeltY = BodyY + BodyH + Gap;
		const float LegsY = BeltY + BeltH + Gap;

		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::Head, HeadX, HeadY, HeadW, HeadH));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::Body, TorsoX, BodyY, TorsoW, BodyH));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::LeftHand, 0.f, HandY, HandW, HandH));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::RightHand, RightX, HandY, HandW, HandH));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::Belt, BeltX, BeltY, BeltW, BeltH));
		OutPage.Boxes.Add(MakeWorn(Loadout, ESlot::Legs, TorsoX, LegsY, TorsoW, LegsH));

		const float DollRight = RightX + HandW;
		const float DollBottom = LegsY + LegsH;
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
					Box.Item = Loadout.Pack[Index];
				}
				OutPage.Boxes.Add(Box);
			}
		}

		OutPage.PackX = PackX;
		OutPage.PackLabelY = 0.f;
		OutPage.PackGridW = GridW;
		OutPage.ContentW = PackX + GridW;
		OutPage.ContentH = FMath::Max(DollBottom, GridY + GridH);
	}
}
