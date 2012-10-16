#!/bin/bash

# Linux shell file to generate dynamic library files
# copyright Akshay Bhardwaj

gcc -c -DBUILDING_RS232 rs232.c
gcc -shared -o rs232.dll rs232.o
gcc -shared -o rs232.so rs232.o
#gcc -dylib -o rs232.dylib rs232.o
gcc -c -DBUILDING_TAKEPICTURE TakePicture.c
gcc -shared -o TakePicture.dll TakePicture.o rs232.dll
gcc -shared -o TakePicture.so TakePicture.o rs232.so
#gcc -dylib -o TakePicture.dylib TakePicture.o rs232.dylib


