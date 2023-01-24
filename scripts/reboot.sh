#!/bin/bash

# Reboots all connected Teensies.

if ! command -v tycmd &>/dev/null; then
  echo -e "Error: requires tytools, which were not found https://github.com/Koromix/tytools\n" >&2
  exit 1
fi

# Query all connected Teensies
teensies=($(tycmd list | grep -Eo "[0-9]+-Teensy"))

# Upload
if ((${#teensies[@]} == 0)); then
  echo "No Teensies found."
else
  for i in "${!teensies[@]}"; do
    echo "Rebooting Teensy $((i + 1)) of ${#teensies[@]}."
    tycmd reset -B "${teensies[$i]}"
  done
  echo "Done!"
fi