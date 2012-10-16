REM Open Mission Control
REM Windows batch file to generate dynamic library files
REM copyright Akshay Bhardwaj
REM *Insert License*

REM gcc -c -DBUILDING_CFDP CFDP.cpp
REM gcc -shared -o CFDP.dll CFDP.o
gcc -c -DBUILDING_RS232 rs232.c
gcc -shared -o rs232.dll rs232.o
gcc -shared -o rs232.so rs232.o
REM gcc -dylib -o rs232.dylib rs232.o
gcc -c -DBUILDING_TAKEPICTURE TakePicture.c
gcc -shared -o TakePicture.dll TakePicture.o rs232.dll
gcc -shared -o TakePicture.so TakePicture.o rs232.so
REM gcc -dylib -o TakePicture.dylib TakePicture.o rs232.dylib
