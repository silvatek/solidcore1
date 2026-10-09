#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Party/SolidBattlePlan.h"
#include "SolidCompany.generated.h"

/**
 * Company — owns the full catalog of battle plans the Party may be assigned.
 */
UCLASS()
class SOLIDCORE1_API USolidCompany : public UObject
{
	GENERATED_BODY()

public:
	/** Seed Line / Column / Tight mob / Loose mob starter plans (idempotent). */
	void InitializeDefaultBattlePlans();

	const TArray<FSolidBattlePlan>& GetAllBattlePlans() const { return AllBattlePlans; }
	int32 GetBattlePlanCount() const { return AllBattlePlans.Num(); }

	const FSolidBattlePlan* GetBattlePlan(int32 Index) const;

	int32 FindBattlePlanIndexByName(const FString& Name) const;

	/** Append a plan to the Company catalog; returns its index. */
	int32 AddBattlePlan(const FSolidBattlePlan& Plan);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company|BattlePlans")
	TArray<FSolidBattlePlan> AllBattlePlans;
};
