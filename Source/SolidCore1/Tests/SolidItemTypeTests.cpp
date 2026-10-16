#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Inventory/SolidItemType.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidItemTypeTestPrivate
{
	bool IsHexId(const TCHAR* Id)
	{
		const FString Text(Id ? Id : TEXT(""));
		if (Text.Len() != SolidItemType::IdLength)
		{
			return false;
		}
		for (const TCHAR Char : Text)
		{
			if (!FChar::IsHexDigit(Char))
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidItemTypeCatalogTest,
	"SolidCore1.Inventory.ItemTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidItemTypeCatalogTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("four items"), SolidItemType::CatalogCount, 4);
	TestTrue(TEXT("the catalog fits"), SolidItemType::CatalogCount <= SolidItemType::MaxItems);

	const FSolidItemType* Rags = SolidItemType::FindById(SolidItemType::IdRags);
	const FSolidItemType* Helmet = SolidItemType::FindById(SolidItemType::IdDentedHelmet);
	const FSolidItemType* Spear = SolidItemType::FindById(SolidItemType::IdCrudeSpear);
	const FSolidItemType* Torch = SolidItemType::FindById(SolidItemType::IdTorch);
	TestNotNull(TEXT("rags are listed"), Rags);
	TestNotNull(TEXT("the helmet is listed"), Helmet);
	TestNotNull(TEXT("the spear is listed"), Spear);
	TestNotNull(TEXT("the torch is listed"), Torch);
	TestNull(TEXT("an unknown id is empty"), SolidItemType::FindById(TEXT("missing")));
	TestNull(TEXT("an empty id is empty"), SolidItemType::FindById(nullptr));
	TestEqual(TEXT("an unknown name is empty"), FString(SolidItemType::NameFor(TEXT("missing"))), FString(TEXT("")));

	if (!Rags || !Helmet || !Spear || !Torch)
	{
		return false;
	}

	TestEqual(TEXT("rags name"), FString(Rags->Name), FString(TEXT("Rags")));
	TestEqual(TEXT("helmet name"), FString(Helmet->Name), FString(TEXT("Dented helmet")));
	TestEqual(TEXT("spear name"), FString(Spear->Name), FString(TEXT("Crude spear")));
	TestEqual(TEXT("torch name"), FString(Torch->Name), FString(TEXT("Torch")));
	TestEqual(
		TEXT("rags are armor"),
		static_cast<uint8>(Rags->Type),
		static_cast<uint8>(ESolidItemKind::Armor));
	TestEqual(
		TEXT("the helmet is armor"),
		static_cast<uint8>(Helmet->Type),
		static_cast<uint8>(ESolidItemKind::Armor));
	TestEqual(
		TEXT("the spear is a weapon"),
		static_cast<uint8>(Spear->Type),
		static_cast<uint8>(ESolidItemKind::Weapon));
	TestEqual(
		TEXT("the torch is a tool"),
		static_cast<uint8>(Torch->Type),
		static_cast<uint8>(ESolidItemKind::Tool));

	const FSolidItemType* Listed[] = { Rags, Helmet, Spear, Torch };
	for (const FSolidItemType* Item : Listed)
	{
		TestTrue(TEXT("item id is eight hex digits"), SolidItemTypeTestPrivate::IsHexId(Item->Id));
		TestEqual(TEXT("the page name follows the id"), FString(SolidItemType::NameFor(Item->Id)), FString(Item->Name));
	}
	TestTrue(
		TEXT("the four ids differ"),
		FCString::Strcmp(Rags->Id, Helmet->Id) != 0
			&& FCString::Strcmp(Rags->Id, Spear->Id) != 0
			&& FCString::Strcmp(Rags->Id, Torch->Id) != 0
			&& FCString::Strcmp(Helmet->Id, Spear->Id) != 0
			&& FCString::Strcmp(Helmet->Id, Torch->Id) != 0
			&& FCString::Strcmp(Spear->Id, Torch->Id) != 0);

	TestEqual(TEXT("rags are first"), FString(SolidItemType::Catalog[0].Id), FString(SolidItemType::IdRags));
	TestEqual(TEXT("the helmet is second"), FString(SolidItemType::Catalog[1].Id), FString(SolidItemType::IdDentedHelmet));
	TestEqual(TEXT("the spear is third"), FString(SolidItemType::Catalog[2].Id), FString(SolidItemType::IdCrudeSpear));
	TestEqual(TEXT("the torch is fourth"), FString(SolidItemType::Catalog[3].Id), FString(SolidItemType::IdTorch));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
