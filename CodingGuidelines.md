# Coding guidelines

Rules for new C++ and for C++ a story is already changing. How the game plays is [`GameVision.md`](GameVision.md). Agent workflow is [`AGENTS.md`](AGENTS.md). The test filter list and how to run the suite stay in [`README.md`](README.md).

## A game concept is a class

A thing the design names is one C++ class, and the class name is that thing. The class holds the data for one of those things and the answers to the questions that thing is asked. A caller asks the object. An answer that needs logic is a method on it.

`FSolidStronghold` and `FSolidBodyPart` are the examples. Each kind is a subclass. A call site asks the object, as it does with `FSolidStronghold::Supported`. Monster is `FSolidMonster`. Where it stands, how tall it is, and the scale of its body are questions that object answers. `SolidMonster::CollectChoices` left those questions as free functions in a namespace. That was the miss.

An enum can stay as a token a map or a save stores. A lookup such as `SolidStronghold::TypeOf` returns the class for that token.

A value that only carries data stays a struct. `FSolidStrongholdView` is a pitch and a yaw. A calculation the design does not name as its own thing stays a function in a namespace. `SolidTerrainNoise` is that kind of calculation. The namespace names the calculation. It does not stand in for a thing the design names, and a test that can call a function without a world does not make that function the concept.

Code that already uses a namespace and a struct for a game concept stays as it is until a story changes that concept. That story moves the concept onto the class. A concept the story is introducing starts as the class.

## Encapsulation and cohesion

Code strives to maximise encapsulation and cohesion. Specialisation and generalisation through C++ classes is the usual way to do that.

Encapsulation keeps a type's data and the operations on that data together. A simple field, with no logic to read or update it, can be public or reached through a method. Either is fine. A field that needs logic to read or to update is reached through a method. The type hides that logic, so a change to it stays inside the type.

Cohesion means the parts of a type all serve the one thing that type is. A stronghold type that holds the stronghold's size, its exit, and the answers about that stronghold is cohesive. A type that also formats the journal and samples the terrain is not.

A general type is the shared concept. A specialised type is one kind of it, and it adds or replaces what is particular to that kind. `FSolidStronghold` is the general company base. `FSolidDungeon` and `FSolidTower` are specialisations of it.

## Plain C++ unless the engine owns it

A game concept the engine does not spawn, tick, or save is a plain C++ class, as `FSolidDungeon` is. It is a `UObject` or a Blueprint when the engine or content has to own it, as `ASolidCharacter` and `USolidCompany` do.

Blueprint classes stay `BP_SolidCharacter` and `BP_SolidGameMode`. A new game concept starts as C++.

## Shape of a class

A function another file calls is declared in the header. That is C++. A method the engine calls is declared there too, such as an override or a `UFUNCTION`. A helper that only its `.cpp` uses stays in that `.cpp`.

The definition stays in the `.cpp`. Defining a function in the header is discouraged. A template is the case where the caller has to see the body. `SolidClickMove::IntersectHeight` is one.

A simple field can be public or reached through a method, as the encapsulation section says.

The destructor is virtual when subclasses exist. A copy that must keep the subclass implements `Clone` and returns `TUniquePtr` to the base, as `FSolidBodyPart::Clone` does.

One concept is one header and one `.cpp`. `FSolidDungeon`, `FSolidFortress`, and `FSolidTower` each have their own pair.

A header in another folder is included by its module path (`Inventory/SolidItemType.h`). A header in the same folder is included by its file name.

## Ownership and references

A pointer says which of two things it is: this object owns that one, or this object is related to that one.

An owning pointer means this object owns that one. The owner creates it and destroys it, and is the only owner. Use `TUniquePtr`. `FSolidBodyMesh::Parts` owns each body part. `Clone` returns a `TUniquePtr` so the caller owns the copy.

A reference means this object is related to that one, and someone else owns it. Use a raw pointer for a plain object. Use `TWeakObjectPtr` for a `UObject` that may go away. `ASolidCompanionCharacter::FollowTarget` is the captain that companion follows. The companion does not own the captain.

A `TObjectPtr` does not say who owns the object. One with `UPROPERTY` can be a subobject the actor owns, such as `ASolidBuilding::BodyMesh`, and the engine tracks that. A `TObjectPtr` used only to find an object has a comment on the declaration that says so.

A raw `new` is not an owning pointer. Ownership is a `TUniquePtr` or an engine subobject.

## Lists and arrays

A runtime list is a `TArray`. Its length is `Num()`. Prefer `TInlineAllocator` sized to the known cap when the list rides on a copied company value such as `FGameState`. Check the cap before `Add`.

A fixed slot, a compile-time table, and a geometric tuple stay a C array. Worn and pack boxes, craft and chest cells, `bFired` per definition index, `Changes` and `Requires` on a constexpr event, recipe `Inputs`, catalogs sized with `UE_ARRAY_COUNT`, and hex corners or triangle vertices are those cases.

## Names

Types start with `Solid`. Unreal's prefixes stay: `F` for a plain class or struct, `A` for an actor, `U` for a `UObject`, `E` for an enum.

A method name is the question or the act, as `Supported` and `CollectGroundHexes` are.

Words come from the design: Captain, Company, Squad, Companion, Stronghold. The word den is not a name in this code.

## Coordinates

Use world coordinates, in centimetres, rather than world-map cells. Convert a cell when the world map is first loaded, and keep the world position from then on. A new field stores that world position. Leave an existing cell field in place until a change is already editing that code.

## Comments

A comment above a function or a method says what it does and why it exists. It leaves out the steps inside the body.

A comment above a type says what that type is, in a sentence or two. A comment on a field says the unit, or which end of a range, when the name leaves that open.

## Automated tests

UE Automation tests live under `Source/SolidCore1/Tests/` (editor builds, `WITH_DEV_AUTOMATION_TESTS`).

### Policy

1. Existing tests keep passing when product code changes. The product change does not edit those tests to make itself pass.
2. When a change has to update an existing test (an API rename, or an intentional behavior change), the commit names those tests and says why. Read a constant the product already defines, such as `USolidWorldChart::MapWidth`, instead of copying a number.
3. New product code ships with tests in the same commit. Prefer a pure-logic test or a `NewObject` unit test. Add a PIE test only when the behavior needs a running world.
4. Never pass `TObjectPtr<T>` (or a ternary that yields one) to `TestNotNull` / `TestNull`. UE 5.8 cannot deduce `const ValueType*` from it. `GetStaticMesh()` is the usual case. Bind a raw `T*` first, or call `.Get()`.
5. The Windows agent runs the full suite, filter `SolidCore1`, with `tools\test.bat` before it pushes a product change. A change in one area can break a test elsewhere. A cloud agent does not run that suite, as `AGENTS.md` says.
6. A test does not assert the biomes of the shipped world. The chart is painted by hand, and a test that names those cells fails when the paint changes. A behavior that reads biomes uses a chart the test builds. A shipped chart test may check that the asset loads, and where a mark or a road end sits.

An earlier rule shipped a test change as its own buildable tip before the product change, so the tests could run against the current build. That extra landing is too slow. The test change and the product change ship together.
