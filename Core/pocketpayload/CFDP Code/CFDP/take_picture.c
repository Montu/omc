/*
 * Take Picture Command
 * copyright Akshay Bhardwaj
 */


#include "take_picture.h"

int take_picture() {
  if(cfdp.SendCommand(3, Parameters, LengthOfParameter, 1, 4, I2C))
  {
    cfdp.ReceiveData(&SlaveReceiveData, 0, 4, 0, I2C);
  }
  delay(1000);
}
void SlaveReceiveData(uint16_t packPicDataSize, unsigned long Offset, uint8_t* pack)
{
    for(int i=0; i<packPicDataSize; i++)
  {
    Serial.print(pack[i]);
  }
  
}


