#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Party/SolidBattlePlan.h"

class USkeletalMesh;

#include "SolidCompany.generated.h"

/**
 * One companion in the company catalog.
 * Name and mesh for now; more attributes will be added later.
 */
USTRUCT(BlueprintType)
struct FSolidCompanyCompanion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TSoftObjectPtr<USkeletalMesh> Mesh;
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

	/** Seed Sam, Alex, Max, Morgan, and Quinn. Each entry sets its own mesh (idempotent). */
	void InitializeDefaultCompanions();

	const TArray<FSolidBattlePlan>& GetAllBattlePlans() const { return AllBattlePlans; }
	int32 GetBattlePlanCount() const { return AllBattlePlans.Num(); }

	const FSolidBattlePlan* GetBattlePlan(int32 Index) const;

	int32 FindBattlePlanIndexByName(const FString& Name) const;

	/** Append a plan to the Company catalog; returns its index. */
	int32 AddBattlePlan(const FSolidBattlePlan& Plan);

	const TArray<FSolidCompanyCompanion>& GetAllCompanions() const { return AllCompanions; }
	int32 GetCompanionCount() const { return AllCompanions.Num(); }

	const FSolidCompanyCompanion* GetCompanion(int32 Index) const;

	int32 FindCompanionIndexByName(const FString& Name) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company|BattlePlans")
	TArray<FSolidBattlePlan> AllBattlePlans;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company|Companions")
	TArray<FSolidCompanyCompanion> AllCompanions;
};
