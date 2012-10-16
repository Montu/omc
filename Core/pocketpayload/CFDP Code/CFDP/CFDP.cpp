/**
 * Copyright 2012
 * Johan Marx
 */

#include "CFDP.h"
#ifdef ARDUINO
#include "..\Wire\Wire.cpp"
#include <avr/pgmspace.h>
#else
#include <windows.h>
#endif
/******************************************************************************
 * Constants & variables
 *****************************************************************************/
static const uint8_t transactionNumberLength = 1; //length in bytes (max 8)
static const uint8_t BeginPacket = transactionNumberLength + 6;
bool ReceiveFlag;
bool UnexpectedPacket;
bool ReceivedUnexpectedPacket;
unsigned int ReceivedBytes;
unsigned int SlavePduLength;
unsigned long SlaveTransaction;
unsigned int SlaveMissingSegment=0;
unsigned int CommanderId=0;
uint8_t ReceivedLengthOfID = 0;
uint8_t ReceivedLengthOfTransaction = 0;
uint8_t ReceivedHeaderLength = 0;
uint8_t TypeOfMessage = 0;
uint8_t bytesReceived[MAX_PACKET_LENGTH];
uint8_t bytesReceivedOverI2C[32];
long Parameter[MAX_COMMAND_PARAMETERS];
CFDP::TransactionStatus SlaveTransactionStatus;
CFDP::ConditionCode SlaveConditionCode;
CFDP::DeliveryCode SlaveDeliveryCode;
CFDP::FileStatus SlaveFileStatus;
CFDP cfdp;

/******************************************************************************
 * Messages
 *****************************************************************************/
#if defined(ARDUINO) && defined(DEBUG)
prog_char notInitMessage[] PROGMEM = "Not initiated";
prog_char initMessage[] PROGMEM = "Initiated with address : ";
prog_char sentMessage[] PROGMEM = " sent";
prog_char receivedMessage[] PROGMEM = " received";
prog_char metaMessage[] PROGMEM = "Metadata";
prog_char dataMessage[] PROGMEM = "Data";
prog_char lastDataMessage[] PROGMEM = "Last data";
prog_char eofMessage[] PROGMEM = "EOF";
prog_char eofAckMessage[] PROGMEM = "EOF Ack";
prog_char finMessage[] PROGMEM = "FIN";
prog_char finAckMessage[] PROGMEM = "FIN Ack";
prog_char ackAcceptedMessage[] PROGMEM = "ACK accepted";
prog_char beginTransMessage[] PROGMEM = "Beginning transmission";
prog_char endTransMessage[] PROGMEM = "Ended transmission with : ";
prog_char notReceivedMessage[] PROGMEM = "Nothing received";
prog_char overReceivedMessage[] PROGMEM = "Received ";
prog_char wrongReceivedMessage[] PROGMEM = " bytes and expected ";
prog_char ackNotAcceptedMessage[] PROGMEM = "Wrong ACK";
prog_char wrongParaMessage[] PROGMEM = "Wrong Parameter";

PROGMEM const char *message_table[] = 
{   
  notInitMessage, //0
  initMessage, //1
  sentMessage, //2
  receivedMessage, //3
  metaMessage, //4
  dataMessage, //5
  lastDataMessage, //6
  eofMessage, //7
  eofAckMessage, //8
  finMessage, //9
  finAckMessage, //10
  ackAcceptedMessage, //11
  beginTransMessage, //12
  endTransMessage, //13
  notReceivedMessage, //14
  overReceivedMessage, //15
  wrongReceivedMessage, //16
  ackNotAcceptedMessage, //17
  wrongParaMessage //18
};
#endif

/******************************************************************************
 * Methods
 *****************************************************************************/

/**
 * Constructor.
 */
CFDP::CFDP()
{
	SourceId=0;
	for(int i=0; i<MAX_PACKET_LENGTH; i++)
	{
		bytesReceived[i]=0;
		bytesToSend[i]=0;
		if(i<32) bytesReceivedOverI2C[i]=0;
	}
	ReceiveFlag = false;
	ReceivedBytes = 0;
	UnexpectedPacket = true;
	ReceivedUnexpectedPacket = false;
	pduDataLength = 0;
}

/**
 * Initiate the I²C
 * @parameters:
 * address: a 7-bit address used if slave
 * isMaster: put MASTER if master, SLAVE if slave
 * @return True if successful, false otherwise
 */
bool CFDP::initiate(uint8_t address, bool isMaster, unsigned int protocol)
{
	char buffer[30];
	#ifdef ARDUINO
	if(protocol == I2C)
	{
		if(address>127)
		{
			#ifdef DEBUG
			strcpy_P(buffer, (char*)pgm_read_word(&(message_table[0])));
			Serial.println(buffer);
			#endif
			return false;
		}
		Wire.begin(address);
		SourceId = address;
		Wire.onReceive(ReceivePacket);
		#ifdef DEBUG
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[1])));
		Serial.print(buffer);
		Serial.println(address);
		#endif
	}
	#endif
  return true;
}
/**
 * Code the packet header
 * @parameters:
 * Type: Tells us which type of packet it is
 * LengthOfParameter
 * DestinationId: The address to send the data to
 * Transaction: What kind of data it is
 * @return True if successful, false otherwise
 */
