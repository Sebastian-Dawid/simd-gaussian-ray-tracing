#!/usr/bin/env sh

BASEDIR=$(dirname "$0")

help_msg()
{
    printf "\nusage: %s <.obj> <.git> [OPTIONS]\n" "$0"
    printf "OPTIONS:\n"
    printf "\t-t Specify the number of threads to use for rendering\n"
    printf "\t-s Specify the scale of the images\n"
    printf "\t-f Specify the scaling factor to use for the final gif\n"
    printf "\t-h Show this help message\n"
    exit 1
}

if [ $# -lt 2 ]; then
    help_msg
fi

while getopts "t:s:f:h" opt
do
    case "$opt" in
        t ) threads="$OPTARG" ;;
        s ) scale="$OPTARG" ;;
        f ) factor="$OPTARG" ;;
        h ) help_msg ;;
        ? ) help_msg ;;
    esac
done

if [ -z "$scale" ]; then scale=512; fi
if [ -z "$factor" ]; then factor=2; fi
if [ -z "$threads" ]; then threads=16; fi

final_scale=$((factor * scale))
simd=8
if grep avx512 /proc/cpuinfo > /dev/null ; then
    simd=16
fi

tile_count=$((scale/simd))

mkdir -p tmp

rm -f "$2"
"$BASEDIR"/build/bin/release/volumetric-ray-tracer -f "$1" -t"$threads" -q --tiles "$tile_count" --frames 24 -o ./tmp/img.png -w "$scale"
ffmpeg -f image2 -framerate 12 -i ./tmp/img_%d.png -vf scale="$final_scale"x"$final_scale" gif.avi
ffmpeg -i gif.avi -pix_fmt rgb24 "$2"
rm gif.avi
rm -rf tmp
