#!/usr/bin/env sh

if [ $# -lt 1 ]; then
    notify-send -e "No file specified!"
    exit 0
fi
julia -O3 -t32 --project=julia "$1"
notify-send -e "$1 finished!"
