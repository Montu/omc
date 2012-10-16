/*
 * Take Picture Command
 * copyright Akshay Bhardwaj
 */

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"
#include "stdio.h"
#include "TakePicture.h"

#define PORT_NO 1
#define BDRATE 576000
#define BUFFER_SIZE 64

int TakePicture(char* PictureLocation)
{
	int Flag = 0; 	//Return status to define errors
			//0 implies no error
	int n;
	size_t ReadSize;

	unsigned char buf[BUFFER_SIZE];
	FILE *PicturePointer;
	PicturePointer = fopen(PictureLocation,"ab");
	if (PicturePointer == NULL)
	{
		Flag = 1;
		//printf("Cant creat file");
		return(Flag);
	}

	if(OpenComport(PORT_NO, BDRATE))
	{
		Flag = 2;
		//printf("Can't open port");
		return (Flag);
	}

	do{
		n = PollComport(PORT_NO, buf, BUFFER_SIZE);
		ReadSize = fread(buf, sizeof(buf[0]), sizeof(buf)/sizeof(buf[0]), PicturePointer); 
		#ifdef _WIN32
			Sleep(100);
		#else
			usleep(100000);
		#endif
	}while (ReadSize>0);

	return Flag;
}
