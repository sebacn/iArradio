#!python3

from PIL import Image, ImageOps
from argparse import ArgumentParser
import sys
import math

SCREEN_WIDTH = 264
SCREEN_HEIGHT = 176

if SCREEN_WIDTH % 2:
    print("image width must be even!", file=sys.stderr)
    sys.exit(1)

parser = ArgumentParser()
parser.add_argument('-i', action="store", dest="inputfile")
parser.add_argument('-n', action="store", dest="name")
parser.add_argument('-bpp', action="store", type=int, dest="bpp", default=4) #1-4(bpp 1=bw,2=4gray,4=16gray,8=256gray)
parser.add_argument('-o', action="store", dest="outputfile")

args = parser.parse_args()

if args.bpp != 1 and args.bpp != 2 and args.bpp != 4 and args.bpp != 8:
    print("bit per pixel ({}) image must be 1,2,4 or 8!".format(args.bpp), file=sys.stderr)
    sys.exit(1)

im = Image.open(args.inputfile)
# convert to grayscale
im = im.convert(mode='L')
im.thumbnail((150, 150), Image.Resampling.LANCZOS) #SCREEN_WIDTH, SCREEN_HEIGHT), Image.Resampling.LANCZOS)
arrsize = 0;

# Write out the output file.
with open(args.outputfile, 'w') as f:

    f.write("#ifndef _{}_h\n#define _{}_h\n\n".format(args.name, args.name))
    f.write("#include <Arduino.h>\n\n")
    f.write("//Image bpp: {}\n".format(args.bpp))
    f.write("const uint32_t {}_width = {};\n".format(args.name, im.size[0]))
    f.write("const uint32_t {}_height = {};\n".format(args.name, im.size[1]))
    f.write(
        "const uint8_t {}[] PROGMEM = {{\n\t".format(args.name, math.ceil(im.size[0] / 2) * 2, im.size[1], math.ceil(8/args.bpp))
    )

    #byte = 0
    #done = True
    #idx = math.ceil(8/args.bpp)
    
    for y in range(0, im.size[1]):
        byte = 0
        done = True
        idx = math.ceil(8/args.bpp)
        for x in range(0, im.size[0]):
            l = im.getpixel((x, y))
            #print("0x{:02X}, ".format(l))
            #f.write("({:d},{:d},px{:02X}), ".format(x,y,l))
            '''
bitpp   bytes   depth(greys)
8       1       256
4       2       16
2       4       4
1       8       1 b/w
            '''
            done = False
            idx -= 1
            pbyte = l >> (8-args.bpp) # l/16
            #f.write("x{:d},y{:d},px{:02X},pb{:02X}, ".format(x,y,l,pbyte))
            byte |= pbyte << (idx)*args.bpp
            if idx == 0: #(8/args.bpp):
                #f.write("({:d})_".format(idx))
                f.write("0x{:02X}, ".format(byte))
                arrsize +=1;
                byte = 0
                idx = math.ceil(8/args.bpp)
                done = True
            '''
            if x % 2 == 0:
                byte = l >> 4
                done = False;
            else:
                byte |= l & 0xF0
                f.write("0x{:02X}, ".format(byte))
                done = True
            '''
        if done == False:
            #f.write("({:d})_".format(idx))
            f.write("0x{:02X}, ".format(byte))
            idx=0;
            arrsize +=1;
        f.write("\n\t");
    f.write("};\n\n")
    f.write("//array size {}\n\n".format(arrsize))
    f.write("#endif\n")

print("Image {} generated to {} array ({})".format(args.inputfile, args.name, arrsize))