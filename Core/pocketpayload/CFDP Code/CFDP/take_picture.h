/*
 * Take Picture Command
 * copyright Akshay Bhardwaj
 */

#include "CFDP.h"

//Control AUDRINO_TEST value to chose the mode
//0 --> Test on windows by transfering to usb
//1 --> Test with Audrino
//2 --> Debug with Audrino
#define AUDRINO_TEST 0

//Setup test/debug/audrino system
#if AUDRINO_TEST == 1
#define AUDRINO 1
#endif





