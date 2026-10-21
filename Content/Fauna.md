# Overview

There are 3 main phyla/classes of fauna in the game...

- Human
- Suarobestia (dinosaurs & dragons)
- Arthrobesia (giant insects, spiders, etc)

All characters and monsters in the game should be procedurally generated from one of these three core body types.

# Humans

Humans are generated based on a set of core numeric parameters. Each parameter can have any value between 0.0 and 1.0, but we apply labels to the extremes (and often centre) for convenience:-

- build: dwarven - common - elfin
- height: short - average - tall (combines with build for final character height)
- gender: feminine - androgenous - masculine
- age: youthful - mature - aged
- skintone: dark to light



# Suarobestia

Use the Velociraptor skeleton & animations from PBR Velociraptor (Animated), https://www.fab.com/listings/5d5fa3d3-4459-4677-be46-c686f733908c, at /Game/Fab/PBR_Velociraptor__Animated_. Flying creatures don't actually need to fly in-game at first, so we can just add folded wings to the base mesh.

The downloaded mesh is the body for now. Its standing height is 100 cm, a uniform scale of that mesh, and the feet sit on the ground. Eventually a Velociraptor is built the way a human mesh is built, and the way the Character Editor changes one: the same scaling and deforming procedural mesh generation, on this skeleton, so the clips still play.

Main parameters are...

- size: tiny to immense
- winged: true/false
- colour

# Arthrobestia

Core parameters...

- size: tiny to immense
- segments: integer (number of body segments, 3 for insects, 4 for spiders, 10 for centipedes)
- wings: (list of integers specifying which body segments have wings)
- colour

The model will be Head > Thorax (specified number of segments) > Abdomen.