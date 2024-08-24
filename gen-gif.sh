#!/usr/bin/env sh

cd images || exit
rm -f tea.gif
../build/bin/release/volumetric-ray-tracer -f ../test-objects/teapot.obj -t32 -q --tiles 32 --frames 24 -o img.png -w 512
ffmpeg -f image2 -framerate 12 -i img_%d.png -vf scale=1024x1024 gif.avi
ffmpeg -i gif.avi -pix_fmt rgb24 "$ARCH"_tea.gif
rm gif.avi
rm -rf img*
cd ..
