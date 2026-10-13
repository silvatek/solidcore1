# Overview

There are 3 main phyla/classes of fauna in the game...

- Human
- Suarobestia (dinosaurs & dragons)
- Arthrobesia (giant insects, spiders, etc)

All characters and monsters in the game should be procedurally generated from one of these three core body types.

# Humans

Humans are generated based on a set of core numeric parameters. Each parameter can have any value between 0 and 1, but we apply labels to the extremes (and often centre) for convenience:-

- build: dwarven - common - elfin
- height: short - average - tall (combines with build for final character height)
- gender: feminine - androgenous - masculine
- age: youthful - mature - aged
- skintone: dark to light

# Suarobestia

Use the Allosaurus skeleton & animations. Flying creatures don't actually need to fly in-game at first, so we can just add folded wings to the base mesh.

Main parameters are...

- size: tiny to immense
- winged: true/false
- colour

# Arthrobesia

Core parameters...

- size: tiny to immense
- segments: integer (number of body segments, 3 for insects, 4 for spiders, 10 for centipedes)
- wings: (list of integers specifying which body segments have wings)
- colour

The model will be Head > Thorax (specified number of segments) > Abdomen.