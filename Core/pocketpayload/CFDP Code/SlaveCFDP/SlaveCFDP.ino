#include <CFDP.h>

unsigned long Parameters[1];
uint8_t LengthOfParameter[1];
void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);
   cfdp.initiate(2, SLAVE,I2C);
   
   LengthOfParameter[0] = 1;
   
   Parameters[0] = 1;
}
void loop() {
  // put your main code here, to run repeatedly: 
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
void HandleCommand(unsigned long CommandId, unsigned long Parameter[], uint8_t NumberOfParameters, uint8_t DestinationId)
{
}

