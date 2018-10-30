#!/bin/bash

cd kernel_module
sudo make
sudo make install
read -p "Press any key..."
cd ..

# example
printf "growing on the number of processes\n\n"
printf "Running ./test.sh 128 4096 1 1\n"
./test.sh 128 4096 1 1
printf "\n\n"
printf "Running ./test.sh 128 4096 2 1\n"
./test.sh 128 4096 2 1
printf "\n\n"
printf "Running ./test.sh 128 4096 4 1\n"
./test.sh 128 4096 4 1
printf "\n\n"
printf "Running ./test.sh 128 4096 8 1\n"
./test.sh 128 4096 8 1
printf "\n\n"
printf "Running ./test.sh 128 4096 16 1\n"
./test.sh 128 4096 16 1
printf "\n\n"
printf "Running ./test.sh 128 4096 64 1\n"
./test.sh 128 4096 64 1
printf "\n\n"
printf "Running ./test.sh 128 4096 128 1\n"
./test.sh 128 4096 128 1
printf "\n\n"

printf "growing on the number of objects\n\n"
printf "Running ./test.sh 128 4096 1 1\n"
./test.sh 128 4096 1 1
printf "\n\n"
printf "Running ./test.sh 512 4096 1 1\n"
./test.sh 512 4096 1 1
printf "\n\n"
printf "Running ./test.sh 1024 4096 1 1\n"
./test.sh 1024 4096 1 1
printf "\n\n"
printf "Running ./test.sh 4096 4096 1 1\n"
./test.sh 4096 4096 1 1
printf "\n\n"

printf "growing on the object size\n\n"
printf "Running ./test.sh 128 4096 1 1\n"
./test.sh 128 4096 1 1
printf "\n\n"
printf "Running ./test.sh 128 8192 1 1\n"
./test.sh 128 8192 1 1
printf "\n\n"

printf "growing on the number of containers\n\n"
printf "Running ./test.sh 128 4096 2 2\n"
./test.sh 128 4096 2 2
printf "\n\n"
printf "Running ./test.sh 128 4096 8 8\n"
./test.sh 128 4096 8 8
printf "\n\n"
printf "Running ./test.sh 128 4096 64 64\n"
./test.sh 128 4096 64 64
printf "\n\n"

printf "combination\n\n"
printf "Running ./test.sh 128 8192 8 4\n"
./test.sh 128 8192 8 4
printf "Running ./test.sh 128 1288 8 4\n"
./test.sh 128 1288 8 4
printf "Running ./test.sh 128 16384 8 4\n"
./test.sh 128 16384 8 4
printf "Running ./test.sh 128 20480 8 4\n"
./test.sh 128 20480 8 4
printf "\n\n"
