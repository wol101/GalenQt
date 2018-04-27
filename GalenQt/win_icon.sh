#!/bin/sh
# uses imagemagick convert to generate all the required resolutions from a ncie big square design (1024x1024 works well)
# note that 'mogrify -strip image.png' can be used to get rid of some of the load errors in Qt
convert images/icon_design.png -bordercolor white -border 0 \( -clone 0 -resize 16x16 \) \( -clone 0 -resize 24x24 \) \( -clone 0 -resize 32x32 \) \( -clone 0 -resize 40x40 \) \( -clone 0 -resize 48x48 \) \( -clone 0 -resize 64x64 \) \( -clone 0 -resize 256x256 \) -delete 0 -alpha off -colors 256 icon_256x256.ico

