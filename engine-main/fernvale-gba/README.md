# Fernvale: mountain-and-city edition for Game Boy Advance

This version uses the supplied mountain village and city map.
It preserves the original 16x32 trainer, animation frames and movement speeds.
Open `fernvale.gba` in a GBA emulator. This is a standalone native GBA ROM,
not a patch for another game. No other ROM or save is needed.

The game opens with a looping animated drive toward Pittsburgh. The supplied
neon skyline pans behind the car, the wheel hubs animate, road lights streak past,
and the car settles into its cruising position. Press Start at the
`PRESS START TO PLAY` prompt; the scene fades to black and then fades into the
playable world at the central city intersection.

## Controls

| GBA input | Action |
| --- | --- |
| D-pad | Walk, one direction at a time |
| Hold B | Run |
| Start | Begin from the opening; pause / show controls / resume in-game |
| Select | Return to the central road when unpaused |
| Up at red-roof door | Enter the top-left house |
| Up at blue glass door | Enter the city gym |
| Down at interior door | Leave the house |
| A | Reserved; it does not skip the opening |

Spawn is at the central city intersection at (815,330). Streets, paths, grass,
both river bridges and open clearings are walkable. A continuous north-south
rock cliff intentionally separates the western yellow path from the stone plaza.
Buildings, cliffs, forest, ponds, water and major fences/scenery have
hand-authored collision. The top-left red-roof house opens into an original
retro room with a bed, rug, bookcase, writing desk, plant and working furniture
collision. The people and signs pictured in the supplied image are static
artwork. The blue glass building opens into the supplied recreation-center
interior with a weight bench, treadmill, exercise frame and matching equipment
collision. There is no NPC
dialogue, music, battles or in-game saving in this edition.

## Build and edit in VS Code

Extract the entire folder and open it in VS Code. With Python 3 and devkitARM
installed, run `python3 build.py`. The helper also accepts an ARM bare-metal
GCC toolchain on PATH or the `GBA_TOOLCHAIN` environment variable pointing to
its bin directory. No C standard library or desktop graphics framework is used.

| File | Purpose |
| --- | --- |
| source/city_main.c | Active game: movement, camera, collision, renderer, pause |
| tools/make_city.py | Convert image and edit collision polygons/rectangles |
| art/map-source.png | Supplied crisp mountain village and city artwork |
| tools/clean_map.py | Resizes the supplied art with hard pixel sampling |
| art/map-polished.png | GBA-scale map used by the converter |
| art/city-map.png | Palette-converted full map |
| art/city-collision.png | White means solid, black means walkable |
| art/red-house-interior-source.png | Original high-resolution room artwork |
| art/red-house-interior.png | GBA-ready 240x160 interior |
| art/red-house-interior-collision.png | Interior collision preview |
| tools/make_interior.py | Converts the room and authors its collision |
| art/recCenterInterior.jpg | Supplied high-resolution recreation-center artwork |
| art/glass-gym-interior.png | GBA-ready 240x160 gym |
| art/glass-gym-collision.png | Gym collision preview |
| tools/make_gym.py | Converts the gym and authors its collision |
| art/cliff-wall-approved.png | Approved continuous-cliff visual reference |
| art/steelhacks-pittsburgh.png | Supplied neon Pittsburgh skyline |
| art/car-sprite-original.jpg | Original supplied orange car drawing |
| art/driving-car-source.png | Clean transparent redraw of the supplied orange car |
| art/driving-car.png | Four-frame hard-edged hub-animation sheet |
| art/pittsburgh-drive.png | GBA-ready 320x160 scrolling panorama |
| tools/make_title.py | Builds the shared palette, panorama and car frames |
| tools/apply_cliff_wall.py | Applies the approved cliff connection to the map source |
| source/city.s | Embeds generated binary map data in ROM |
| source/city_palette.h | Generated GBA palette and dimensions |
| tools/trainer_art.py | Original character drawings |
| source/assets.c | Existing trainer and UI font data |
| source/main.c | Previous village implementation, retained but not compiled |
| build.py | Build, cartridge header, checksum, power-of-two padding |

Generated map data is included. After editing the collision or source images,
install Pillow, run `python3 tools/clean_map.py`, `python3 tools/make_city.py`,
`python3 tools/make_title.py`, `python3 tools/make_interior.py` and
`python3 tools/make_gym.py`, then run `python3 build.py`.
The old village generator and reference art remain available but are not the
active city map.

## GBA rendering

The source image is converted to a 1208x648 scrolling world and 252 colors for
mode 4. Two native 240x160
framebuffers alternate each frame; DMA3 copies visible rows from ROM into the
hidden buffer, and the screen switches during VBlank. The original trainer
uses a hardware sprite. Horizontal camera scrolling uses two-pixel increments
for aligned transfers; the player still moves at the original subpixel speeds.
The opening uses the same double-buffered mode-4 renderer. It streams a moving
240-pixel window from a 320-pixel panorama and draws the supplied simple orange
car with four stable-tire hub-animation frames at 60 updates per second. The car preserves
the supplied block layout, is reduced to seven flat colors using nearest-neighbor
sampling, and has no added body outline or blended edge pixels.

The current ROM is 1 MiB, with a valid GBA boot header. Emulator testing covers
60 Hz movement and rendering. No physical hardware test was performed. Flash
cartridge compatibility depends on its hardware and correct flashing profile.
Keep cartridge backups before replacing any existing game.

The active map has no bottom-left town label or franchise-specific symbols.
Storefronts and civic buildings use generic original signs and emblems.