bool CFDP::CodePacketHeader(int Type, uint8_t NumberOfBytesToCode, uint8_t LengthOfParameter[], uint8_t NumberOfParameters, uint8_t MissingSegment, uint8_t DestinationId, unsigned long Transaction)
{
	for(int i=0; i<transactionNumberLength; i++)
	{
		bytesToSend[5+i] = (Transaction>>(8*(transactionNumberLength - i - 1)) & 0xFF);
	}
	bytesToSend[5+transactionNumberLength] = DestinationId;
	if(Type == DATA)
	{
		bytesToSend[0]=0x12; //0b00010010;
		pduDataLength = NumberOfBytesToCode + 4;
	}
	else
	{
		bytesToSend[0]=0x2; //0b00000010;
		if(Type == METADATA)
		{
			pduDataLength = 8;
			for(int i=0; i<NumberOfParameters; i++)
			{
				pduDataLength += LengthOfParameter[i]+2;
			}
		}
		if(Type == EOFile)
		{
			pduDataLength = EOF_PACKET_LENGTH;
		}
		if(Type == ACK)
		{
			pduDataLength = ACK_PACKET_LENGTH;
		}
		if(Type == FIN)
		{
			pduDataLength = 5;
		}
		if(Type == NACK)
		{
			if(MissingSegment == 0)
			{
				return false;
			}
			pduDataLength = 9 + 8 * MissingSegment;
		}
	}
	bytesToSend[1] = ((pduDataLength>>8) & 0xFF);
	bytesToSend[2] = (pduDataLength & 0xFF);
	bytesToSend[3] = (transactionNumberLength-1);
	bytesToSend[4] = SourceId;

	return true;
}
/**
 * Code the Metadata Packet
 * @parameters:
 * Offset: where does the data go
 * DestinationId: The address to send the data to
 * Transaction: What kind of data it is
 * @return True if successful, false otherwise
 */
bool CFDP::CodeMetadata(unsigned long FileSize, uint8_t LengthOfParameter[], unsigned long Command[], uint8_t NumberOfParameters)
{
	int Length = BeginPacket+8;
	bytesToSend[BeginPacket] = 0x07;
	bytesToSend[BeginPacket+1] = 0x00;
	bytesToSend[BeginPacket+2] = ((FileSize>>24) & 0xFF);
	bytesToSend[BeginPacket+3] = ((FileSize>>16) & 0xFF);
	bytesToSend[BeginPacket+4] = ((FileSize>>8) & 0xFF);
	bytesToSend[BeginPacket+5] = (FileSize & 0xFF);
	bytesToSend[BeginPacket+6] = 0x00;
	bytesToSend[BeginPacket+7] = 0x00;
	for(int i=0; i<NumberOfParameters; i++)
	{
		if(LengthOfParameter[i] > 0 && LengthOfParameter[i] < 5)
		{
			bytesToSend[Length] = 0x00; //Only filestore request handled
			Length++;
			bytesToSend[Length] = LengthOfParameter[i];
			Length++;
			for(int j=0; j<LengthOfParameter[i]; j++)
			{
				bytesToSend[Length] = (Command[i]>>(8*(LengthOfParameter[i]-1-j))) & 0xFF;
				Length++;
			}
		}
	}
	return true;
}
/**
 * Code the Data Packet
 * @parameters:
 * Offset: where does the data go
 * bytesToCode: only used here to know the number of bytes to code
 * DestinationId: The address to send the data to
 * Transaction: What kind of data it is
 * @return True if successful, false otherwise
 */
bool CFDP::CodeDataPacket(unsigned long Offset, uint8_t bytesToCode[], uint8_t NumberOfBytesToCode, uint8_t DestinationId, unsigned long Transaction)
{
	CodePacketHeader(DATA, NumberOfBytesToCode, 0, 0, 0, DestinationId, Transaction);
	bytesToSend[BeginPacket] = ((Offset>>24) & 0xFF);
	bytesToSend[BeginPacket+1] = ((Offset>>16) & 0xFF);
	bytesToSend[BeginPacket+2] = ((Offset>>8) & 0xFF);
	bytesToSend[BeginPacket+3] = (Offset & 0xFF);
	for(int i=0; i<NumberOfBytesToCode; i++)
	{
		bytesToSend[BeginPacket+4+i] = bytesToCode[i];
	}
	return true;
}
/**
 * Code the End Of File (EOFile)
 * @parameters:
 * FileSize: total size of the file sent
 * Checksum: checksum of the packets sent
 * conditionCode: Status of the transmission
 * @return True if successful, false otherwise
 */
bool CFDP::CodeEndOfFile(unsigned long FileSize, unsigned long Checksum, ConditionCode conditionCode)
{
	bytesToSend[BeginPacket] = 0x04;
	bytesToSend[BeginPacket+1] = (conditionCode<<4);
	bytesToSend[BeginPacket+2] = ((Checksum>>24) & 0xFF);
	bytesToSend[BeginPacket+3] = ((Checksum>>16) & 0xFF);
	bytesToSend[BeginPacket+4] = ((Checksum>>8) & 0xFF);
	bytesToSend[BeginPacket+5] = (Checksum & 0xFF);
	bytesToSend[BeginPacket+6] = ((FileSize>>24) & 0xFF);
	bytesToSend[BeginPacket+7] = ((FileSize>>16) & 0xFF);
	bytesToSend[BeginPacket+8] = ((FileSize>>8) & 0xFF);
	bytesToSend[BeginPacket+9] = (FileSize & 0xFF);
	return true;
}
/**
 * Code the Acknowledge
 * @parameters:
 * PacketToAcknowledge: which packet to acknowledge (FIN or EOFile)
 * conditionCode: Status of the transmission
 * transactionStatus: Status of the transmission
 * @return True if successful, false otherwise
 */
