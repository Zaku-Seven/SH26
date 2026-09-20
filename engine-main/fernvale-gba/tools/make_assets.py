#!/usr/bin/env python3
"""Draw original pixel art and convert it to native GBA tiles.

Only needed when changing the artwork or map. Generated C data is included.
Run: python3 -m pip install Pillow && python3 tools/make_assets.py
"""
from pathlib import Path
import random
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "source"
ART = ROOT / "art"
OUT.mkdir(exist_ok=True)
ART.mkdir(exist_ok=True)

# One shared, carefully limited palette. Index 0 is transparent on the GBA.
COLORS = [
    "#183f39", "#81b85a", "#91c565", "#70aa51", "#a8cf76", # grass
    "#305843", "#41734a", "#538e4d", "#6aa951", "#87bb60", # leaves
    "#bed786", "#d2dba0", "#c9aa72", "#debf85", "#ecd09a", # paths
    "#f4dda7", "#b69767", "#8b7955", "#386f72", "#408b8c", # water
    "#53a3a0", "#74beb0", "#a4d3bb", "#f3ecc9", "#d4c8a1", # foam, wall
    "#ad9675", "#786652", "#aa5b4f", "#c97259", "#e58c69", # roof
    "#f1ad7b", "#6d4842", "#35565c", "#56868b", "#93bec1", # windows
    "#c1ddd2", "#825c40", "#ad7c4e", "#cf9d63", "#e2b77d", # wood
    "#e4b55c", "#f6d879", "#db796f", "#f3aa92", "#fff0c0", # flowers
    "#6b725d", "#969d78", "#b6bb91", "#375e57", "#203e3c", # rocks, UI
    "#57907a", "#2b5347", "#244036", "#f7eccb", "#c3d6b0", # UI
]
RGB = [tuple(bytes.fromhex(c[1:])) for c in COLORS]
PAL = [v for rgb in RGB for v in rgb] + [0] * (768 - len(RGB) * 3)

def canvas(w, h, color=0):
    im = Image.new("P", (w, h), color)
    im.putpalette(PAL)
    return im

def paste(dst, src, x, y):
    mask = Image.frombytes("L", src.size, bytes(255 if p else 0 for p in src.tobytes()))
    dst.paste(src, (x, y), mask)

def rect(draw, box, color):
    x, y, w, h = box
    if w > 0 and h > 0:
        draw.rectangle((x, y, x+w-1, y+h-1), fill=color)

def grass(variant=0, tall=False, flowers=False):
    im = canvas(16, 16, 1)
    d = ImageDraw.Draw(im)
    rng = random.Random(19 + variant)
    for _ in range(5):
        x, y = rng.randrange(2, 14), rng.randrange(2, 14)
        d.line((x-1, y, x, y+1, x+2, y-1), fill=3 if _ % 2 else 2)
    if tall:
        for x,y in ((2,6),(8,4),(13,7),(5,13),(11,14)):
            d.line((x-2,y-3,x,y,x+2,y-4), fill=6)
            d.line((x-1,y-3,x,y-1,x+1,y-4), fill=4)
            d.point((x,y+1), fill=3)
    if flowers:
        for x,y in ((4,5),(11,11)):
            d.line((x,y,x,y+3), fill=6)
            c = 42 if variant % 2 else 40
            d.line((x-1,y,x+1,y), fill=c)
            d.line((x,y-1,x,y+1), fill=c)
            d.point((x,y), fill=44)
    return im

terrain = [["g"] * 32 for _ in range(32)]
solid = [[0] * 64 for _ in range(64)]

def paint(x, y, w, h, kind):
    for yy in range(y, y+h):
        for xx in range(x, x+w):
            if 0 <= xx < 32 and 0 <= yy < 32:
                terrain[yy][xx] = kind

# Main village lanes and the small southern garden.
paint(11, 3, 2, 27, "p")
paint(3, 14, 27, 2, "p")
paint(6, 12, 1, 3, "p")
paint(16, 12, 1, 3, "p")
paint(6, 24, 15, 2, "p")
paint(7, 23, 1, 3, "p")
paint(11, 5, 15, 2, "p")

# A winding stream opens into a pond, with a walkable bridge across it.
for y in range(3, 30):
    start = 25 if y < 18 else 24
    paint(start, y, 3, 1, "w")
for y in range(20, 29):
    for x in range(17, 29):
        if ((x-22.5)/5.6)**2 + ((y-24)/4.4)**2 < 1:
            terrain[y][x] = "w"
