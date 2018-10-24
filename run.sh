#!/bin/bash

cd kernel_module
sudo make
sudo make install
read -p "Press any key..."
cd ..
sudo dmesg -C
./test.sh 128 4096 1 1
sudo dmesg
