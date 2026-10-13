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

	const FSolidBodyParams Seed = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(Companion.Name));
	FSolidBodyControls Controls;
	Controls.Height = Companion.BodyHeight;
	Controls.Build = Companion.BodyBuild;
	Controls.Gender = Companion.BodyGender;
	const FSolidBodyParams Params = SolidBodyControls::Resolve(Seed, Controls);

	AddChannel(OutLines, TEXT("Height"), Controls.Height);
	AddChannel(OutLines, TEXT("Build"), Controls.Build);
	AddChannel(OutLines, TEXT("Gender"), Controls.Gender);
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
	OutLines.Add(TEXT("Up / Down"));
	OutLines.Add(TEXT("Page Up / Page Down"));
	OutLines.Add(TEXT("F10  open world"));
}

int32 SolidCharacterEditor::SliderForLine(const FString& Line)
{
	if (Line.StartsWith(TEXT("Height ")))
	{
		return SolidBodyControls::Height;
	}
	if (Line.StartsWith(TEXT("Build ")))
	{
		return SolidBodyControls::Build;
	}
	if (Line.StartsWith(TEXT("Gender ")))
	{
		return SolidBodyControls::Gender;
	}
	return INDEX_NONE;
}

FSolidSliderRow SolidCharacterEditor::MakeSliderRow(
	const int32 Slider,
	const float LabelX,
	const float LineY,
	const float LineH,
	const float TextRight)
{
	FSolidSliderRow Row;
	Row.Slider = Slider;
	Row.X = LabelX;
	Row.Y = LineY;
	Row.H = LineH;
	Row.Bar.X = TextRight + SliderBarGap;
	Row.Bar.Y = LineY;
	Row.Bar.W = SliderBarWidth;
	Row.Bar.H = LineH;
	return Row;
}

FSolidSliderHit SolidCharacterEditor::HitSliders(
	const FSolidSliderRow* Rows,
	const int32 Count,
	const float X,
	const float Y)
{
	FSolidSliderHit Hit;
	if (!Rows || Count <= 0)
	{
		return Hit;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Rows[Index].Bar.Contains(X, Y))
		{
			Hit.Slider = Rows[Index].Slider;
			Hit.Kind = ESolidSliderHit::Bar;
			return Hit;
		}
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FSolidSliderRow& Row = Rows[Index];
		if (Y >= Row.Y && Y <= Row.Y + Row.H && X >= Row.X && X < Row.Bar.X)
		{
			Hit.Slider = Row.Slider;
			Hit.Kind = ESolidSliderHit::Label;
			return Hit;
		}
	}
	return Hit;
}

float SolidCharacterEditor::BarFraction(const FSolidSliderBar& Bar, const float X)
{
	if (Bar.W <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return FMath::Clamp((X - Bar.X) / Bar.W, 0.f, 1.f);
}
