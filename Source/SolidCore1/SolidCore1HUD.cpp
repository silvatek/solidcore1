#include "SolidCore1HUD.h"
#include "SolidCore1BuildId.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ASolidCore1HUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const FString Label = FString::Printf(TEXT("Build %s"), SOLIDCORE1_BUILD_ID);
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	const float PadX = 16.f;
	const float PadY = 12.f;

	float TextWidth = 0.f;
	float TextHeight = 0.f;
	if (Font)
	{
		Canvas->StrLen(Font, Label, TextWidth, TextHeight);
	}
	else
	{
		TextWidth = static_cast<float>(Label.Len() * 8);
		TextHeight = 14.f;
	}

	const float BoxPad = 6.f;
	const float X = PadX;
	const float Y = PadY;

	FCanvasTileItem Background(
		FVector2D(X - BoxPad, Y - BoxPad * 0.5f),
		FVector2D(TextWidth + BoxPad * 2.f, TextHeight + BoxPad),
		FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Background.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Background);

	FCanvasTextItem TextItem(FVector2D(X, Y), FText::FromString(Label), Font, FLinearColor::White);
	TextItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	Canvas->DrawItem(TextItem);
}
