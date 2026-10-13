#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Party/SolidBattlePlan.h"

class USkeletalMesh;

#include "SolidCompany.generated.h"

/** One skeletal mesh the company can assign to a companion. */
USTRUCT(BlueprintType)
struct FSolidCharacterMesh
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMesh")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMesh")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/**
	 * Asset-registry token used when Mesh is not a direct path.
	 * Viking is SK_Viking. Assassin is the pack folder Medieval_Assassin_Girl.
	 * Blacksmith is the pack folder Elf_Blacksmith. Those folders may sit
	 * anywhere under Content.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMesh")
	FString AssetToken;
};

/** What a companion is trained to do. */
UENUM(BlueprintType)
enum class ESolidVocation : uint8
{
	Warrior UMETA(DisplayName = "Warrior"),
	Rogue UMETA(DisplayName = "Rogue"),
	Mage UMETA(DisplayName = "Mage"),
	Cleric UMETA(DisplayName = "Cleric"),
};

namespace SolidVocation
{
	/** One letter for the nameplate, such as W for Warrior. */
	inline const TCHAR* Initial(const ESolidVocation Vocation)
	{
		switch (Vocation)
		{
		case ESolidVocation::Warrior: return TEXT("W");
		case ESolidVocation::Rogue: return TEXT("R");
		case ESolidVocation::Mage: return TEXT("M");
		case ESolidVocation::Cleric: return TEXT("C");
		default: return TEXT("?");
		}
	}

	/** Nameplate text: "Sam (R)". */
	inline FString NameLabel(const FString& Name, const ESolidVocation Vocation)
	{
		return FString::Printf(TEXT("%s (%s)"), *Name, Initial(Vocation));
	}
}

/**
 * Plain matte clothes colors for a companion. The same name always draws the same pair.
 */
namespace SolidCompanionColors
{
	void PaletteForName(const FString& Name, FLinearColor& OutTorso, FLinearColor& OutLegs);

	/** Light pink for a generated head and hands. */
	inline FLinearColor SkinPink()
	{
		return FLinearColor(0.96f, 0.76f, 0.72f);
	}

	/** Sam's shirt and Alex's trousers. */
	inline FLinearColor ShirtRed()
	{
		return FLinearColor(0.75f, 0.12f, 0.10f);
	}

	/** Sam's trousers and Alex's shirt. */
	inline FLinearColor TrouserBlue()
	{
		return FLinearColor(0.10f, 0.22f, 0.72f);
	}

	/**
	 * Shirt, trousers, and skin for a companion name.
	 * Shirt and trousers match PaletteForName until a name is given its own clothes.
	 */
	void ColorsForName(
		const FString& Name,
		FLinearColor& OutTorso,
		FLinearColor& OutLegs,
		FLinearColor& OutSkin);
}

/**
 * One companion in the company catalog.
 * Name, mesh reference, vocation, and the shirt, trouser, and skin colors of the generated body.
 */
USTRUCT(BlueprintType)
struct FSolidCompanion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	FString Name;

	/** Index into USolidCompany::AllCharacterMeshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	int32 CharacterMeshIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	ESolidVocation Vocation = ESolidVocation::Warrior;

	/** Matte shirt: torso, arms, and neck. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	FLinearColor TorsoColor = FLinearColor(0.29f, 0.38f, 0.55f);

	/** Matte trousers: hips, legs, and feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	FLinearColor LegColor = FLinearColor(0.12f, 0.11f, 0.10f);

	/** Matte skin: head and hands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	FLinearColor SkinColor = FLinearColor(0.96f, 0.76f, 0.72f);
};

/**
 * Company — catalog of battle plans and every companion the party may field.
 */
UCLASS()
class SOLIDCORE1_API USolidCompany : public UObject
{
	GENERATED_BODY()

public:
	/** Seed Line / Column / Tight mob / Loose mob / Parade starter plans (idempotent). */
	void InitializeDefaultBattlePlans();

	/** Seed Viking, Assassin, and Blacksmith. Captain stays on Viking (idempotent). */
	void InitializeDefaultCharacterMeshes();

	/** Seed Sam, Alex, Max, Morgan, and Quinn, each with a mesh and a vocation (idempotent). */
	void InitializeDefaultCompanions();

	const TArray<FSolidBattlePlan>& GetAllBattlePlans() const { return AllBattlePlans; }
	int32 GetBattlePlanCount() const { return AllBattlePlans.Num(); }

	const FSolidBattlePlan* GetBattlePlan(int32 Index) const;

	int32 FindBattlePlanIndexByName(const FString& Name) const;

	/** Append a plan to the Company catalog; returns its index. */
	int32 AddBattlePlan(const FSolidBattlePlan& Plan);

	const TArray<FSolidCharacterMesh>& GetAllCharacterMeshes() const { return AllCharacterMeshes; }
	int32 GetCharacterMeshCount() const { return AllCharacterMeshes.Num(); }

	const FSolidCharacterMesh* GetCharacterMesh(int32 Index) const;

	int32 FindCharacterMeshIndexByName(const FString& Name) const;

	/** Append a mesh to the catalog; returns its index. */
	int32 AddCharacterMesh(const FSolidCharacterMesh& CharacterMesh);

	/** Soft mesh referenced by this companion, or unset when the index misses. */
	TSoftObjectPtr<USkeletalMesh> GetMeshForCompanion(const FSolidCompanion& Companion) const;

	const TArray<FSolidCompanion>& GetAllCompanions() const { return AllCompanions; }
	int32 GetCompanionCount() const { return AllCompanions.Num(); }

	const FSolidCompanion* GetCompanion(int32 Index) const;

	int32 FindCompanionIndexByName(const FString& Name) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company|BattlePlans")
	TArray<FSolidBattlePlan> AllBattlePlans;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company|CharacterMeshes")
	TArray<FSolidCharacterMesh> AllCharacterMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company|Companions")
	TArray<FSolidCompanion> AllCompanions;
};
