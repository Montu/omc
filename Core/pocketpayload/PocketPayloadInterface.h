
#ifndef POCKETPAYLOAD_PUBLIC_H_
#define POCKETPAYLOAD_PUBLIC_H_

#include "WProgram.h"

// SYSTEM COMMANDS
#define MPQ_SYSTEM_TIME             0x01
#define MPQ_DATA_PACKET             0x02

// POCKET PAYLOAD APP ID (LDP)
#define MPQ_FLIGHT_COMPUTER_ADDRESS 0x03
#define MPQ_ORBIT_VIEW_ADDRESS      0x04
#define MPQ_QUBDUINO_ADDRESS        0x05
#define MPQ_OPENSPACE365_ADDRESS    0x06
#define MPQ_SUPERLAB                0x07
#define MPQ_SUPERSPRITE             0x08
#define MPQ_IQEA                    0x09
#define MPQ_GROUND                  0x10


// //////////////////////////////////////////////////////////
// Must call the following function in the pocket payload 
// setup
/////////////////////////////////////////////////////////////

void initialise_pocketpayloadinterface(uint8_t device_address, uint8_t use_timer);


// //////////////////////////////////////////////////////////
// Must implement the following function to process incoming
// data/command packets
/////////////////////////////////////////////////////////////

// function to process incoming packets from Earth or the flight
// computer
void process_packet(
    uint8_t* data,
    uint16_t data_length, 
    uint8_t has_header, 
    uint8_t sequence_flags, 
    uint16_t sequence_number);

// //////////////////////////////////////////////////////////
// Optional functions that can be called
/////////////////////////////////////////////////////////////

// get the time
uint32_t get_time(void);

// function to send packets to the flight computer or to Earth.
int16_t send_packet(
    uint16_t destination, 
    uint8_t type,
    uint8_t* data, 
    uint16_t data_length, 
    uint8_t has_header,
    uint16_t max_sequence_number, 
    uint16_t sequence_number);


#endif