bool CFDP::CodeAck(int PacketToAcknowledge, ConditionCode conditionCode, TransactionStatus transactionStatus)
{
	if(PacketToAcknowledge == EOFile)
	{
		bytesToSend[BeginPacket+1] = 0x40;
	}
	else if(PacketToAcknowledge == FIN)
	{
		bytesToSend[BeginPacket+1] = 0x51;
	}
	else
	{
		return false;
	}
	bytesToSend[BeginPacket] = 0x06;
	bytesToSend[BeginPacket+2] = (conditionCode<<4) + transactionStatus;
	return true;
}
/**
 * Code the finished packet
 * @parameters:
 * conditionCode: Status of the transmission
 * deliveryCode: Data complete/incomplete
 * fileStatus: Status of the file
 * Response: response to the data sent
 * @return True if successful, false otherwise
 */
bool CFDP::CodeFin(ConditionCode conditionCode, DeliveryCode deliveryCode, FileStatus fileStatus, uint8_t Response)
{
	bytesToSend[BeginPacket] = 0x05;
	bytesToSend[BeginPacket+1] = (conditionCode<<4) + 8 + (deliveryCode<<2) + fileStatus;
	bytesToSend[BeginPacket+2] = 0x01;
	bytesToSend[BeginPacket+3] = 0x01;
	bytesToSend[BeginPacket+4] = Response;
	return true;
}
/**
 * Code the negative acknowledge
 * @parameters:
 * StartOfScope: beginning of the missing data
 * EndOfScope: end of the missing data
 * StartOffset: array containing the beginning of the missing offset
 * EndOffset: array containing the end of the missing offset
 * MissingSegments: number of missing segments
 * @return True if successful, false otherwise
 */
