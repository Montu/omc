/**
 * Copyright 2012
 * Johan Marx
 */

#ifndef CFDP_h
#define CFDP_h

#include <inttypes.h>
#if defined(ARDUINO) && ARDUINO >= 100  
	#include "Arduino.h"
	#define EXTERN_CFDP_DLL
	#define CFDP_DLL
#elif defined(ARDUINO)
	#include "WProgram.h"
	#include "WConstants.h"
	#define EXTERN_CFDP_DLL
	#define CFDP_DLL
#else
	#ifdef BUILDING_DLL
		#define EXTERN_CFDP_DLL extern "C" __declspec(dllexport)
		#define CFDP_DLL __declspec(dllexport)
	#else
		#define EXTERN_CFDP_DLL extern "C" __declspec(dllimport)
		#define CFDP_DLL __declspec(dllimport)
	#endif
#endif

#define I2C 1

#define MAX_PACKET_LENGTH 80

#define MAX_MISSING_SEGMENT 8

#define MAX_COMMAND_PARAMETERS 10

//Maximum waiting time in seconds
#define I2C_MAX_WAITING_TIME 5

#define ACK_PACKET_LENGTH 3 

#define EOF_PACKET_LENGTH 10
// The data packet size (must be a multiple of 4)
#define DATA_PACKET_SIZE 40
// I²C Master
#define MASTER 1
// I²C Slave
#define SLAVE 0

// Data packet
#define DATA 3
// EOF packet
#define EOFile 4
// FIN packet
#define FIN 5
// ACK packet
#define ACK 6
// Metadata packet
#define METADATA 7
// NACK packet
#define NACK 8

/**
 * Provides a driver interface to the C328R camera from COMedia Ltd.
 */
class CFDP_DLL CFDP
{
  public:
    CFDP();
    bool initiate(uint8_t, bool, unsigned int);
	bool SendData(void (*)(uint16_t, unsigned long, uint8_t[]), unsigned long, unsigned long, uint8_t, unsigned long, unsigned int);
	bool ReceiveData(void (*)(uint16_t, unsigned long, uint8_t*), unsigned long, uint8_t, unsigned long, unsigned int);
	bool ReceivedCommand(void (*)(unsigned long, unsigned long[], uint8_t, uint8_t), uint8_t, unsigned int);
	bool SendCommand(unsigned long, unsigned long[], uint8_t[], uint8_t, uint8_t, unsigned int);
	
	enum TransactionStatus
	{
		Undefined = 0x00,
		Active = 0x01,
		Terminated = 0x02,
		Unrecognized = 0x03
	};
	enum ConditionCode
    {
		NoError = 0x00,
		AckLimit = 0x01,
		KeepAliveLimit = 0x02,
		InvalidTransmissionMode = 0x03,
		FilestoreRejection = 0x04,
		ChecksumFailed = 0x05,
		SizeError = 0x06,
		NakLimit = 0x07,
		Inactivity = 0x08,
		InvalidFileStructure = 0x09,
		//SuspendRequest = 0x0E,
		//CancelRequest = 0x0F
    };
	enum DeliveryCode
	{
		DataComplete = 0x00,
		DataIncomplete = 0x01
	};
	enum FileStatus
	{
		FileDiscardedDeliberately = 0x00,
		FileDiscarded = 0x01,
		FileRetained = 0x02,
		FileUnreported = 0x03
	};
  
	bool CodePacketHeader(int, uint8_t, uint8_t[], uint8_t, uint8_t, uint8_t, unsigned long);
	bool CodeMetadata(unsigned long, uint8_t[], unsigned long[], uint8_t);
	bool CodeDataPacket(unsigned long, uint8_t[], uint8_t, uint8_t, unsigned long);
	bool CodeEndOfFile(unsigned long, unsigned long, ConditionCode);
	bool CodeAck(int, ConditionCode, TransactionStatus);
	bool CodeFin(ConditionCode, DeliveryCode, FileStatus, uint8_t);
	bool CodeNack(unsigned long, unsigned long, unsigned long[],unsigned long[], uint8_t);
	uint8_t bytesToSend[MAX_PACKET_LENGTH];
	int SourceId;
	unsigned int pduDataLength;
	private:
	bool SendDataOver(uint8_t[], uint8_t, uint8_t, unsigned int);
	int ReceiveDataOver(uint8_t[], uint8_t, unsigned int);
	void Delay_ms(uint16_t);
	
};
void HandleCommand(unsigned long, unsigned long[], uint8_t, uint8_t);
#ifdef ARDUINO
void ReceivePacket(int);
#endif
extern bool UnexpectedPacket;
extern bool ReceivedUnexpectedPacket;
extern bool ReceiveFlag;
extern uint8_t bytesReceived[MAX_PACKET_LENGTH];
extern uint8_t bytesReceivedOverI2C[32];
extern unsigned int ReceivedBytes;
extern long Parameter[MAX_COMMAND_PARAMETERS];
extern CFDP cfdp;

#endif
