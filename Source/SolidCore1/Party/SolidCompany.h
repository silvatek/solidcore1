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

/**
 * One companion in the company catalog.
 * Name, mesh reference, and vocation; more attributes will be added later.
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

	/** Seed the character mesh catalog. Starts with Viking only (idempotent). */
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