bool CFDP::CodeNack(unsigned long StartOfScope, unsigned long EndOfScope, unsigned long StartOffset[],unsigned long EndOffset[], uint8_t MissingSegments)
{
	bytesToSend[BeginPacket] = 0x08;
	bytesToSend[BeginPacket+1] = ((StartOfScope>>24) & 0xFF);
	bytesToSend[BeginPacket+2] = ((StartOfScope>>16) & 0xFF);
	bytesToSend[BeginPacket+3] = ((StartOfScope>>8) & 0xFF);
	bytesToSend[BeginPacket+4] = (StartOfScope & 0xFF);
	bytesToSend[BeginPacket+5] = ((EndOfScope>>24) & 0xFF);
	bytesToSend[BeginPacket+6] = ((EndOfScope>>16) & 0xFF);
	bytesToSend[BeginPacket+7] = ((EndOfScope>>8) & 0xFF);
	bytesToSend[BeginPacket+8] = (EndOfScope & 0xFF);
	for(int i=0;i<MissingSegments; i++)
	{
		bytesToSend[BeginPacket+9+8*i] = ((StartOffset[i]>>24) & 0xFF);
		bytesToSend[BeginPacket+10+8*i] = ((StartOffset[i]>>16) & 0xFF);
		bytesToSend[BeginPacket+11+8*i] = ((StartOffset[i]>>8) & 0xFF);
		bytesToSend[BeginPacket+12+8*i] = (StartOffset[i] & 0xFF);
		bytesToSend[BeginPacket+13+8*i] = ((EndOffset[i]>>24) & 0xFF);
		bytesToSend[BeginPacket+14+8*i] = ((EndOffset[i]>>16) & 0xFF);
		bytesToSend[BeginPacket+15+8*i] = ((EndOffset[i]>>8) & 0xFF);
		bytesToSend[BeginPacket+16+8*i] = (EndOffset[i] & 0xFF);
	}
	return true;
}
/**
 * Sends Data
 * @parameters:
 * callback function: function to use to read the data
 * BeginOffset: The offset to start reading from
 * EndOffset: The offset to stop reading from (included)
 * DestinationId: The address to send the data to
 * Transaction: What kind of data it is
 * Protocol: the protocol used to send the data over
 * @return True if successful, false otherwise
 */
 bool CFDP::SendData(void (*callback)(uint16_t packPicDataSize, unsigned long Offset, uint8_t* pack), unsigned long BeginOffset, unsigned long EndOffset, uint8_t DestinationId, unsigned long Transaction, unsigned int Protocol)
{
	UnexpectedPacket = false;
	unsigned long Checksum = 0;
	uint8_t Data[DATA_PACKET_SIZE];
	int LengthOfBytesReceived = 0;
	char buffer[30];
	
	if(!CodePacketHeader(METADATA, 0, 0, 0, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeMetadata(EndOffset - BeginOffset + 1, 0, 0, 0))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[4])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	
	for(int i=1; (BeginOffset + i*DATA_PACKET_SIZE - 2 ) < EndOffset; i++)
	{
		callback(DATA_PACKET_SIZE, BeginOffset + (i-1)*DATA_PACKET_SIZE, Data);
		if(!CodeDataPacket((i-1)*DATA_PACKET_SIZE, Data, DATA_PACKET_SIZE, DestinationId, Transaction))
		{
			UnexpectedPacket = true;
			return false;
		}
		for(unsigned int j=0; j<pduDataLength; j++)
		{
			if((4*j)<pduDataLength)
			{
				Checksum += ((unsigned long)(bytesToSend[BeginPacket+4*j])>>24)
							+ ((unsigned long) (bytesToSend[BeginPacket+4*j+1])>>16)
							+ ((unsigned long)(bytesToSend[BeginPacket+4*j+2])>>8)
							+ bytesToSend[BeginPacket+4*j+3];
			}
		}
		#if defined(ARDUINO) && defined(DEBUG)
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[5])));
		Serial.print(buffer);
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
		Serial.println(buffer);
		#endif
		if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
		{
			UnexpectedPacket = true;
			return false;
		}
	}
	
	uint8_t DataLeft = (EndOffset - BeginOffset + 1)%DATA_PACKET_SIZE;
	if(DataLeft!=0)
	{
		uint8_t FinalData[DataLeft];
		callback(DataLeft, EndOffset - DataLeft + 1, FinalData);
		if(!CodeDataPacket(EndOffset - BeginOffset - DataLeft + 1 , FinalData, DataLeft, DestinationId, Transaction))
		{
			UnexpectedPacket = true;
			return false;
		}
		for(unsigned int j=0; j<pduDataLength; j++)
		{
			if((4*j)<pduDataLength)
			{
				Checksum += ((unsigned long)(bytesToSend[BeginPacket+4*j])>>24)
							+ ((unsigned long)(bytesToSend[BeginPacket+4*j+1])>>16)
							+ ((unsigned long)(bytesToSend[BeginPacket+4*j+2])>>8)
							+ bytesToSend[BeginPacket+4*j+3];
			}
		}
		#if defined(ARDUINO) && defined(DEBUG)
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[6])));
		Serial.print(buffer);
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
		Serial.println(buffer);
		#endif
		if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
		{
			UnexpectedPacket = true;
			return false;
		}		
	}
	
	if(!CodePacketHeader(EOFile, 0, 0, 0, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeEndOfFile(EndOffset - BeginOffset + 1, Checksum, NoError))
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[7])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	Delay_ms(100);
	
	LengthOfBytesReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[8])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	if((bytesReceived[4] == DestinationId) && (bytesReceived[ReceivedHeaderLength + 1] == 0x40) && ((bytesReceived[ReceivedHeaderLength + 2]&0x03) == 1))
	{
		#if defined(ARDUINO) && defined(DEBUG)
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[11])));
		Serial.println(buffer);
		#endif
		int Condition = bytesReceived[ReceivedHeaderLength + 2]>>4;
		if(Condition !=0)
		{
			//To be implemented
		}
	}
	else
	{
		UnexpectedPacket = true;
		return false;
	}

	Delay_ms(100);
	LengthOfBytesReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	if(bytesReceived[4] != DestinationId)
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[9])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
	Serial.println(buffer);
	Serial.println(LengthOfBytesReceived);
	#endif
	if(TypeOfMessage == NACK) //Not FIN
	{
		uint16_t NumberOfMissingElement = (LengthOfBytesReceived-9)/8;
		unsigned long StartOffset[NumberOfMissingElement];
		unsigned long StopOffset[NumberOfMissingElement];
		for(unsigned int i=0;i<NumberOfMissingElement; i++)
		{
			StartOffset[i]=((unsigned long)(bytesReceived[ReceivedHeaderLength + 9+8*i])<<24)
							+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 10+8*i])<<16)
							+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 11+8*i])<<8)
							+ bytesReceived[ReceivedHeaderLength + 12+8*i];
			StopOffset[i]=((unsigned long)(bytesReceived[ReceivedHeaderLength + 13+8*i])<<24)
							+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 14+8*i])<<16)
							+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 15+8*i])<<8)
							+ bytesReceived[ReceivedHeaderLength + 16+8*i];
			if (StopOffset[i]>(EndOffset - BeginOffset + 1)) StopOffset[i] = (EndOffset - BeginOffset + 1);
		}
		if((StartOffset[0] == 0) && (StopOffset[0] == 0)) //Missing Metadata
		{
			if(!CodePacketHeader(METADATA, 0, 0, 0, 0, DestinationId, Transaction))
			{
				UnexpectedPacket = true;
				return false;
			}
			if(!CodeMetadata(EndOffset - BeginOffset + 1, 0, 0, 0))
			{
				UnexpectedPacket = true;
				return false;
			}
			if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
			{
				UnexpectedPacket = true;
				return false;
			}
			
			for(unsigned int i=1; i<NumberOfMissingElement; i++)
			{
				callback(DATA_PACKET_SIZE, BeginOffset + StartOffset[i], Data);
				if(!CodeDataPacket(StartOffset[i], Data, DATA_PACKET_SIZE, DestinationId, Transaction))
				{
					UnexpectedPacket = true;
					return false;
				}
				if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
				{
					UnexpectedPacket = true;
					return false;
				}
			}
		}
		else
		{
			for(unsigned int i=0; i<NumberOfMissingElement; i++)
			{
				callback(DATA_PACKET_SIZE, BeginOffset + StartOffset[i], Data);
				if(!CodeDataPacket(StartOffset[i], Data, DATA_PACKET_SIZE, DestinationId, Transaction))
				{
					UnexpectedPacket = true;
					return false;
				}				
				if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
				{
					UnexpectedPacket = true;
					return false;
				}				
			}
		}

		LengthOfBytesReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
		if(bytesReceived[4] != DestinationId)
		{
			UnexpectedPacket = true;
			return false;
		}
	}
	if(TypeOfMessage == FIN)
	{
		if((bytesReceived[ReceivedHeaderLength + 1]&0x04)!=0)
		{
			#if defined(ARDUINO) && defined(DEBUG)
			Serial.println(bytesReceived[ReceivedHeaderLength + 1]);
			#endif
			UnexpectedPacket = true;
			return false;
		}
		if(!CodePacketHeader(ACK, 0, 0, 0, 0, DestinationId, Transaction))
		{
			UnexpectedPacket = true;
			return false;
		}
		if(!CodeAck(FIN, NoError, Active))
		{
			UnexpectedPacket = true;
			return false;
		}
		if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
		{
			UnexpectedPacket = true;
			return false;
		}
		#if defined(ARDUINO) && defined(DEBUG)
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[10])));
		Serial.print(buffer);
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
		Serial.println(buffer);
		#endif
	}
	else
	{
		UnexpectedPacket = true;
		return false;
	}
	UnexpectedPacket = true;
	return true;
}
/**
 * ReceiveData
 * @parameters:
 * callback function: function to use to read the data
 * BeginOffset: The offset to start reading from
 * DestinationId: The address to send the data to
 * Transaction: What kind of data it is
 * Protocol: the protocol used to receive the data 
 * @return True if successful, false otherwise
 */
 bool CFDP::ReceiveData(void (*callback)(uint16_t packPicDataSize, unsigned long Offset, uint8_t* pack), unsigned long BeginOffset, uint8_t DestinationId, unsigned long Transaction, unsigned int Protocol)
{
	unsigned long MissingBeginOffset[MAX_MISSING_SEGMENT];
	unsigned long MissingEndOffset[MAX_MISSING_SEGMENT];
	ConditionCode Condition = NoError;
	DeliveryCode Delivery = DataComplete;
	FileStatus Status = FileRetained;
	uint8_t NumberOfMissingSegment = 0;
	int LengthOfBytesReceived = 0;
	unsigned long PreviousOffset = 0;
	unsigned long FileSize = 0;
	unsigned long Checksum = 0;
	unsigned long ReceivedChecksum = 0;
	unsigned long ExpectedFileSize = 0;
	char buffer[30];
	bool IsEof = false;
	UnexpectedPacket = false;

	LengthOfBytesReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[4])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
	Serial.println(buffer);
	#endif
	if(LengthOfBytesReceived < 0)
	{
		UnexpectedPacket = true;
		return false;
	}
	else if(LengthOfBytesReceived == 0)
	{
		if(NumberOfMissingSegment < MAX_MISSING_SEGMENT)
		{
			MissingBeginOffset[NumberOfMissingSegment] = 0;
			MissingEndOffset[NumberOfMissingSegment] = 0;
		}
		NumberOfMissingSegment++;
	}
	else
	{
		ExpectedFileSize = ((unsigned long)(bytesReceived[ReceivedHeaderLength + 2])<<24)
							+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 3])<<16)
							+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 4])<<8)
							+ bytesReceived[ReceivedHeaderLength + 5];
	}
	while(!IsEof)
	{
		LengthOfBytesReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
		if(LengthOfBytesReceived < 0)
		{
			UnexpectedPacket = true;
			return false;
		}
		if(LengthOfBytesReceived == 0)
		{
			if(NumberOfMissingSegment < MAX_MISSING_SEGMENT)
			{
				MissingBeginOffset[NumberOfMissingSegment] = PreviousOffset;
				MissingEndOffset[NumberOfMissingSegment] = PreviousOffset + LengthOfBytesReceived-4;
			}
			NumberOfMissingSegment++;
		}
		else if(LengthOfBytesReceived == EOF_PACKET_LENGTH && TypeOfMessage == EOFile) 
		{
			IsEof = true;
			#if defined(ARDUINO) && defined(DEBUG)
			strcpy_P(buffer, (char*)pgm_read_word(&(message_table[7])));
			Serial.print(buffer);
			strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
			Serial.println(buffer);
			#endif
			FileSize = ((unsigned long)(bytesReceived[ReceivedHeaderLength + 6])<<24)
						+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 7])<<16)
						+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 8])<<8)
						+ bytesReceived[ReceivedHeaderLength + 9];
			if((ExpectedFileSize != 0) && (ExpectedFileSize != FileSize))
			{
				Condition = SizeError;
			}
			ReceivedChecksum = ((unsigned long)(bytesReceived[ReceivedHeaderLength + 2])<<24)
						+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 3])<<16)
						+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 4])<<8)
						+ bytesReceived[ReceivedHeaderLength + 5];
			if(ReceivedChecksum != Checksum)
			{
				Condition = ChecksumFailed;
			}
		}
		else if(TypeOfMessage == DATA)
		{
			uint8_t DataToStore[(bytesReceived[1]>>8)+bytesReceived[2]-4];
			#if defined(ARDUINO) && defined(DEBUG)
			strcpy_P(buffer, (char*)pgm_read_word(&(message_table[5])));
			Serial.print(buffer);
			strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
			Serial.println(buffer);
			#endif
			for(int i=0; i<LengthOfBytesReceived-4;i++)
			{
				DataToStore[i] = bytesReceived[ReceivedHeaderLength + i + 4];
			}
			for(int j=0; j<(LengthOfBytesReceived-4); j++)
			{
				if((4*j)<(LengthOfBytesReceived-4))
				{
					Checksum += ((unsigned long)(DataToStore[4*j])>>24)
								+ ((unsigned long) (DataToStore[4*j+1])>>16)
								+ ((unsigned long)(DataToStore[4*j+2])>>8)
								+ DataToStore[4*j+3];
				}
			}
			callback(LengthOfBytesReceived - 4, BeginOffset + PreviousOffset, DataToStore);
			PreviousOffset += LengthOfBytesReceived-4;
		}
		else
		{
			UnexpectedPacket = true;
			return false;			
		}
	}
	if(!CodePacketHeader(ACK, 0, 0, 0, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeAck(EOFile, NoError, Active))
	{
		UnexpectedPacket = true;
		return false;
	}
	Delay_ms(100);
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[8])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	//Send EOF acknowledgement
	if(!SendDataOver(bytesToSend, BeginPacket + 3, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(NumberOfMissingSegment>0)
	{
		if(NumberOfMissingSegment>MAX_MISSING_SEGMENT) 
		{
			Condition = NakLimit;
			Delivery = DataIncomplete;
		}
		else
		{
			if(!CodePacketHeader(NACK, 0, 0, 0, NumberOfMissingSegment, DestinationId, Transaction))
			{
				UnexpectedPacket = true;
				return false;
			}
			if(!CodeNack(MissingBeginOffset[0], MissingEndOffset[NumberOfMissingSegment-1], MissingBeginOffset, MissingEndOffset, NumberOfMissingSegment))
			{
				UnexpectedPacket = true;
				return false;
			}
			Delay_ms(100);
			//Send NACK
			if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
			{
				UnexpectedPacket = true;
				return false;
			}
			while(NumberOfMissingSegment>0)
			{
				LengthOfBytesReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
				if((MissingBeginOffset[0]==0) && (MissingEndOffset[0]==0))
				{
					if(TypeOfMessage != METADATA)
					{
						UnexpectedPacket = true;
						return false;
					}
					ExpectedFileSize = ((unsigned long)(bytesReceived[ReceivedHeaderLength + 2])<<24)
								+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 3])<<16)
								+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 4])<<8)
								+ bytesReceived[ReceivedHeaderLength + 5];
					if((ExpectedFileSize != 0) && (ExpectedFileSize != FileSize))
					{
						Condition = SizeError;
					}
					MissingEndOffset[0] = 1; //Avoid waiting only for Metadata
				}
				else
				{
					uint8_t DataToStore[(bytesReceived[1]>>8)+bytesReceived[2]-4];
					for(int i=0; i < LengthOfBytesReceived - 4;i++)
					{
						DataToStore[i] = bytesReceived[ReceivedHeaderLength + i + 4];
					}
					PreviousOffset = ((unsigned long)(bytesReceived[ReceivedHeaderLength])<<24)
									+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 1])<<16)
									+ ((unsigned long)(bytesReceived[ReceivedHeaderLength + 2])<<8)
									+ bytesReceived[ReceivedHeaderLength + 3];
					callback(LengthOfBytesReceived-4,BeginOffset + PreviousOffset, DataToStore);
				}
				NumberOfMissingSegment--;
			}
		}
	}
	
	if(!CodePacketHeader(FIN, 0, 0, 0, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeFin(Condition, Delivery, Status, 0))
	{
		UnexpectedPacket = true;
		return false;
	}
	Delay_ms(100);
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[9])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	//Send FIN
	if(!SendDataOver(bytesToSend, BeginPacket + 5, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	
	//Receive FIN ack
	LengthOfBytesReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	if(LengthOfBytesReceived!= ACK_PACKET_LENGTH && TypeOfMessage == ACK) 
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[10])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
	Serial.println(buffer);
	#endif
	Delay_ms(100);
	UnexpectedPacket = true;
	return true;
}
/**
 * Send Data Over. Send data over the chosen protocol
 * @parameters:
 * data: array of bytes to send
 * length: the number of bytes to send
 * address: the address to which the data is sent
 * protocol: the protocol used to write the data (only I2C implemented yet)
 * @return true if the data were correctly sent
 */
