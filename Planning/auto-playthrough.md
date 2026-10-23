# Auto playthrough

Status: New

## Story

The player opens the F10 menu and chooses Auto playthrough. That starts AutoPlaythrough, which plays the opening of the game from a script. The player watches. The script is a list of steps, and a later story can change the list. This story ships the first list.

The row can be chosen at the very start of a new game, while the captain is still in Iglin and the playthrough has not been started. Choosing it starts the script and dims the row for the rest of that game. Moving away from Iglin dims it too, when it was not already dim. A playthrough that has already started keeps running when the script walks him out.

AutoPlaythrough is the game concept this story introduces. It is the class `FSolidAutoPlaythrough`.

The first script starts from a new game, with the captain in Iglin.

He walks from Iglin onto a ramp of the Iglin bridge. The monsters kill him: "You are killed by monsters" shows, he closes it, and he wakes in the cave. He builds a torch from the chest and puts it in his main hand. He leaves the cave and walks back to Iglin. Iglin casts him out. He closes "You are driven from the village" and walks to the Iglin tunnels. He passes through them and leaves by the far mouth. He walks to Relion, where Sam joins and he is in raven sight. He walks to Kanfold, where Alex joins. He walks to Dragongate, where the company gains Max, Morgan, and Quinn and the squad cap becomes the whole company. He walks to the Imperious stronghold site and constructs an Imperious Tower. He climbs to the top floor, returns to the ground floor, and leaves. Build squad is open, and he selects Sam, Alex, Max, Morgan, and Quinn.

## Stages

Agree this order before a stage is planned in detail. A stage is played before the next stage is planned. The story stays In Progress until the whole script has been played and agreed.

1. **The runner and the death.** The menu row, including the dim row once the playthrough has started or the captain has left Iglin, AutoPlaythrough, and a script that can walk, close a message, and stop. The steps are the walk from Iglin to the bridge, the monster message, and the wake in the cave.
2. **The torch and the village.** Craft the torch, put it in the main hand, leave the cave, close Build squad, walk to Iglin, and close the driven-out message.
3. **The tunnels and the company.** Through the tunnels, then Relion, Kanfold, and Dragongate. This stage waits until [Events add a companion](events-add-a-companion.md) is Done. Relion already adds Sam. Kanfold's Alex and Dragongate's Max, Morgan, and Quinn are that story.
4. **The tower and the squad.** The Imperious site, the climb to the top floor, the leave, and selecting the squad.

## Acceptance

Choosing Auto playthrough on a new game runs the script to the end.

- The F10 rows, from the top, are Use portal, Journal, Build squad, Character editor, Auto playthrough, Credits, and Exit Game. Credits stays the row before Exit Game. The new row reads **Auto playthrough**.
- At the start of a new game, while the captain is still in Iglin and the playthrough has not been started, the row can be chosen. Choosing it closes the menu, starts the script, and dims the row.
- The row stays dim for the rest of that game after the playthrough has been started. Stopping the script, including with Esc while he is still in Iglin, leaves the row dim. Confirming the dim row does nothing, and the menu stays open.
- Once the captain moves away from Iglin without having started the playthrough, the row stays dim for the rest of that game. Walking back into Iglin leaves the row dim.
- A playthrough that has already started keeps running when the script walks him out of Iglin. Leaving Iglin is a step, and it is not a stop.
- If he makes no progress toward the current point for 4 seconds, the playthrough stops where he is. The HUD line goes, and the menu stays closed.
- The HUD shows `Auto playthrough` while the script runs. The line goes when the script finishes, and when the player stops it.
- The player's Esc stops the playthrough. The captain stays where he is, and the menu stays closed. The player's F10 stops it and does not open the menu. After the playthrough has stopped, F10 opens the menu as before.
- While the script runs, the player's move, look, and clicks leave the captain on the script. The script walks in true sight, and clicks the ground in raven sight.
- The script walks. It leaves the portal list alone.
- From Iglin he walks onto an Iglin ramp of the Iglin–Relion bridge. The deck and the gap past the lip do not count. "You are killed by monsters" shows. He closes it. He is inside the cave, with Rags on his body and empty hands.
- In the cave he takes one stick and one pitch from the chest, crafts one torch, and puts that torch in the main hand. The rusty sword, the old boots, and the coins stay in the chest. Rags stay on the body.
- He leaves by the dark opening. Build squad opens. He closes it with the squad and the plans as they were, and walks to Iglin.
- Entering Iglin shows "You are driven from the village". He closes it. The Iglin tunnels are open. He enters at the X mouth, follows the straight run, and leaves at the Y mouth.
- He walks to Relion. Sam is in the company and in the squad. The captain is in raven sight, and he stays in raven sight.
- He walks to Kanfold. Alex is in the company and in the squad, beside Sam.
- He walks from Kanfold to Dragongate by way of Relion. Enter Relion stays as it was. At Dragongate, Max, Morgan, and Quinn are in the company. The squad is still Sam and Alex. The squad cap is the company.
- He walks to the stronghold site at row 16, column 36, clicks the stone, and chooses Imperious Tower. He arrives inside, on the ground floor.
- He climbs the stair to floor 10, walks back to the door on floor 1, and leaves onto the grass outside that door.
- Build squad is open on that grass. He places Max, then Morgan, then Quinn into the empty squad boxes. Sam and Alex stay in their boxes. He closes the page. The squad is Sam, Alex, Max, Morgan, and Quinn. The playthrough has stopped, and the HUD line is gone.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. This plan commit does not.

