# Solid Core

This file is the design north star. It describes how the game plays. The world is described in [`Content/WorldLore.md`](Content/WorldLore.md). One-off work lives in [`Planning`](Planning).

## The game

Solid Core is an open-world real-time strategy role-playing game set in a medieval fantasy world.

## The company

Each player has one Captain. The Captain leads a Company of Companions from a single Stronghold. The player forms a Squad from that Company and takes the Squad out to explore and to fight. Companions who are not in the Squad stay at the Stronghold.

The Company keeps one Stronghold. The player can raise it to a larger size or change it to another type.

## Views

The game has a first-person view and a third-person view. The player can switch between them in most situations. Game events turn particular views on and off.

First person carries the role-playing. Third person carries the strategy.

## Combat

Combat is won by preparation. The Squad's members, their equipment, and the battle plan in use decide the outcome.

During a fight the player moves the Captain and switches among the battle plans available to the Squad. A Squad has up to eight battle plans.

## Who the player fights

The game is for most age groups. Blood and gore stay minimal by default.

The Captain and the Companions are human. Human and humanoid NPCs may fight alongside them. Every opponent is a monster, and a monster is a reptile or an insect. A player-versus-player mode, if one is added, is where a player can fight a human or a humanoid.

Parental controls may arrive later, especially if a player-versus-player mode is added.

## Death

Death leaves a mark on the Captain and on Companions. The player can reduce that mark. Some consequence always remains. The exact mark that remains will be defined as development continues.

When the Captain dies, magic returns them to the Stronghold. The body and the gear stay where the Captain died.

## Looks and function

How a thing works and how it looks are separate choices, as far as the game can keep them separate. The player chooses the function of armour, weapons, and similar gear, and chooses the appearance on its own.

## Early work, later relief

The early game asks for a lot of work by hand. That work is deliberate. Progress unlocks ways to do less of it, and that relief is the reward.

Early work includes foraging by hand, crafting by hand, walking back to recover the gear left on a body, and making extra journeys because storage is limited.

Later unlocks include automatic pickup, storage that does not run out, and Stronghold characters who handle crafting.

## Hidden numbers

A numeric model runs every part of the game, including weapon damage. Those numbers stay off the screen. Each value is drawn from a range, so repeated play still leaves the exact figure unknown.

The player sees qualitative grades. Item quality is `ESolidItemQuality`.

## The journal

A journal records every significant event, so the player can look back over their progress.

## Many players

Multiplayer will have its own vision. The shape so far is this.

It starts as many Companies on one server. A later cooperative mode can place other players' Captains in a Squad alongside Companions. A player-versus-player mode may follow.

When a server is part of the game, it measures play, sets targets from those measurements, and makes small automatic adjustments toward those targets.
