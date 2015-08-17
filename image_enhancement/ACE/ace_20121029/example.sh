#! /bin/sh
# ACE automatic color enhancement demo

# Echo shell commands
set -v

# Perform ACE automatic color enhancement on avs.jpg with alpha = 8
./ace -a 8 -m interp:12 avs.jpg ace.bmp

# Perform uniform histogram equalization
./histeq avs.jpg equalized.bmp
