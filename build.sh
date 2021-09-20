#!/bin/bash

version=$(sed -nE "s/.*VERSION: (.*)/\1/p" FT-Night-Radian-LED-Controller.ino)

mkdir -p build

for file in configs/*.h; do
  echo $file
  configname=$(basename $file .h)
  outname=${configname}_v${version}.hex
  echo "--- $configname ---"

  ln -sf $file ./config.h
  arduino-cli compile --fqbn arduino:avr:nano --output-dir _build
  mv _build/FT-Night-Radian-LED-Controller.ino.hex build/$outname
done
