#include "SolidCharacterEditor.h"
#include "SolidBodyMesh.h"

namespace
{
	const TCHAR* VocationName(const ESolidVocation Vocation)
	{
		switch (Vocation)
		{
		case ESolidVocation::Warrior: return TEXT("Warrior");
		case ESolidVocation::Rogue: return TEXT("Rogue");
		case ESolidVocation::Mage: return TEXT("Mage");
		case ESolidVocation::Cleric: return TEXT("Cleric");
		default: return TEXT("Companion");
		}
	}

	void AddChannel(TArray<FString>& Lines, const TCHAR* Label, const float Value)
	{
		Lines.Add(FString::Printf(TEXT("%s %.2f"), Label, Value));
	}

	void AddColor(TArray<FString>& Lines, const TCHAR* Label, const FLinearColor& Color)
	{
		Lines.Add(FString::Printf(TEXT("%s %.2f %.2f %.2f"), Label, Color.R, Color.G, Color.B));
	}
}

int32 SolidCharacterEditor::WrapIndex(const int32 Index, const int32 Delta, const int32 Count)
{
	if (Count <= 0)
	{
		return 0;
	}

	int32 Wrapped = (Index + Delta) % Count;
	if (Wrapped < 0)
	{
		Wrapped += Count;
	}
	return Wrapped;
}

void SolidCharacterEditor::Describe(const FSolidCompanion& Companion, TArray<FString>& OutLines)
{
	OutLines.Reset();
	OutLines.Add(Companion.Name);
	OutLines.Add(VocationName(Companion.Vocation));

	const FSolidBodyParams Params = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(Companion.Name));
	AddChannel(OutLines, TEXT("Height"), Params.Height);
	AddChannel(OutLines, TEXT("Shoulder width"), Params.ShoulderWidth);
	AddChannel(OutLines, TEXT("Hip width"), Params.HipWidth);
	AddChannel(OutLines, TEXT("Torso girth"), Params.TorsoGirth);
	AddChannel(OutLines, TEXT("Arm length"), Params.ArmLength);
	AddChannel(OutLines, TEXT("Leg length"), Params.LegLength);
	AddChannel(OutLines, TEXT("Head size"), Params.HeadSize);
	AddChannel(OutLines, TEXT("Neck length"), Params.NeckLength);
	AddChannel(OutLines, TEXT("Bulk"), Params.Bulk);
	AddColor(OutLines, TEXT("Shirt"), Companion.TorsoColor);
	AddColor(OutLines, TEXT("Trousers"), Companion.LegColor);
	AddColor(OutLines, TEXT("Skin"), Companion.SkinColor);
	OutLines.Add(TEXT("Left / Right"));
	OutLines.Add(TEXT("F10  open world"));
}
