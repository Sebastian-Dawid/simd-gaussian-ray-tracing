#!/usr/bin/env sh

BASEDIR=$(dirname "$0")

if [ $# -lt 2 ]; then
    echo "usage: gen-gif.sh <.obj> <.gif>"
    exit 1
fi

mkdir -p tmp

rm -f "$2"
"$BASEDIR"/build/bin/release/volumetric-ray-tracer -f "$1" -t32 -q --tiles 32 --frames 24 -o ./tmp/img.png -w 512
ffmpeg -f image2 -framerate 12 -i ./tmp/img_%d.png -vf scale=1024x1024 gif.avi
ffmpeg -i gif.avi -pix_fmt rgb24 "$2"
rm gif.avi
rm -rf tmp
