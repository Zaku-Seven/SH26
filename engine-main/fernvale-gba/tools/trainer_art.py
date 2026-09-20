"""Original 16-bit-era trainer pixels for the native GBA sprite.

Each letter is one pixel. The compact character occupies about 16x22 pixels
inside its 16x32 hardware sprite. The head overlaps the shoulders so there is
no thin neck. Short arms and boots keep the walking silhouette cartoonish.
"""
from PIL import Image, ImageDraw

OBJ_RGB = [
    (0,0,0), (35,46,57), (90,62,46), (202,139,91),
    (249,198,143), (249,245,220), (176,199,180), (36,116,77),
    (101,178,88), (154,49,51), (235,96,69), (64,76,89),
    (119,141,144), (237,210,114), (174,218,126), (64,105,53),
]
PIXELS = {ch:i for i,ch in enumerate('.ohspWcgGrRdlyLz')}

def pixel_layer(rows, width=16):
    assert all(len(row)<=width for row in rows)
    result=Image.new('P',(width,len(rows)),0)
    for y,row in enumerate(rows):
        for x,ch in enumerate(row):
            result.putpixel((x,y),PIXELS[ch])
    return result

HEAD_DOWN=pixel_layer([
    '.....oooooo.....',
    '....oWWWWWWo....',
    '...oWWWWWWWWo...',
    '..oWWWWWWWWWWo..',
    '.oWWWWccWWWWWWo.',
    '.oWWWcgGcWWWWWo.',
    '..oggGGGGggggo..',
    '..ohspppppshho..',
    '..osWoppppoWso..',
    '..osWoppppoWso..',
    '...oppppppppo...',
    '....spppppps....',
])
HEAD_UP=pixel_layer([
    '.....oooooo.....',
    '....oWWWWWWo....',
    '...oWWWWWWWWo...',
    '..oWWWWWWWWWWo..',
    '.oWWWWWWWWWWWWo.',
    '.oWWWWWccWWWWWo.',
    '..ogggGGgggggo..',
    '..ohggggggghho..',
    '...ohhhhhhhho...',
    '...ohhhhhhhho...',
    '....hhhhhhhh....',
    '....ohhhhhho....',
])
HEAD_RIGHT=pixel_layer([
    '....oooooo......',
    '...oWWWWWWoo....',
    '..oWWWWWWWWW....',
    '.oWWWWWWWWWWo...',
    '.oWWWccWWWWWWoo.',
    '.oWWcgGGggWWWWo.',
    '..ohgggggggooo..',
    '..ohsspWoppso...',
    '..ohsppWopppso..',
    '...osppWopppso..',
    '....hsppppppo...',
    '....osppppss....',
])
TORSO_DOWN=pixel_layer([
    '...oggddggoo....',
    '..oggGddGgggo...',
    '..odgGddddGgdo..',
    '...ogGddddGgo...',
    '...ogdRRRRdgo...',
    '...ogddddddgo...',
    '...oddddddddo...',
    '...odddoodddo...',
])
TORSO_UP=pixel_layer([
    '...ooggggoo.....',
    '..ogGLLLLGgo....',
    '..ogLGGGGLGgo...',
    '..ogLGyyGLGgo...',
    '...ogGLLGGgo....',
    '...ogGGGGGgo....',
    '...oddddddddo...',
    '...odddoodddo...',
])
TORSO_RIGHT=pixel_layer([
    '...ogGdddoo.....',
    '..ogLGddddoo....',
    '.ogLGGdddddRo...',
    '.ogLLGddldRdo...',
    '..ogGGddRRddo...',
    '...ogGdddddgo...',
    '....odddddddo...',
    '....oddoodddo...',
])
BOOT=pixel_layer(['oddo','oRRo','orRo','.oo.'], width=4)
ARM=pixel_layer(['.oo','odd','opp','.so'], width=3)

def paste(dst, src, x, y):
    mask=Image.frombytes('L',src.size,bytes(255 if p else 0 for p in src.tobytes()))
    dst.paste(src,(x,y),mask)

def make_frames():
    frames=[]
    for direction in range(4): # down, up, left, right
        for step in range(4):
            im=Image.new('P',(16,32),0)
            ImageDraw.Draw(im).ellipse((2,28,13,30),fill=15)
            stride=(0,-1,0,1)[step]
            bob=1 if step in (1,3) else 0
            if direction<2:
                paste(im,BOOT,3,26+stride)
                paste(im,BOOT,9,26-stride)
                paste(im,TORSO_DOWN if direction==0 else TORSO_UP,0,18+bob)
                paste(im,ARM,1,20-stride)
                paste(im,ARM.transpose(Image.Transpose.FLIP_LEFT_RIGHT),12,20+stride)
                paste(im,HEAD_DOWN if direction==0 else HEAD_UP,0,8+bob)
            else:
                paste(im,BOOT,5-stride,26-stride)
                paste(im,BOOT,8+stride,26+stride)
                paste(im,TORSO_RIGHT,0,18+bob)
                paste(im,ARM,10,20-stride)
                paste(im,HEAD_RIGHT,0,8+bob)
                if direction==2:
                    im=im.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
            frames.append(im)
    return frames
