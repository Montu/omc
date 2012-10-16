#!/bin/bash

for file in filelist; do
#	g++ -c -fpic TakePicture.cpp
#	g++ -shared -lc -o TakePicture.dll TakePicture.o
#	g++ -shared -lc -o TakePicture.so TakePicture.o
#	g++ -shared -lc -o TakePicture.dylib TakePicture.o
#	g++ -c -fpic "$file.cpp"
#	g++ -shared -lc -o "$file.dll" "$file.o"
#	g++ -shared -lc -o "$file.so" "$file.o"
#	g++ -shared -lc -o "$file.dylib" "$file.o"
#	g++ -o Test.exe Test.o rs232.dll

g++ -c -DBUILDING_CFDP CFDP.cpp
g++ -shared -o CFDP.dll CFDP.o
g++ -c -DBUILDING_RS232 rs232.c
g++ -shared -o rs232.dll rs232.o
g++ -c -DBUILDING_TAKEPICTURE take_picture.cpp
g++ -shared -o take_picture.dll take_picture.o rs232.dll CFDP.DLL
