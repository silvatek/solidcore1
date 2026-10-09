#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Party/SolidBattlePlan.h"
#include "SolidParty.generated.h"

class USolidCompany;

/**
 * Party — Captain + active Companions, with up to 8 assigned battle plans
 * drawn from the Company catalog, one of which is active.
 */
UCLASS()
class SOLIDCORE1_API USolidParty : public UObject
{
	GENERATED_BODY()

public:
	/** Bind to Company, assign starter plans (Line/Column/Mob), activate Column. */
	void InitializeFromCompany(USolidCompany* InCompany);

	USolidCompany* GetCompany() const { return Company; }

	int32 GetAssignedCount() const { return AssignedCompanyPlanIndices.Num(); }
	int32 GetActiveAssignedSlot() const { return ActiveAssignedSlot; }

	const TArray<int32>& GetAssignedCompanyPlanIndices() const { return AssignedCompanyPlanIndices; }

	/** Replace assigned list (truncated to MaxAssignedBattlePlans; invalid indices skipped). */
	bool SetAssignedBattlePlans(const TArray<int32>& CompanyPlanIndices);

	/** Select assigned slot 0..7 (must be within assigned count). F1 → slot 0. */
	bool SelectAssignedSlot(int32 SlotIndex);

	const FSolidBattlePlan* GetAssignedBattlePlan(int32 SlotIndex) const;
	const FSolidBattlePlan* GetActiveBattlePlan() const;

	ESolidBattleFormation GetActiveFormation() const;

	FString GetActiveBattlePlanDebugString() const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<USolidCompany> Company;

	/** Indices into Company::AllBattlePlans (max 8). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Party|BattlePlans")
	TArray<int32> AssignedCompanyPlanIndices;

	/** Index into AssignedCompanyPlanIndices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Party|BattlePlans")
	int32 ActiveAssignedSlot = 0;
};
