#!/bin/bash
for i in *.jpg; do echo "Processing $i"; exiftool -overwrite_original -all= "$i"; done
