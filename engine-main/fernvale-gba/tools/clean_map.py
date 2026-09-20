#!/usr/bin/env python3
"""Prepare the supplied crisp map at the game's authored world size."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
source = Image.open(ROOT / "art/map-source.png").convert("RGB")
# Preserve the supplied artwork exactly except for its required GBA world size.
# Nearest-neighbour sampling retains its hard pixel edges.
clean = source.resize((1208, 648), Image.Resampling.NEAREST)
clean.save(ROOT / "art/map-polished.png", optimize=True)
print(f"Prepared supplied map: {clean.width}x{clean.height}.")
