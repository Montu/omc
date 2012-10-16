#include <CFDP.h>
uint8_t Destination;
long Command;
void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
cfdp.initiate(4, MASTER,I2C);
}
void WriteData(uint16_t packPicDataSize, unsigned long Offset, uint8_t* pack)
{
  for(int i=0; i<packPicDataSize; i++)
  {
    pack[i]=i;
  }
}
void loop() {
  // put your main code here, to run repeatedly: 
  if(ReceivedUnexpectedPacket)
  {
    ReceivedUnexpectedPacket = false;
    if(cfdp.ReceivedCommand(&HandleCommand, bytesReceived[4] ,I2C))
    {
      if(Command == 3)
      {
        cfdp.SendData(&WriteData, 0, 40, Destination, 0, I2C);
      }
    }
  }
  delay(100);
}

void HandleCommand(unsigned long CommandId, unsigned long Parameter[], uint8_t NumberOfParameters, uint8_t DestinationId)
{
  Serial.println(Command);
  for(int i =0; i<NumberOfParameters; i++)
  {
    Serial.print(Parameter[i]);
  }
  Serial.println(DestinationId);
  Destination = DestinationId;
  Command = CommandId;
}