## Implementation notes

- `FSolidAutoPlaythrough` is a plain C++ class in `Playthrough/SolidAutoPlaythrough.h`. A step is a value on that class. The first script is the list in the story. A later story edits that list and leaves the class.
- The game instance holds the object. `OpenLevel` destroys the game mode on the cave trip, the tunnel trip, and the tower trip. The company and the play state survive on the instance. The new mode steps the same object. A script kept only on the mode would vanish when he wakes in the cave.
- One flag on the play state dims the row. Choosing the row sets it, and so does `OccupiedTown` leaving Iglin, including a portal. Spawn sets `OccupiedTown` from none to Iglin and does not set the flag. The flag stays set. The running script does not treat the departure as a stop. The row uses the same dim drawing as Build squad, and confirm does nothing.
- The row goes before Credits, as [Reorder the game menu](Done/reorder-game-menu.md) requires. Key 5 confirms Auto playthrough, key 6 confirms Credits, and key 7 confirms Exit Game. Key 8 has no row. `SolidCore1.MainMenu.Order` already allows a row before Credits and already checks key 8. `SolidCore1.Exit` already requires Credits immediately before Exit Game. Those tests stay. The product commit adds the new label and its action.
- The player's Esc and F10 are the stop. The script calls `CloseMessage`, the inventory close, and the squad-page close. It does not send Esc or F10. After a start, the dim row does not start the script again.
- He walks at his normal speed, 500 cm/s. True sight uses `AddMovementInput`. Raven sight sets the click-move destination. There is no navigation mesh. Follow a road one cell at a time, from `SolidRoad`. A straight line across the map stops on a tree or a building. If the distance to the current point does not fall for 4 seconds, stop where he is and clear the HUD line. A cell is 8 m, so 4 seconds is more than twice a clear step.
- Iglin to the ramp is town and grass. Stop on an Iglin ramp. The broken deck stays closed, and this script does not cross it. The cave trail, the grass in front of each tunnel mouth, Relion, the Kanfold highway, the Dragongate road, and the grass south of Dragongate to the site are the other walks.
- The death step is the shipped message. Standing on the ramp shows it, and `CloseMessage` fires Killed by monsters. `PresentEventResult` enters the stronghold. [Iglin Bridge Fight Enhancements](iglin-bridge-fight-enhancements.md) replaces that message with the raptor fight. When that story is Done, this script's death step changes to walking into that fight and waiting until he wakes in the cave. This story leaves that step on the message.
- The Kanfold walk back to Dragongate follows the highway to Relion and the road on to Dragongate. Stepping onto Relion again leaves Enter Relion fired once.
- The site line is `! 16, 36 = StrongholdSite (Imperious)`. The choice row is `Imperious Tower`. Tower is the built type at that size. Call `OfferStrongholdSite` and `ConfirmStrongholdChoice` once he is at the stone.
- The tower is one interior. Floor 1 is the ground and floor 10 is the top. The floor pitch is 300 cm and the riser count is 17, so a riser is about 17.6 cm. The captain's step height stays at the engine default, 45 cm, the flight is 140 cm wide, and the capsule radius is 42 cm. The script walks points along that flight from the tower plan, up to floor 10 and back to the door. One destination on the top floor does not climb the spiral. The script does not travel from floor to floor.
- Inside the cave and the tunnels the open-world footing check is off. The cave walk uses the Meagre floor. The tunnels follow the straight run from the X mouth to the Y mouth.
- Killed by monsters enables Attack and Defend. Those plans sit outside the five the company starts with, so leaving the cave opens Build squad. The script closes that page and changes nothing on it.
- The torch recipe is one stick and one pitch. The chest starts with five sticks and three pitch. The script opens the inventory, calls `SolidInventory::Move` and `Craft`, puts the torch in the main hand, and closes the page. A torch in the hand lights the tunnels. On the last squad page it calls `SolidSquadBuilder::Drop` for Max, then Morgan, then Quinn, and closes the page. The pages are on screen. The script does not drag with the pointer.
- A test that needs no world reads the step names in order, the menu row, and the flag that dims that row after a start and after leaving Iglin. The walk is what PIE shows.
- The detailed plan for stage 1 is the next plan commit, after this staging is agreed. That commit stays on this file.
- Do not use the word den.
