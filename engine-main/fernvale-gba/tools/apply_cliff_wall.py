#!/usr/bin/env python3
"""Apply the approved continuous-cliff mockup to the full-resolution map."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
WORLD_W, WORLD_H = 1208, 648

source_path = ROOT / "art/map-source.png"
approved_path = ROOT / "art/cliff-wall-approved.png"
source = Image.open(source_path).convert("RGB")
approved = Image.open(approved_path).convert("RGB")

# The approved image depicts this exact world-space crop. Only the central
# cliff join is pasted, leaving all unrelated generated pixels out of the map.
crop_world = (330, 120, 630, 340)
join_world = (436, 176, 491, 271)

def sx(x):
    return round(x * source.width / WORLD_W)

def sy(y):
    return round(y * source.height / WORLD_H)

crop_box = tuple((sx(v) if i % 2 == 0 else sy(v))
                 for i, v in enumerate(crop_world))
join_box = tuple((sx(v) if i % 2 == 0 else sy(v))
                 for i, v in enumerate(join_world))

approved = approved.resize((crop_box[2] - crop_box[0],
                            crop_box[3] - crop_box[1]),
                           Image.Resampling.LANCZOS)

local = (join_box[0] - crop_box[0], join_box[1] - crop_box[1],
         join_box[2] - crop_box[0], join_box[3] - crop_box[1])
source.paste(approved.crop(local), join_box[:2])
source.save(source_path, optimize=True)
print("Applied approved continuous rock wall to map source.")