paint(24, 14, 5, 2, "b")
paint(24, 5, 5, 2, "b")
paint(18, 8, 4, 4, "t")
paint(3, 18, 5, 2, "t")
paint(13, 27, 4, 3, "t")
for x,y,w,h in ((4,13,2,1),(18,13,2,1),(14,17,3,2),(5,27,3,2),(20,4,2,1)):
    paint(x,y,w,h,"f")

ground = canvas(512,512,1)
over = canvas(512,512)
gd, od = ImageDraw.Draw(ground), ImageDraw.Draw(over)

def block(x,y,w,h):
    for yy in range(max(0,y//8), min(64,(y+h+7)//8)):
        for xx in range(max(0,x//8), min(64,(x+w+7)//8)):
            solid[yy][xx] = 1

for ty in range(32):
    for tx in range(32):
        kind = terrain[ty][tx]
        tile = grass((tx*7+ty*11) % 3, kind=="t", kind=="f")
        d = ImageDraw.Draw(tile)
        if kind == "p":
            tile.paste(14,(0,0,16,16))
            # Four exposed edges form a grassy outline around the sand.
            for dx,dy,box,inner in ((0,-1,(0,0,16,2),(0,2,16,1)),
                                    (0,1,(0,14,16,2),(0,13,16,1)),
                                    (-1,0,(0,0,2,16),(2,0,1,16)),
                                    (1,0,(14,0,2,16),(13,0,1,16))):
                xx,yy=tx+dx,ty+dy
                if not(0<=xx<32 and 0<=yy<32) or terrain[yy][xx] not in "pb":
                    rect(d,box,3); rect(d,inner,12)
            d.point((5,5),fill=13); d.line((10,11,11,11),fill=15)
        elif kind == "w":
            tile.paste(19,(0,0,16,16))
            d.line((1,5,5,5), fill=20); d.point((6,4),fill=21)
            d.line((10,12,14,12),fill=20); d.point((9,13),fill=21)
            for dx,dy,box,inner in ((0,-1,(0,0,16,3),(0,3,16,1)),
                                    (0,1,(0,13,16,3),(0,12,16,1)),
                                    (-1,0,(0,0,3,16),(3,0,1,16)),
                                    (1,0,(13,0,3,16),(12,0,1,16))):
                xx,yy=tx+dx,ty+dy
                if not(0<=xx<32 and 0<=yy<32) or terrain[yy][xx] not in "wb":
                    rect(d,box,12); rect(d,inner,22)
            block(tx*16,ty*16,16,16)
        elif kind == "b":
            tile.paste(36,(0,0,16,16))
            for y in range(0,16,4):
                rect(d,(0,y,16,3),38)
                d.line((0,y,15,y),fill=39)
                d.point((2,y+1),fill=36); d.point((13,y+1),fill=36)
        ground.paste(tile,(tx*16,ty*16))

def tree(x,y):
    """Upper leaves are a foreground layer; the trunk is solid."""
    im=canvas(32,40); d=ImageDraw.Draw(im)
    d.ellipse((3,31,28,39),fill=3)
    rect(d,(13,25,7,13),36); rect(d,(14,27,3,10),38)
    d.polygon([(2,17),(4,9),(9,9),(9,4),(14,2),(21,3),(26,8),
               (28,14),(30,17),(29,27),(25,32),(18,34),(10,33),(4,29),(1,23)],fill=5)
    d.polygon([(4,16),(6,10),(12,7),(16,4),(22,5),(26,11),(26,16),
               (28,20),(25,28),(17,31),(9,29),(4,25)],fill=6)
    for box in ((5,10,18,22),(10,5,23,18),(15,13,27,25),(6,20,18,28)):
        d.ellipse(box,fill=7)
    for box in ((7,9,16,17),(13,6,21,13),(17,13,24,20),(8,19,15,24)):
        d.ellipse(box,fill=8)
    for x1,y1 in ((11,9),(16,7),(19,13),(8,20),(22,19),(13,23)):
        d.line((x1,y1,x1+2,y1),fill=9)
    paste(ground,im.crop((0,24,32,40)),x,y+24)
    paste(over,im.crop((0,0,32,24)),x,y)
    block(x+8,y+24,16,16)

# Dense tree border, with smaller groups that leave the lanes open.
for x in range(0,512,32):
    tree(x,0); tree(x,472)
for y in range(32,472,32):
    tree(0,y); tree(480,y)
for x,y in ((48,64),(80,56),(128,40),(208,32),(256,32),(304,32),
            (32,112),(32,152),(144,96),(176,64),(304,112),(336,128),
            (352,192),(48,264),(96,288),(144,288),(208,288),
            (256,288),(32,336),(144,368),(192,408),(240,432),
            (80,432),(128,440),(448,80),(448,176),(448,304),(448,400)):
    tree(x,y)

def house(x,y,roof=27):
    im=canvas(64,64); d=ImageDraw.Draw(im)
    rect(d,(4,57,58,6),3)
    rect(d,(5,26,54,33),26); rect(d,(7,28,50,27),23)
    rect(d,(7,46,50,9),24)
    for yy in (32,39,47,53):
        d.line((7,yy,56,yy),fill=24 if yy<46 else 25)
    for xx in (11,42):
        rect(d,(xx,33,12,13),25); rect(d,(xx+1,33,10,11),32)
        rect(d,(xx+2,34,8,9),33); rect(d,(xx+3,34,6,3),34)
        rect(d,(xx+5,34,1,10),35); rect(d,(xx+1,38,10,1),35)
        rect(d,(xx-1,46,14,2),26); rect(d,(xx,44,12,2),39)
    rect(d,(27,36,12,21),26); rect(d,(29,37,8,20),36)
    rect(d,(30,39,6,8),37); d.point((35,49),fill=41)
    rect(d,(25,56,16,4),25); rect(d,(25,56,16,1),23)
    d.polygon([(0,26),(8,7),(15,1),(49,1),(56,7),(63,26),(63,30),(0,30)],fill=31)
    d.polygon([(3,25),(11,8),(17,3),(47,3),(53,8),(60,25)],fill=roof)
    for yy in (8,14,20,25):
        inset=max(4,15-yy//2)
        d.line((inset,yy,63-inset,yy),fill=roof+1)
        d.line((inset,yy+1,63-inset,yy+1),fill=roof+2)
        for xx in range(inset+4+(yy%3),63-inset,10):
            d.line((xx,yy-4,xx-2,yy-1),fill=31)
    rect(d,(15,2,35,2),roof+3)
    rect(d,(3,27,58,2),roof+1)
    rect(d,(44,1,8,10),25); rect(d,(43,0,10,3),24)
    # Keep roof data transparent, independent of grass tile variation.
    paste(over,im.crop((0,0,64,32)),x,y)
    paste(ground,im.crop((0,32,64,64)),x,y+32)
    block(x+8,y+24,48,32)

house(64,144)
house(240,144)
house(80,328)

def fence(x,y,w):
    rect(gd,(x,y+6,w,3),36); rect(gd,(x,y+6,w,1),39)
    rect(gd,(x,y+12,w,3),37)
    for xx in range(x,x+w,16):
        rect(gd,(xx+3,y+2,5,15),36); rect(gd,(xx+4,y+2,3,13),39)
        gd.point((xx+5,y+1),fill=39)
    block(x,y+8,w,8)

fence(64,216,32); fence(112,216,24)
fence(240,216,16); fence(272,216,32)
fence(48,408,80)

def sign(x,y):
    rect(gd,(x+7,y+8,3,8),36)
    rect(gd,(x+1,y+1,14,10),36); rect(gd,(x+2,y+2,12,7),39)
    gd.line((x+4,y+4,x+11,y+4),fill=26)
    gd.line((x+4,y+6,x+9,y+6),fill=26)
    block(x,y+8,16,8)

sign(208,224); sign(352,240); sign(224,384)
for x,y in ((64,96),(320,80),(336,352),(144,416),(432,432)):
    gd.ellipse((x,y+5,x+15,y+14),fill=45)
    gd.polygon([(x+1,y+10),(x+3,y+4),(x+8,y+1),(x+13,y+4),(x+14,y+10)],fill=46)
    gd.line((x+4,y+5,x+8,y+3,x+11,y+5),fill=47)
    block(x,y+8,16,8)
block(0,0,512,16); block(0,496,512,16)
block(0,0,16,512); block(496,0,16,512)

# Simple 3x5 lettering, expanded into 8x8 tiles for native BG0 text.
FONT = {
 'A':'010101111101101','B':'110101110101110','C':'011100100100011',
 'D':'110101101101110','E':'111100110100111','F':'111100110100100',
 'G':'011100101101011','H':'101101111101101','I':'111010010010111',
 'J':'001001001101010','K':'101101110101101','L':'100100100100111',
 'M':'101111111101101','N':'101111111111101','O':'010101101101010',
 'P':'110101110100100','Q':'010101101111011','R':'110101110101101',
 'S':'011100010001110','T':'111010010010010','U':'101101101101111',
 'V':'101101101101010','W':'101101111111101','X':'101101010101101',
 'Y':'101101010010010','Z':'111001010100111',
 '0':'111101101101111','1':'010110010010111','2':'110001010100111',
 '3':'110001010001110','4':'101101111001001','5':'111100110001110',
 '6':'011100111101111','7':'111001010010010','8':'111101111101111',
 '9':'111101111001110',' ':'000000000000000','.':'000000000000010',
 ':':'000010000010000','!':'010010010000010','?':'110001010000010',
 '-':'000000111000000','/':'001001010100100',
}

tiles = [bytes(64)]
lookup = {tiles[0]:0}
def add_tile(data):
    data=bytes(data)
    if data not in lookup:
        lookup[data]=len(tiles); tiles.append(data)
    return lookup[data]

def tilemap(im):
    # GBA maps are four consecutive 32x32 screen blocks, not flat 64x64 rows.
    result=[0]*4096
    for ty in range(64):
        for tx in range(64):
            data=im.crop((tx*8,ty*8,tx*8+8,ty*8+8)).tobytes()
            i=((ty//32)*2+tx//32)*1024+(ty%32)*32+tx%32
            result[i]=add_tile(data)
    return result

ground_map=tilemap(ground)
over_map=tilemap(over)

# A compact house interior. It uses the same shared palette and tile set as
# the overworld, so changing areas only requires swapping tile maps in VRAM.
interior=canvas(512,512,0)
interior_over=canvas(512,512)
interior_solid=[[1]*64 for _ in range(64)]
idraw=ImageDraw.Draw(interior)

# Match the outdoor house: cream plaster, dark timber edges, muted warm wood.
# Every furniture item has a top plane, a short front face, and a floor shadow.
rect(idraw,(8,8,224,144),26)
rect(idraw,(10,10,220,140),25)
rect(idraw,(16,16,208,128),23)
rect(idraw,(16,44,208,100),38)
for row,y in enumerate(range(48,144,8)):
    idraw.line((16,y,223,y),fill=37)
    idraw.line((16,y+1,223,y+1),fill=39)
    for x in range(16+(row%2)*16,224,32):
        idraw.line((x,y+2,x,y+7),fill=37)
        if x+22<224: idraw.line((x+15,y+5,x+22,y+5),fill=39)
rect(idraw,(16,16,208,2),24)
rect(idraw,(16,36,208,10),24)
for x in range(16,224,16): rect(idraw,(x,37,1,7),25)
rect(idraw,(16,43,208,3),26)
rect(idraw,(16,43,208,1),39)
rect(idraw,(16,46,208,2),37)
# Side-wall bevels and a visible opening in the south wall.
rect(idraw,(12,16,4,128),24); rect(idraw,(224,16,4,128),24)
rect(idraw,(16,140,88,4),26); rect(idraw,(136,140,88,4),26)
rect(idraw,(16,140,88,1),39); rect(idraw,(136,140,88,1),39)
rect(idraw,(104,140,32,12),36); rect(idraw,(108,140,24,12),14)
rect(idraw,(108,145,24,1),16)

# Two curtained windows use the same blue glass as the outdoor windows.
for x in (64,144):
    rect(idraw,(x-2,18,36,23),25)
    rect(idraw,(x,19,32,19),26); rect(idraw,(x+2,20,28,16),32)
    rect(idraw,(x+3,21,26,13),33); rect(idraw,(x+4,21,24,5),34)
    idraw.line((x+5,25,x+10,21),fill=35)
    rect(idraw,(x+15,20,2,16),35); rect(idraw,(x+2,27,28,1),35)
    rect(idraw,(x-3,17,38,2),36)
    for xx in (x-1,x+28):
        rect(idraw,(xx,19,5,16),50); rect(idraw,(xx+1,19,2,12),54)
        rect(idraw,(xx,30,5,2),39)
    rect(idraw,(x-3,38,38,3),26); rect(idraw,(x-3,38,38,1),39)

# Low bookshelf with little book spines and paneled lower doors.
rect(idraw,(24,56,32,3),37)
rect(idraw,(24,34,32,22),36); rect(idraw,(25,35,30,2),39)
rect(idraw,(27,38,26,9),26)
for x,h,c in ((28,7,28),(32,8,50),(36,6,40),(41,8,33),(46,7,42),(50,6,24)):
    rect(idraw,(x,46-h,3,h),c); idraw.point((x+1,44),fill=23)
rect(idraw,(25,47,30,2),39)
for x in (27,41):
    rect(idraw,(x,50,12,5),37); rect(idraw,(x+1,50,10,1),38)
    idraw.point((x+8,52),fill=41)

# Bed sized to the trainer, with pillow, folded green quilt, and timber feet.
rect(idraw,(26,109,33,4),37)
rect(idraw,(24,61,32,49),36); rect(idraw,(25,62,30,5),39)
rect(idraw,(27,67,26,37),24); rect(idraw,(28,67,24,9),23)
rect(idraw,(30,68,20,7),53); rect(idraw,(31,74,18,1),24)
rect(idraw,(27,77,26,25),6); rect(idraw,(28,78,24,22),50)
rect(idraw,(28,78,24,4),54); rect(idraw,(29,83,2,17),8)
rect(idraw,(49,83,2,17),6)
for y in (86,94):
    for x in (35,43):
        idraw.line((x,y-2,x+2,y,x,y+2,x-2,y,x,y-2),fill=54)
rect(idraw,(25,103,30,5),37); rect(idraw,(25,103,30,1),39)
rect(idraw,(25,108,4,3),36); rect(idraw,(51,108,4,3),36)

# A woven rug anchors the room without obstructing the central walking route.
rect(idraw,(80,82,72,44),37)
rect(idraw,(80,80,72,44),31); rect(idraw,(81,81,70,42),28)
rect(idraw,(84,84,64,36),39); rect(idraw,(86,86,60,32),29)
rect(idraw,(89,89,54,26),28)
for x in range(84,149,4):
    idraw.line((x,78,x,79),fill=24); idraw.line((x,124,x,125),fill=24)
for x in (94,116,138):
    idraw.line((x,95,x+6,102,x,109,x-6,102,x,95),fill=39)
    idraw.point((x,102),fill=23)

# Small table: horizontal top, short apron, compact legs. Not a giant desk.
rect(idraw,(165,96,38,4),37)
rect(idraw,(168,88,4,10),36); rect(idraw,(194,88,4,10),36)
rect(idraw,(160,66,40,26),36); rect(idraw,(161,67,38,20),38)
rect(idraw,(162,68,36,1),39); rect(idraw,(163,72,34,1),39)
rect(idraw,(162,86,36,4),37); rect(idraw,(162,86,36,1),39)
# Open book and a little cup on the tabletop.
rect(idraw,(166,73,16,10),25); rect(idraw,(166,72,16,9),23)
rect(idraw,(173,72,1,9),25)
for y in (74,77):
    rect(idraw,(168,y,4,1),24); rect(idraw,(176,y,4,1),24)
idraw.ellipse((188,74,195,79),fill=26)
rect(idraw,(188,71,6,6),35); rect(idraw,(189,71,4,2),32)
idraw.point((195,74),fill=23)
# Stool below the table, leaving generous circulation around the rug.
rect(idraw,(176,109,3,8),36); rect(idraw,(189,109,3,8),36)
rect(idraw,(174,102,20,10),36); rect(idraw,(175,103,18,6),38)
rect(idraw,(176,103,16,1),39); rect(idraw,(176,109,16,2),37)

# Leaf clusters repeat the shaded greens used on outdoor trees.
rect(idraw,(200,54,16,3),37)
rect(idraw,(202,43,12,12),36); rect(idraw,(203,44,10,9),28)
rect(idraw,(202,43,12,3),30); rect(idraw,(206,53,6,1),27)
idraw.line((208,31,208,44),fill=36)
for box in ((198,31,209,40),(206,27,216,38),(201,24,211,35)):
    idraw.ellipse(box,fill=5)
    a,b,c,d=box
    idraw.ellipse((a+1,b+1,c-2,d-2),fill=7)
    idraw.line((a+3,b+2,a+5,b+2),fill=9)
# Clearly recognizable doormat aligned with the actual exit trigger.
rect(idraw,(106,131,28,8),36); rect(idraw,(107,132,26,6),39)
for y in (133,135): rect(idraw,(109,y,22,1),37)

# Walkable rectangle, with furniture marked solid. The doorway remains open.
for yy in range(6,18):
    for xx in range(2,28): interior_solid[yy][xx]=0
def interior_block(x,y,w,h):
    for yy in range(max(0,y//8),min(64,(y+h+7)//8)):
        for xx in range(max(0,x//8),min(64,(x+w+7)//8)):
            interior_solid[yy][xx]=1
interior_block(24,48,32,8)
interior_block(24,64,32,48)
interior_block(160,64,40,32)
interior_block(176,104,16,16)
interior_block(200,48,16,8)

interior_map=tilemap(interior)
interior_over_map=tilemap(interior_over)
font_ids=[]
for ch in range(32,96):
    im=canvas(8,8,49)
    d=ImageDraw.Draw(im)
    bits=FONT.get(chr(ch),FONT[' '])
    for y in range(5):
        for x in range(3):
            if bits[y*3+x]=='1':
                rect(d,(x*2+1,y+1,2,1),53)
    font_ids.append(add_tile(im.tobytes()))
panel_tile=add_tile(bytes([49]*64))
assert len(tiles)<=640, f'Too many BG tiles: {len(tiles)} (max 640)'

# Character pixels are kept separate so the world art is easy to preserve.
from trainer_art import OBJ_RGB, make_frames
frames=make_frames()

sprite_bytes=[]
for im in frames:
    for ty in range(4):
        for tx in range(2):
            pix=list(im.crop((tx*8,ty*8,tx*8+8,ty*8+8)).tobytes())
            sprite_bytes.extend(pix[i] | (pix[i+1]<<4) for i in range(0,64,2))

def bgr555(rgb):
    r,g,b=rgb
    return (r>>3)|((g>>3)<<5)|((b>>3)<<10)

def words(data):
    return [data[i]|(data[i+1]<<8) for i in range(0,len(data),2)]

arrays={
    'bg_palette': ('unsigned short',[bgr555(c) for c in RGB]+[0]*(256-len(RGB))),
    'bg_tiles': ('unsigned short',words(b''.join(tiles))),
    'ground_map': ('unsigned short',ground_map),
    'foreground_map': ('unsigned short',over_map),
    'collision_map': ('unsigned char',[v for row in solid for v in row]),
    'interior_ground_map': ('unsigned short',interior_map),
    'interior_foreground_map': ('unsigned short',interior_over_map),
    'interior_collision_map': ('unsigned char',[v for row in interior_solid for v in row]),
    'font_tiles': ('unsigned short',font_ids),
    'player_palette': ('unsigned short',[bgr555(c) for c in OBJ_RGB]),
    'player_tiles': ('unsigned short',words(sprite_bytes)),
}
header=['/* Generated by tools/make_assets.py. */','#ifndef ASSETS_H','#define ASSETS_H',
        f'#define BG_TILE_HALFWORDS {len(tiles)*32}',f'#define PANEL_TILE {panel_tile}',
        '#define PLAYER_TILE_HALFWORDS 2048']
body=['/* Generated original artwork. Edit tools/make_assets.py, then regenerate. */',
      '#include "assets.h"']
for name,(ctype,values) in arrays.items():
    header.append(f'extern const {ctype} {name}[{len(values)}];')
    body.append(f'const {ctype} {name}[{len(values)}] __attribute__((aligned(4))) = {{')
    for i in range(0,len(values),16):
        body.append('    '+','.join(str(v) for v in values[i:i+16])+',')
    body.append('};')
header.append('#endif')
(OUT/'assets.h').write_text('\n'.join(header)+'\n')
(OUT/'assets.c').write_text('\n'.join(body)+'\n')

preview=ground.copy(); paste(preview,over,0,0)
preview.convert('RGB').save(ART/'world.png')
interior_preview=interior.copy(); paste(interior_preview,interior_over,0,0)
interior_preview.crop((0,0,240,160)).convert('RGB').save(ART/'house-interior.png')
sheet=Image.new('RGB',(16*4,32*4),(129,184,90))
objpal=[v for c in OBJ_RGB for v in c]+[0]*(768-len(OBJ_RGB)*3)
for i,im in enumerate(frames):
    im.putpalette(objpal)
    mask=Image.frombytes('L',im.size,bytes(255 if p else 0 for p in im.tobytes()))
    sheet.paste(im.convert('RGB'),((i%4)*16,(i//4)*32),mask)
sheet.save(ART/'character.png')
print(f'Generated {len(tiles)} background tiles, 16 character frames, an overworld, and a house interior.')
