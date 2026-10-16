#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Crafting/SolidRecipe.h"
#include "Inventory/SolidItemType.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCraftingTorchRecipeTest,
	"SolidCore1.Crafting.Torch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCraftingTorchRecipeTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("the list has a recipe"), SolidCrafting::RecipeCount >= 1);
	TestTrue(TEXT("the list fits"), SolidCrafting::RecipeCount <= SolidCrafting::MaxRecipes);

	const FSolidRecipe* Torch = SolidCrafting::FindByName(TEXT("Torch"));
	TestNotNull(TEXT("the torch recipe is listed"), Torch);
	TestNull(TEXT("an unknown recipe is missing"), SolidCrafting::FindByName(TEXT("missing")));
	TestNull(TEXT("an empty name is missing"), SolidCrafting::FindByName(nullptr));
	TestEqual(TEXT("the torch recipe is first"), FString(SolidCrafting::Recipes[0].Name), FString(TEXT("Torch")));
	if (!Torch)
	{
		return false;
	}

	TestEqual(TEXT("torch recipe name"), FString(Torch->Name), FString(TEXT("Torch")));
	TestTrue(TEXT("the player knows the torch recipe"), Torch->bKnown);
	TestEqual(TEXT("the torch recipe makes a torch"), FString(Torch->OutputId), FString(SolidItemType::IdTorch));
	const FSolidItemType* Output = SolidItemType::FindById(Torch->OutputId);
	TestNotNull(TEXT("the output is an item type"), Output);
	if (!Output)
	{
		return false;
	}
	TestEqual(TEXT("the output is named Torch"), FString(Output->Name), FString(TEXT("Torch")));
	TestEqual(
		TEXT("the output is a tool"),
		static_cast<uint8>(Output->Type),
		static_cast<uint8>(ESolidItemKind::Tool));

	TestEqual(TEXT("the torch recipe takes two inputs"), Torch->InputCount, 2);
	TestTrue(TEXT("the input list holds those two"), Torch->InputCount <= SolidCrafting::MaxInputs);
	TestEqual(TEXT("the first input is a stick"), FString(Torch->Inputs[0].ItemId), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the torch recipe takes one stick"), Torch->Inputs[0].Count, 1);
	TestEqual(TEXT("the second input is pitch"), FString(Torch->Inputs[1].ItemId), FString(SolidItemType::IdPitch));
	TestEqual(TEXT("the torch recipe takes one pitch"), Torch->Inputs[1].Count, 1);
	TestNotNull(TEXT("the stick is an item type"), SolidItemType::FindById(Torch->Inputs[0].ItemId));
	TestNotNull(TEXT("the pitch is an item type"), SolidItemType::FindById(Torch->Inputs[1].ItemId));
	for (int32 Index = Torch->InputCount; Index < SolidCrafting::MaxInputs; ++Index)
	{
		TestTrue(TEXT("an unused input is empty"), Torch->Inputs[Index].ItemId == nullptr);
		TestEqual(TEXT("an unused input has no count"), Torch->Inputs[Index].Count, 0);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
