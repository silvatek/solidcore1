#include "SolidMaterials.h"
#include "SolidCore1.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UMaterialInterface* SolidMaterials::CreateSolidColor(
	UObject* Outer,
	const FLinearColor& Color,
	const TCHAR* DebugName,
	float Roughness)
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
		UE_LOG(LogSolid, Error,
			TEXT("SolidMaterials::CreateSolidColor(%s): no FlatCol / DefaultColorway parent."),
			DebugName ? DebugName : TEXT("?"));
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
	if (!MID)
	{
		return Parent;
	}

	MID->SetVectorParameterValue(TEXT("Base Color"), Color);
	MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
	MID->SetScalarParameterValue(TEXT("Roughness"), FMath::Clamp(Roughness, 0.f, 1.f));
	return MID;
}
