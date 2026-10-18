#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Squad/SolidBattlePlan.h"
#include "Squad/SolidCompany.h"
#include "SolidSquad.generated.h"

class USolidCompany;

/**
 * Squad — Captain plus the companions chosen from the company.
 * Up to 8 assigned battle plans are drawn from the company catalog, one of which is active.
 * The selected companion list is the choice; the fielded list is that choice clamped
 * to the current max squad size (the events own that limit).
 */
UCLASS()
class SOLIDCORE1_API USolidSquad : public UObject
{
	GENERATED_BODY()

public:
	/** Bind to Company, assign starter plans, activate F1, select Sam then Alex. */
	void InitializeFromCompany(USolidCompany* InCompany);

	USolidCompany* GetCompany() const { return Company; }

	int32 GetAssignedCount() const { return AssignedCompanyPlanIndices.Num(); }
	int32 GetActiveAssignedSlot() const { return ActiveAssignedSlot; }

	/** Replace assigned list (truncated to MaxAssignedBattlePlans; invalid indices skipped). */
	bool SetAssignedBattlePlans(const TArray<int32>& CompanyPlanIndices);

	/** Select assigned slot 0..7 (must be within assigned count). F1 → slot 0. */
	bool SelectAssignedSlot(int32 SlotIndex);

	const FSolidBattlePlan* GetAssignedBattlePlan(int32 SlotIndex) const;
	const FSolidBattlePlan* GetActiveBattlePlan() const;

	ESolidBattleFormation GetActiveFormation() const;
	ESolidBattleSpacing GetActiveSpacing() const;

	FString GetActiveBattlePlanDebugString() const;

	int32 GetSelectedCompanionCount() const { return SelectedCompanionIndices.Num(); }
	const TArray<int32>& GetSelectedCompanionIndices() const { return SelectedCompanionIndices; }

	/**
	 * Replace the chosen companions (company indexes).
	 * Invalid indices and duplicates are skipped. An empty choice is allowed.
	 * Returns false when no company is bound.
	 */
	bool SetSelectedCompanions(const TArray<int32>& CompanyCompanionIndices);

	/**
	 * Chosen companions who are with the captain right now, in selection order.
	 * Stops at MaxCount. Missing company records are skipped.
	 */
	void CollectFieldedCompanions(int32 MaxCount, TArray<FSolidCompanion>& Out) const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<USolidCompany> Company;

	/** Indices into Company::AllBattlePlans (max 8). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|BattlePlans")
	TArray<int32> AssignedCompanyPlanIndices;

	/** Index into AssignedCompanyPlanIndices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|BattlePlans")
	int32 ActiveAssignedSlot = 0;

	/** Indices into Company::AllCompanions, in the order they join the squad. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Companions")
	TArray<int32> SelectedCompanionIndices;
};
