#!/bin/sh
set -e

make

sudo mkdir -p /usr/local/share/badapple
sudo install -m 755 badapple /usr/local/bin/badapple
sudo install -m 644 bad_apple.mp4 /usr/local/share/badapple/
sudo install -m 644 bad_apple.wav /usr/local/share/badapple/

echo "Installed to /usr/local/bin/badapple"