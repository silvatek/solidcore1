#include "SolidPartyBuilder.h"
#include "Party/SolidCompany.h"
#include "Party/SolidParty.h"

void SolidPartyBuilder::CollectRows(const USolidParty* Party, TArray<FRow>& OutRows)
{
	OutRows.Reset();
	const USolidCompany* Company = Party ? Party->GetCompany() : nullptr;
	if (!Company)
	{
		return;
	}

	const TArray<int32>& Selected = Party->GetSelectedCompanionIndices();
	for (int32 Index = 0; Index < Company->GetCompanionCount(); ++Index)
	{
		const FSolidCompanion* Companion = Company->GetCompanion(Index);
		if (!Companion)
		{
			continue;
		}

		FRow Row;
		Row.CompanyIndex = Index;
		Row.bSelected = Selected.Contains(Index);
		const FString Name = SolidVocation::NameLabel(Companion->Name, Companion->Vocation);
		Row.Label = FString::Printf(TEXT("%s  %s"), Row.bSelected ? TEXT("[x]") : TEXT("[ ]"), *Name);
		OutRows.Add(Row);
	}
}

bool SolidPartyBuilder::Toggle(USolidParty* Party, const int32 CompanyIndex, const int32 MaxPartySize)
{
	if (!Party || !Party->GetCompany() || !Party->GetCompany()->GetCompanion(CompanyIndex))
	{
		return false;
	}

	TArray<int32> Next = Party->GetSelectedCompanionIndices();
	const int32 At = Next.IndexOfByKey(CompanyIndex);
	if (At != INDEX_NONE)
	{
		Next.RemoveAt(At);
		return Party->SetSelectedCompanions(Next);
	}

	if (Next.Num() >= MaxPartySize)
	{
		return false;
	}

	Next.Add(CompanyIndex);
	return Party->SetSelectedCompanions(Next);
}
