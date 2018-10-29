#!/bin/bash

cd kernel_module
sudo make
sudo make install
read -p "Press any key..."
cd ..
sudo dmesg -C
# example
printf "growing on the number of processes\n\n"
printf "Running ./test.sh 128 4096 1 1"
./test.sh 128 4096 1 1
printf "\n\n"
printf "Running ./test.sh 128 4096 2 1"
./test.sh 128 4096 2 1
printf "\n\n"
printf "Running ./test.sh 128 4096 4 1"
./test.sh 128 4096 4 1
printf "\n\n"
printf "Running ./test.sh 128 4096 8 1"
./test.sh 128 4096 8 1
printf "\n\n"
printf "Running ./test.sh 128 4096 16 1"
./test.sh 128 4096 16 1
printf "\n\n"
printf "Running ./test.sh 128 4096 64 1"
./test.sh 128 4096 64 1
printf "\n\n"
printf "Running ./test.sh 128 4096 128 1"
./test.sh 128 4096 128 1
printf "\n\n"

printf "growing on the number of objects"
printf "Running ./test.sh 128 4096 1 1"
./test.sh 128 4096 1 1
printf "\n\n"
printf "Running ./test.sh 512 4096 1 1"
./test.sh 512 4096 1 1
printf "\n\n"
printf "Running ./test.sh 1024 4096 1 1"
./test.sh 1024 4096 1 1
printf "\n\n"
printf "Running ./test.sh 4096 4096 1 1"
./test.sh 4096 4096 1 1
printf "\n\n"

printf "growing on the object size\n\n"
printf "Running ./test.sh 128 4096 1 1"
./test.sh 128 4096 1 1
printf "\n\n"
printf "Running ./test.sh 128 8192 1 1"
./test.sh 128 8192 1 1
printf "\n\n"

printf "growing on the number of containers\n\n"
printf "Running ./test.sh 128 4096 2 2"
./test.sh 128 4096 2 2
printf "\n\n"
printf "Running ./test.sh 128 4096 8 8"
./test.sh 128 4096 8 8
printf "\n\n"
printf "Running ./test.sh 128 4096 64 64"
./test.sh 128 4096 64 64
printf "\n\n"

printf "combination"
printf "Running ./test.sh 256 8192 8 4"
./test.sh 256 8192 8 4
printf "\n\n"
#sudo dmesg
