# Coding guidelines

Rules for new C++ and for C++ a story is already changing. How the game plays is [`GameVision.md`](GameVision.md). Agent workflow is [`AGENTS.md`](AGENTS.md). Tests follow the Automated tests policy in [`README.md`](README.md).

## A game concept is a class

A thing the design names is a C++ class. The class holds the data for one of those things and the answers to the questions that thing is asked.

The company base is the model. `FSolidStronghold` is the base. `FSolidDungeon`, `FSolidFortress`, and `FSolidTower` are the kinds. A body part is the other model. `FSolidBodyPart` is the base, and each part (a thigh, the head, a hand, and the rest) is its own subclass.

An item, a recipe, an event, a battle plan, a companion, a town, and a journal line are game concepts in the same way. New code gives each of them a class.

Code that already uses a namespace and a struct for one of those stays as it is until a story changes that concept. The story that adds behavior moves the concept onto a class. A pass whose only job is that move is its own story.

## What stays a struct or a function

A value that only carries data stays a struct. A hex, a view angle, a vertex, a triangle, and a slider hit are values.

A calculation that is not a thing in the game stays a function in a namespace. Terrain noise, a ground step, and fog sampling are calculations.

A catalog of constants can stay a namespace while it only stores ids and looks them up. When that concept starts answering questions that differ by kind, those questions move onto the class.

## One question, one method

Kinds of the same concept answer the same questions through the base class. `Supported`, `UsesCliffMouth`, `Trip`, and `CollectGroundHexes` are those questions for a stronghold. `Name`, `Fit`, and `Draw` are those questions for a body part.

A call site asks the object. A new switch on the enum is the wrong place for a question the class answers.

The enum can stay. The map, a save, and a journal line still need a token. `TypeOf`, or the lookup that concept already uses, returns the class for that token.

## Plain C++ unless the engine owns it

A game concept the engine does not spawn, tick, or save is a plain C++ class: `FSolidDungeon`, `FSolidBodyPart`. It is a `UObject` or a Blueprint only when content or the engine has to own it.

The engine owns the type when it has to spawn it, tick it, or serialize it. That type is an actor or a `UObject`: `ASolidCharacter`, `USolidCompany`, `ASolidBuilding`.

Blueprint classes stay the ones content already tunes: `BP_SolidCharacter` and `BP_SolidGameMode`. A new game concept starts as C++.

## Shape of a class

The header shows the shape. The `.cpp` holds a method body that is more than a few lines.

Facts the object is are public fields. The stronghold's type, size, and map cell are fields. Questions it answers are `const` methods.

A virtual method has a default that means "not this kind" when that default is honest. `Supported` returns false. `CollectGroundHexes` clears the list.

A copy that must keep the subclass implements `Clone` and returns `TUniquePtr` to the base. The caller owns that copy.

The destructor is virtual. Copy and move stay available when the fields allow it, as they do on `FSolidStronghold` and `FSolidBodyPart`.

One concept is one header and one `.cpp`. Subclasses of one base may share that pair when they are small, as the three stronghold types share `SolidStrongholdType.h`.

A header in another folder is included by its module path (`Inventory/SolidItemType.h`). A header in the same folder is included by its file name.

## Names

Types start with `Solid`. Unreal's prefixes stay: `F` for a plain class or struct, `A` for an actor, `U` for a `UObject`, `E` for an enum.

The folder is the concept: `Strongholds/`, `Inventory/`, `Character/`, `Squad/`.

A method name is the question or the act: `Supported`, `UsesCliffMouth`, `CollectGroundHexes`.

Words come from the design: Captain, Company, Squad, Companion, Stronghold. The first stronghold is the cave. The word den is not a name in this code.

## Comments

A comment above a type says what it is, in game words, in a sentence or two. A comment on a field says the unit, or which end of a range, when the name leaves that open. A comment does not restate the line under it.

## Tests

A new class ships with tests in the same commit. The tests ask the class the questions it exists to answer. Stronghold tests ask `TypeOf` and `Supported`. The Automated tests policy in [`README.md`](README.md) is the rest of the rule.
