#pragma once

#include "CoreMinimal.h"
#include "Inventory/SolidItemType.h"

/** The recipes a player can learn. The list is the ones defined so far. */
namespace SolidCrafting
{
	/** How many materials one recipe can list. */
	inline constexpr int32 MaxInputs = 4;
	inline constexpr int32 MaxRecipes = 16;
}

/**
 * One material a recipe consumes.
 * ItemId is an item-type id. Count is how many of that item the recipe takes.
 */
struct FSolidRecipeInput
{
	const TCHAR* ItemId = nullptr;
	int32 Count = 0;
};

/**
 * One way to make an item.
 * Name is the recipe. OutputId is the item type it makes, one of that item.
 * Inputs are the materials and their counts. bKnown is whether the player has the recipe yet.
 */
struct FSolidRecipe
{
	const TCHAR* Name = TEXT("");
	const TCHAR* OutputId = nullptr;
	FSolidRecipeInput Inputs[SolidCrafting::MaxInputs] = {};
	int32 InputCount = 0;
	bool bKnown = false;
};

namespace SolidCrafting
{
	inline constexpr FSolidRecipe Recipes[] = {
		{
			TEXT("Torch"),
			SolidItemType::IdTorch,
			{
				{ SolidItemType::IdStick, 1 },
				{ SolidItemType::IdPitch, 1 },
			},
			2,
			true,
		},
	};

	static_assert(UE_ARRAY_COUNT(Recipes) <= MaxRecipes, "Raise SolidCrafting::MaxRecipes");
	static_assert(Recipes[0].InputCount <= MaxInputs, "A recipe lists more inputs than it can hold");

	inline constexpr int32 RecipeCount = UE_ARRAY_COUNT(Recipes);

	inline const FSolidRecipe* FindByName(const TCHAR* Name)
	{
		if (!Name || !Name[0])
		{
			return nullptr;
		}
		for (int32 Index = 0; Index < RecipeCount; ++Index)
		{
			if (FCString::Strcmp(Recipes[Index].Name, Name) == 0)
			{
				return &Recipes[Index];
			}
		}
		return nullptr;
	}
}
