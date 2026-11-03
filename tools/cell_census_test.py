#!/usr/bin/env python3
"""Cell census. No Unreal. Run: python -m unittest tools/cell_census_test.py"""

from __future__ import annotations

import unittest

from tools.cell_census import (
    HEX,
    LAIR,
    OTHER,
    WORLD,
    accept_span,
    apply_deliverable_corrections,
    classify,
)


class CellCensusTests(unittest.TestCase):
    def test_whole_word_and_camel_segment(self) -> None:
        self.assertIsNone(accept_span("cancelled", 3))
        self.assertIsNone(accept_span("excellent", 3))
        self.assertEqual(accept_span("a cell here", 2), "cell")
        self.assertEqual(accept_span("two cells", 4), "cells")
        self.assertEqual(accept_span("CellX", 0), "CellX")
        self.assertEqual(accept_span("StrongholdCell", 10), "StrongholdCell")

    def test_known_categories(self) -> None:
        self.assertEqual(
            classify("Source/SolidCore1/Inventory/SolidInventory.h", "int32 PackCell;", "PackCell", 1)[0],
            OTHER,
        )
        self.assertEqual(
            classify("Source/SolidCore1/Fog/SolidTerrainFog.cpp", "int32 CellX", "CellX", 1)[0],
            OTHER,
        )
        self.assertEqual(
            classify("Source/SolidCore1/Strongholds/SolidArrangeSign.h", "CellCm = 2.4f", "CellCm", 1)[0],
            OTHER,
        )
        self.assertEqual(
            classify("Source/SolidCore1/Lairs/SolidLair.h", "CellCm = 200.f", "CellCm", 1)[0],
            LAIR,
        )
        self.assertEqual(
            classify("Source/SolidCore1/Strongholds/SolidStrongholdGrid.h", "struct FCell", "FCell", 1)[0],
            HEX,
        )
        self.assertEqual(
            classify("Source/SolidCore1/Strongholds/SolidStronghold.h", "FIntPoint Cell", "Cell", 1)[0],
            WORLD,
        )

    def test_town_sign_columns_stay_on_the_chart(self) -> None:
        groups = [
            {
                "file": "Source/SolidCore1/Tests/Towns/SolidTownSignTests.cpp",
                "token": "CellX",
                "category": OTHER,
                "note": "",
                "line": 1,
            }
        ]
        apply_deliverable_corrections(groups)
        self.assertEqual(groups[0]["category"], WORLD)


if __name__ == "__main__":
    unittest.main()
