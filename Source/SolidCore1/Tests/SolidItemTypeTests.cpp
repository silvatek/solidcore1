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
	TestTrue(TEXT("the catalog lists the known items"), SolidItemType::CatalogCount >= 4);
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
		TEXT("the spear is a weapon"),
		static_cast<uint8>(Spear->Type),
		static_cast<uint8>(ESolidItemKind::Weapon));
	TestEqual(
		TEXT("the torch is a tool"),
		static_cast<uint8>(Torch->Type),
		static_cast<uint8>(ESolidItemKind::Tool));
	TestEqual(
		TEXT("the torch is common"),
		static_cast<uint8>(Torch->Quality),
		static_cast<uint8>(ESolidItemQuality::Common));
	TestEqual(TEXT("the torch stacks to one"), Torch->MaxStacked, 1);

	const ESolidItemQuality Grades[] = {
		ESolidItemQuality::Inferior,
		ESolidItemQuality::Common,
		ESolidItemQuality::Superior,
		ESolidItemQuality::Exquisite,
		ESolidItemQuality::Legendary,
		ESolidItemQuality::NA,
	};
	TestEqual(TEXT("six grades"), static_cast<int32>(UE_ARRAY_COUNT(Grades)), 6);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Grades); ++Index)
	{
		for (int32 Other = Index + 1; Other < UE_ARRAY_COUNT(Grades); ++Other)
		{
			TestTrue(TEXT("grades differ"), Grades[Index] != Grades[Other]);
		}
	}
	const TCHAR* InferiorIds[] = {
		SolidItemType::IdRags,
		SolidItemType::IdDentedHelmet,
		SolidItemType::IdCrudeSpear,
		SolidItemType::IdRustySword,
	};
	for (const TCHAR* Id : InferiorIds)
	{
		const FSolidItemType* Item = SolidItemType::FindById(Id);
		TestNotNull(TEXT("an inferior item is listed"), Item);
		if (!Item)
		{
			return false;
		}
		TestEqual(
			TEXT("the item is inferior"),
			static_cast<uint8>(Item->Quality),
			static_cast<uint8>(ESolidItemQuality::Inferior));
		TestEqual(TEXT("the item stacks to one"), Item->MaxStacked, 1);
	}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidItemTypeBootsAndCoinsTest,
	"SolidCore1.Inventory.BootsAndCoins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidItemTypeBootsAndCoinsTest::RunTest(const FString& Parameters)
{
	const FSolidItemType* Boots = SolidItemType::FindById(SolidItemType::IdOldBoots);
	const FSolidItemType* Coins = SolidItemType::FindById(SolidItemType::IdCoins);
	TestNotNull(TEXT("old boots are listed"), Boots);
	TestNotNull(TEXT("coins are listed"), Coins);
	if (!Boots || !Coins)
	{
		return false;
	}

	TestEqual(TEXT("old boots name"), FString(Boots->Name), FString(TEXT("Old boots")));
	TestEqual(
		TEXT("old boots are boots"),
		static_cast<uint8>(Boots->Type),
		static_cast<uint8>(ESolidItemKind::Boots));
	TestEqual(
		TEXT("old boots are inferior"),
		static_cast<uint8>(Boots->Quality),
		static_cast<uint8>(ESolidItemQuality::Inferior));
	TestEqual(TEXT("old boots stack to one"), Boots->MaxStacked, 1);

	TestEqual(TEXT("coins name"), FString(Coins->Name), FString(TEXT("Coins")));
	TestEqual(
		TEXT("coins are a material"),
		static_cast<uint8>(Coins->Type),
		static_cast<uint8>(ESolidItemKind::Material));
	TestEqual(
		TEXT("coins have no grade"),
		static_cast<uint8>(Coins->Quality),
		static_cast<uint8>(ESolidItemQuality::NA));
	TestEqual(TEXT("coins stack to 999"), Coins->MaxStacked, 999);
	TestTrue(TEXT("old boots id is eight hex digits"), SolidItemTypeTestPrivate::IsHexId(Boots->Id));
	TestTrue(TEXT("coins id is eight hex digits"), SolidItemTypeTestPrivate::IsHexId(Coins->Id));
	TestTrue(TEXT("the new ids differ"), FCString::Strcmp(Boots->Id, Coins->Id) != 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
