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
 * Squad boxes keep an empty gap. The companions with the captain are the filled
 * boxes from the top, and the walk stops at the event cap.
 */
UCLASS()
class SOLIDCORE1_API USolidSquad : public UObject
{
	GENERATED_BODY()

public:
	/** Bind to Company, assign starter plans, activate F1, select Sam then Alex. */
	void InitializeFromCompany(USolidCompany* InCompany);

	USolidCompany* GetCompany() const { return Company; }

	/** Filled keys. Empty keys between them are holes and are not counted. */
	int32 GetAssignedCount() const;
	const TArray<int32>& GetAssignedCompanyPlanIndices() const { return AssignedCompanyPlanIndices; }
	int32 GetActiveAssignedSlot() const { return ActiveAssignedSlot; }

	/**
	 * Eight fixed keys. INDEX_NONE is an empty key and stays where it was put.
	 * Any other missing index is skipped. A plan already on a key is skipped.
	 * The list stops at MaxAssignedBattlePlans.
	 */
	bool SetAssignedBattlePlans(const TArray<int32>& CompanyPlanIndices);

	/** Select a filled key, 0..7. F1 is slot 0. An empty key returns false. */
	bool SelectAssignedSlot(int32 SlotIndex);

	/** The corner mark goes away. The formation the companions already have stays. */
	void ClearActiveBattlePlan();

	/** Used when no key is active, so the companions keep the formation they had. */
	void SetHeldFormation(ESolidBattleFormation Formation, ESolidBattleSpacing Spacing);

	const FSolidBattlePlan* GetAssignedBattlePlan(int32 SlotIndex) const;
	const FSolidBattlePlan* GetActiveBattlePlan() const;

	ESolidBattleFormation GetActiveFormation() const;
	ESolidBattleSpacing GetActiveSpacing() const;

	FString GetActiveBattlePlanDebugString() const;

	/** Filled squad boxes. An empty box is a hole and is not counted. */
	int32 GetSelectedCompanionCount() const;
	const TArray<int32>& GetSelectedCompanionIndices() const { return SelectedCompanionIndices; }

	/**
	 * Replace the squad boxes (company indexes).
	 * INDEX_NONE is an empty box and stays between filled boxes. A trailing hole is dropped.
	 * Any other missing index, and a companion already in a box, is skipped.
	 * An empty choice is allowed. Returns false when no company is bound.
	 */
	bool SetSelectedCompanions(const TArray<int32>& CompanyCompanionIndices);

	/**
	 * Filled boxes from the top, skipping empty ones.
	 * Only the first MaxCount boxes are walked, so a hole is a gap and the walk stops at the cap.
	 */
	void CollectFieldedCompanions(int32 MaxCount, TArray<FSolidCompanion>& Out) const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<USolidCompany> Company;

	/** Indices into Company::AllBattlePlans. Eight keys. INDEX_NONE is an empty key. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|BattlePlans")
	TArray<int32> AssignedCompanyPlanIndices;

	/** Index into AssignedCompanyPlanIndices. INDEX_NONE when no plan is active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|BattlePlans")
	int32 ActiveAssignedSlot = 0;

	/** Kept when the active plan is cleared, so companions do not change formation. */
	ESolidBattleFormation HeldFormation = ESolidBattleFormation::Line;
	ESolidBattleSpacing HeldSpacing = ESolidBattleSpacing::Standard;

	/** Indices into Company::AllCompanions. INDEX_NONE is an empty box between filled ones. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad|Companions")
	TArray<int32> SelectedCompanionIndices;

	void RememberHeldFormation();
};
