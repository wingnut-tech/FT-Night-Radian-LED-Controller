from PIL import Image

image = Image.open('image.png').quantize(colors=16, kmeans=3, method=1)
image.thumbnail((900, 54))

w, h = image.size

colorsOutFormatted = ''
for i in range(16):
    colorsOutFormatted += '0x%02x%02x%02x' % tuple(image.getpalette()[i:i+3]) + ',\n'

imageOut = ''
for x in range(w):
    for y in range(h-1, -1, -1):
        imageOut += '%x' % image.getpixel((x,y))

imageOutFormatted = ''
for i in range(0, len(imageOut), 2):
    if (i % h) == 0:
        imageOutFormatted += '\n'
    imageOutFormatted += '0x' + imageOut[i:i+2] + ', '


outFile = open('image.h', 'w')

outFile.write('const byte image[] PROGMEM = {')
outFile.write(imageOutFormatted[:-2] + '\n')
outFile.write('};\n\n')
outFile.write('int colors[] = {\n')
outFile.write(colorsOutFormatted[:-2] + '\n')
outFile.write('};\n')
