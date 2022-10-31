#!/bin/bash

if ! command -v tycmd &>/dev/null; then
  echo "Error: requires tytools, which were not found https://github.com/Koromix/tytools" >&2
  echo
  exit 1
fi

# Build
cd "$(dirname "$(realpath "$0")")"/.. || exit 1
pio run
if [ $? -eq 1 ]; then
    exit 1
fi

cd .pio/build/teensy41 || exit 1

echo -e "\nLooking for Teensies to upload to."

# Query all connected Teensies
teensies=($(tycmd list | grep -Eo "[0-9]+-Teensy"))

# Upload
if ((${#teensies[@]} == 0)); then
  echo "No Teensies found."
else
  echo "Teensies found: ${#teensies[@]}"
  for i in "${!teensies[@]}"; do
    echo "Uploading to Teensy $((i + 1)) of ${#teensies[@]}."
    tycmd upload firmware.hex -B "${teensies[$i]}"
  done
  unset i
  echo "Done!"
fi
