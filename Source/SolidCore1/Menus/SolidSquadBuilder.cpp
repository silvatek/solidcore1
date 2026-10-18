#include "SolidSquadBuilder.h"
#include "Squad/SolidCompany.h"
#include "Squad/SolidSquad.h"

void SolidSquadBuilder::CollectRows(const USolidSquad* Squad, TArray<FRow>& OutRows)
{
	OutRows.Reset();
	const USolidCompany* Company = Squad ? Squad->GetCompany() : nullptr;
	if (!Company)
	{
		return;
	}

	const TArray<int32>& Selected = Squad->GetSelectedCompanionIndices();
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

bool SolidSquadBuilder::Toggle(USolidSquad* Squad, const int32 CompanyIndex, const int32 MaxSquadSize)
{
	if (!Squad || !Squad->GetCompany() || !Squad->GetCompany()->GetCompanion(CompanyIndex))
	{
		return false;
	}

	TArray<int32> Next = Squad->GetSelectedCompanionIndices();
	const int32 At = Next.IndexOfByKey(CompanyIndex);
	if (At != INDEX_NONE)
	{
		Next.RemoveAt(At);
		return Squad->SetSelectedCompanions(Next);
	}

	if (Next.Num() >= MaxSquadSize)
	{
		return false;
	}

	Next.Add(CompanyIndex);
	return Squad->SetSelectedCompanions(Next);
}