bool CFDP::SendDataOver(uint8_t data[], uint8_t length, uint8_t address, unsigned int protocol)
{
	char buffer[30];
	Delay_ms(100);
	#ifdef ARDUINO
	if(protocol == I2C)
	{
		#ifdef DEBUG
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[12])));
		Serial.println(buffer);
		#endif
		for(int j = 0; j < length; j+=32)
		{
			int k = 0;
			if((length - j)>32) k=32;
			else k = (length - j);
			Delay_ms(100);
			Wire.beginTransmission(address);
			for(int i = 0; i < k; i++)
			{
				Wire.write(data[j+i]);
				#ifdef DEBUG
				Serial.print(data[j+i]);
				Serial.print(" ");
				#endif
			}
			int i = 0;
			i = Wire.endTransmission();
			if(i!=0) 
			{
				#ifdef DEBUG
				strcpy_P(buffer, (char*)pgm_read_word(&(message_table[13])));
				Serial.print(buffer);
				Serial.println(i);
				#endif
				return false;
			}
		}
	}
	#endif
	return true;
}
/**
 * ReceiveDataOver. Receive data over the chosen protocol
 * @parameters:
 * data: array of bytes received
 * length: the number of bytes received
 * address: the addressing which is sending the data (not yet in use)
 * protocol: the protocol used to read the data (only I2C implemented yet)
 * @return the number of bytes received
 */
