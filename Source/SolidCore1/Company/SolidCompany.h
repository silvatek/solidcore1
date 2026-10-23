#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BattlePlans/SolidBattlePlan.h"
#include "Company/SolidCaptain.h"
#include "Strongholds/SolidStronghold.h"

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

	/** Asset-registry token. Viking is SK_Viking. */
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

	/**
	 * #D6A892 for a generated head and hands.
	 * FLinearColor is linear, so the hex is converted. Storing the sRGB fractions
	 * (0.84, 0.66, 0.57) washes the head out to white under the stage light.
	 */
	inline FLinearColor SkinPink()
	{
		return FLinearColor(FColor(0xD6, 0xA8, 0x92));
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
 * Name, mesh reference, vocation, shirt, trouser, and skin colors, and the height, build, and gender sliders.
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
	FLinearColor SkinColor = FLinearColor(FColor(0xD6, 0xA8, 0x92));

	/** Height slider, 0 to 1. 0.5 leaves the build and gender alone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float BodyHeight = 0.5f;

	/** Build slider, 0 to 1. 0 is short and wide, 1 is tall and thin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float BodyBuild = 0.5f;

	/** Gender slider, 0 to 1. 0 is feminine, 1 is masculine, 0.5 changes nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float BodyGender = 0.5f;

	/** Fixed weapon. The companion does not swap it. The inventory page stays the captain's. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion")
	FString WeaponId;

	/** Fixed armour. The companion does not swap it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion")
	FString ArmourId;
};

/**
 * Every companion an event can add: Sam, Alex, Max, Morgan, and Quinn.
 * The company's own list is USolidCompany::AllCompanions, and it starts empty.
 */
namespace SolidCompanions
{
	const TArray<FSolidCompanion>& AllCompanions();
}

/**
 * Company — catalog of battle plans and every companion the squad may field.
 * Owns the captain and the active stronghold. The game instance holds one company for the play session.
 */
UCLASS()
class SOLIDCORE1_API USolidCompany : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * The company's one captain. Created on first ask. OpenLevel keeps this object.
	 * Title, occupied town, and sight unlocks live here.
	 */
	USolidCaptain* GetCaptain();

	/** Seed the catalog once. A catalog that already has plans is left as it is. */
	void InitializeDefaultBattlePlans();

	/** True when the catalog is those plans and no others, in that order. */
	bool HasOnlyDefaultBattlePlans() const;

	/** Seed Viking. The captain stays on Viking (idempotent). */
	void InitializeDefaultCharacterMeshes();

	/** Ensure the global companion list and this company's Viking mesh. The company's list stays empty. */
	void InitializeDefaultCompanions();

	/**
	 * Seed the Meagre Dungeon on the starting cell, facing north, named The cave.
	 * Idempotent while a stronghold is already held.
	 */
	void InitializeStartingStronghold();

	int32 GetBattlePlanCount() const { return AllBattlePlans.Num(); }

	const FSolidBattlePlan* GetBattlePlan(int32 Index) const;

	/** Catalog index of a plan name, or INDEX_NONE. */
	int32 FindBattlePlanIndexByName(const FString& Name) const;

	/** Append a plan to the Company catalog; returns its index. */
	int32 AddBattlePlan(const FSolidBattlePlan& Plan);

	/** Append a companion to the company catalog; returns the company index. */
	int32 AddCompanion(const FSolidCompanion& Companion);

	const FSolidCharacterMesh* GetCharacterMesh(int32 Index) const;

	int32 FindCharacterMeshIndexByName(const FString& Name) const;

	/** Soft mesh referenced by this companion, or unset when the index misses. */
	TSoftObjectPtr<USkeletalMesh> GetMeshForCompanion(const FSolidCompanion& Companion) const;

	int32 GetCompanionCount() const { return AllCompanions.Num(); }

	const FSolidCompanion* GetCompanion(int32 Index) const;

	/**
	 * Store height, build, and gender on one catalog entry.
	 * Returns false when the index misses or the stored bits are already those numbers.
	 */
	bool SetCompanionBodyControls(int32 Index, float Height, float Build, float Gender);

	int32 FindCompanionIndexByName(const FString& Name) const;

	/** The company base. Null until InitializeStartingStronghold or SetStronghold. */
	const FSolidStronghold* GetStronghold() const { return ActiveStronghold.Get(); }
	FSolidStronghold* GetStronghold() { return ActiveStronghold.Get(); }

	/** Take ownership of the company base. Replaces any previous stronghold. */
	void SetStronghold(TUniquePtr<FSolidStronghold> Stronghold);

	/** Release ownership of the company base. Used when Commit moves it to history. */
	TUniquePtr<FSolidStronghold> TakeStronghold();

protected:
	UPROPERTY()
	TObjectPtr<USolidCaptain> Captain;

	/** Not a UPROPERTY: FSolidBattlePlan is a plain class. */
	TArray<FSolidBattlePlan> AllBattlePlans;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company|CharacterMeshes")
	TArray<FSolidCharacterMesh> AllCharacterMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company|Companions")
	TArray<FSolidCompanion> AllCompanions;

	/** Not a UPROPERTY: FSolidDungeon, FSolidFortress, and FSolidTower stay plain classes. */
	TUniquePtr<FSolidStronghold> ActiveStronghold;
};
