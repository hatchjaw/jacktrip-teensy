#!/bin/bash
file=$1

if [ -z "$file" ]; then
  echo "Expected to receive a .dsp file"
fi

if [[ $file == *.dsp ]]; then
  name=$(basename "$file" .dsp)
  faust2teensy -lib $file
  unzip $name.zip
  rm $name.zip
else
  echo "Usage: faust2teensy.sh [filename].dsp"
fi