int CFDP::ReceiveDataOver(uint8_t data[], uint8_t address, unsigned int protocol)
{
	int LengthOfReceivedBytes = 1;
	int LengthOfReceivedData = 0;
	unsigned int k = 0;
	int j = 0;
	char buffer[30];
	#ifdef ARDUINO
	if(protocol == I2C)
	{
		for(int l = 0; l < LengthOfReceivedBytes; l+=32)
		{

			while(!ReceiveFlag && j< 1000 * I2C_MAX_WAITING_TIME)
			{
				Delay_ms(1);
				j++;
			}
			if(!ReceiveFlag)
			{
				#ifdef DEBUG
				strcpy_P(buffer, (char*)pgm_read_word(&(message_table[14])));
				Serial.println(buffer);
				#endif
				return -1;
			}
			else
			{
				if(l == 0) 
				{
					LengthOfReceivedData = (bytesReceivedOverI2C[1]<<8) + bytesReceivedOverI2C[2];
					ReceivedLengthOfID = ((bytesReceivedOverI2C[3]>>4)& 0x7) + 1;
					ReceivedLengthOfTransaction = (bytesReceivedOverI2C[3] & 0x7) + 1;
					ReceivedHeaderLength = 4 + 2 * ReceivedLengthOfID + ReceivedLengthOfTransaction;
					LengthOfReceivedBytes = LengthOfReceivedData + ReceivedHeaderLength;
					TypeOfMessage = bytesReceivedOverI2C[ReceivedHeaderLength];
					if ((bytesReceivedOverI2C[0] & 0x10) == 0x10) TypeOfMessage = DATA;
				}
				if((LengthOfReceivedBytes - l)>32) k=32;
				else k = (LengthOfReceivedBytes - l);
				ReceiveFlag = false;
				if(k != ReceivedBytes)
				{
					#ifdef DEBUG
					strcpy_P(buffer, (char*)pgm_read_word(&(message_table[15])));
					Serial.print(buffer);
					Serial.print(ReceivedBytes);
					strcpy_P(buffer, (char*)pgm_read_word(&(message_table[16])));
					Serial.print(buffer);
					Serial.println(k);
					#endif
					return 0;
				}
				for(unsigned int i=0; i < k; i++)
				{
				
					data[l+i] = bytesReceivedOverI2C[i];
					#ifdef DEBUG
					Serial.print(bytesReceivedOverI2C[i]);
					Serial.print(" ");
					#endif
				}
				k = 0;
				j = 0;
			}
		}
	}
	#endif
	return LengthOfReceivedData;
}
/**
 * SendCommand
 * @parameters:
 * Transaction: What kind of data it is
 * Parameters: the command parameters to send
 * LengthOfParameter: the length of each parameter (1 to 4 bytes)
 * NumberOfParameters: the number of parameters
 * DestinationId: The address to send the data to
 * Protocol: the protocol used to receive the command
 * @return True if successful, false otherwise
 */
 bool CFDP::SendCommand(unsigned long Transaction, unsigned long Parameters[], uint8_t LengthOfParameter[], uint8_t NumberOfParameters, uint8_t DestinationId, unsigned int Protocol)
{
	char buffer[30];
	UnexpectedPacket = false;
	//int NextParameter = BeginPacket+9;
	int LengthOfDataReceived = 0;
	if(!CodePacketHeader(METADATA, 0, LengthOfParameter, NumberOfParameters, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeMetadata(0, LengthOfParameter, Parameters, NumberOfParameters))
	{
		UnexpectedPacket = true;
		return false;
	}
	//Send Metadata
	if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[4])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	
	Delay_ms(100);	
	if(!CodePacketHeader(EOFile, 0, 0, 0, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeEndOfFile(0, 0, NoError))
	{
		UnexpectedPacket = true;
		return false;
	}
	//Send End Of File
	if(!SendDataOver(bytesToSend, BeginPacket + pduDataLength, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[7])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	Delay_ms(100);
	//Receive End Of File Acknowledgement
	LengthOfDataReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	if(LengthOfDataReceived <= 0)
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[8])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
	Serial.println(buffer);
	#endif
	if((bytesReceived[4] == DestinationId) && (bytesReceived[ReceivedHeaderLength + 1] == 0x40) && ((bytesReceived[ReceivedHeaderLength + 2]&0x03) == 1))
	{
		#if defined(ARDUINO) && defined(DEBUG)
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[11])));
		Serial.println(buffer);
		#endif
		int Condition = bytesReceived[ReceivedHeaderLength + 2]>>4;
		if(Condition !=0)
		{
			//To be implemented
		}
	}
	else
	{
		//Ack not accepted
		#if defined(ARDUINO) && defined(DEBUG)
		strcpy_P(buffer, (char*)pgm_read_word(&(message_table[17])));
		Serial.println(buffer);
		#endif
		UnexpectedPacket = true;
		return false;
	}
	Delay_ms(100);
	//Receive FIN
	LengthOfDataReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	if(LengthOfDataReceived <= 0)
	{
		UnexpectedPacket = true;
		return false;
	}
	if(bytesReceived[4] != DestinationId || ((bytesReceived[ReceivedHeaderLength+1]&0x04)!=0))
	{
		#if defined(ARDUINO) && defined(DEBUG)
		Serial.println(bytesReceived[ReceivedHeaderLength+1]);
		#endif
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[9])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
	Serial.println(buffer);
	Serial.println(LengthOfDataReceived);
	#endif
	
	if(!CodePacketHeader(ACK, 0, 0, 0, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeAck(FIN, NoError, Active))
	{
		UnexpectedPacket = true;
		return false;
	}
	//Send FIN Acknowledge
	if(!SendDataOver(bytesToSend, pduDataLength + BeginPacket, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[10])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	
	UnexpectedPacket = true;
	return true;
}
/**
 * ReceivedCommand
 * @parameters:
 * callback function: function to use to read the command
 * DestinationId: The address to send the data to
 * Protocol: the protocol used to receive the command
 * @return True if successful, false otherwise
 */
 bool CFDP::ReceivedCommand(void (*callback)(unsigned long Command, unsigned long Parameter[], uint8_t NumberOfParameters, uint8_t DestinationId), uint8_t DestinationId, unsigned int Protocol)
{
	char buffer[30];
	UnexpectedPacket = false;
	int LengthOfDataReceived = 0;
	unsigned long Transaction = 0;
	int NextParameter = 9;
	int NbOfParameters = 0;
	uint8_t StartOfParameters = 0;
	//Receive Metadata
	LengthOfDataReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	if(LengthOfDataReceived <= 0 && TypeOfMessage != METADATA)
	{
		UnexpectedPacket = true;
		return false;
	}
	for(int i=0; i<ReceivedLengthOfTransaction; i++)
	{
		Transaction += (bytesReceived[4+ReceivedLengthOfID+i]>>(8*(ReceivedLengthOfTransaction - i - 1)) & 0xFF);
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[4])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
	Serial.println(buffer);
	#endif
	StartOfParameters = 10 + ReceivedHeaderLength + bytesReceived[ReceivedHeaderLength + 6] + bytesReceived[ReceivedHeaderLength + 7 + bytesReceived[ReceivedHeaderLength + 6]];
	NextParameter = StartOfParameters-1;
	while((LengthOfDataReceived + ReceivedHeaderLength) > NextParameter) 
	{
		if(bytesReceived[NextParameter]>0 && bytesReceived[NextParameter]<5)
		{
			NextParameter += bytesReceived[NextParameter] + 2;
			NbOfParameters++;
		}
		else
		{
			#if defined(ARDUINO) && defined(DEBUG)
			Serial.println(bytesReceived[NextParameter]);
			strcpy_P(buffer, (char*)pgm_read_word(&(message_table[18])));
			Serial.println(buffer);
			#endif
			UnexpectedPacket = true;
			return false;//discard the metadata
		}
		#if defined(ARDUINO) && defined(DEBUG)
		Serial.println(NbOfParameters);
		#endif
	}
	unsigned long Parameters[NbOfParameters];
	NextParameter = StartOfParameters-1; 
	for(int i=0; i<NbOfParameters; i++)
	{
		Parameters[i] = 0;
		for(int j=0; j<bytesReceived[NextParameter]; j++)
		{
			Parameters[i] += (bytesReceived[NextParameter+1+j]>>(8*(bytesReceived[NextParameter+1]-1-j))) & 0xFF;
		}
		NextParameter += bytesReceived[NextParameter] + 1;
	}
	CommanderId = DestinationId;
	
	//Receive EOFile
	LengthOfDataReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	if(LengthOfDataReceived != EOF_PACKET_LENGTH && TypeOfMessage != EOFile)
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[7])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
	Serial.println(buffer);
	#endif
	
	if(!CodePacketHeader(ACK, 0, 0, 0, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeAck(EOFile, NoError, Active))
	{
		UnexpectedPacket = true;
		return false;
	}
	Delay_ms(100);
	//Send EOFile Acknowledgement
	if(!SendDataOver(bytesToSend, BeginPacket + 3, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[8])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	
	if(!CodePacketHeader(FIN, 0, 0, 0, 0, DestinationId, Transaction))
	{
		UnexpectedPacket = true;
		return false;
	}
	if(!CodeFin(NoError, DataComplete, FileRetained, 0))
	{
		UnexpectedPacket = true;
		return false;
	}
	Delay_ms(100);
	//Send FIN
	if(!SendDataOver(bytesToSend, BeginPacket + 5, DestinationId, Protocol))
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[9])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[2])));
	Serial.println(buffer);
	#endif
	Delay_ms(100);
	
	//Receive FIN ack
	LengthOfDataReceived = ReceiveDataOver(bytesReceived, DestinationId, Protocol);
	if(LengthOfDataReceived != ACK_PACKET_LENGTH && TypeOfMessage != ACK) 
	{
		UnexpectedPacket = true;
		return false;
	}
	#if defined(ARDUINO) && defined(DEBUG)
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[10])));
	Serial.print(buffer);
	strcpy_P(buffer, (char*)pgm_read_word(&(message_table[3])));
	Serial.println(buffer);
	#endif
	UnexpectedPacket = true;
	callback(Transaction, Parameters, NbOfParameters, DestinationId);
	return true;
}
/**
 * Receive Packet. This function is called as soon as data are available in the I2C RX buffer.
 * THIS FUNCTION IS ONLY USED FOR THE I2C PROTOCOL
 * @parameters:
 * howMany: the number of bytes we are receiving
 * @return None
 */
#ifdef ARDUINO
void ReceivePacket(int howMany)
{
	ReceiveFlag = true;
	unsigned int ByteRead = 0;
	while(Wire.available())
	{
		bytesReceivedOverI2C[ByteRead] = Wire.read();
		ByteRead++;
	}
    ReceivedBytes = ByteRead;
	if(UnexpectedPacket) 
	{
		ReceivedUnexpectedPacket = true;
	}
}
#endif
/**
 * Delay_ms
 * @parameters:
 * timeToWait: the time to wait in milliseconds
 * @return None
 */
void CFDP::Delay_ms(uint16_t timeToWait)
{
	#ifdef ARDUINO
	delay(timeToWait);
	#else
//	sleep(timeToWait);
	#endif
}

/*
 * DllMain. Main function for the DLL
 * @parameters:
 * hInst: Library instance handle
 * reason: Reason this function is being called
 * reserved: Not used
 */
#ifndef ARDUINO
BOOL APIENTRY DllMain (HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}
#endif
