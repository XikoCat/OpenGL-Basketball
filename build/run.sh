#!/bin/sh
rm -fr ./proj
echo "--compiling--"
cmake -S .. -B .
make
echo "---running---"
./proj/basket.exe
