

#ifndef PACKET_H_
#define PACKET_H_

#include "WProgram.h"

#define PACKET_VERSION_1			0x00 //0b00000000
#define PACKET_TYPE_TELEMETRY		0x00 //0x00 //0b00000000		// write/respond
#define PACKET_TYPE_TELECOMMAND		0x01 //0x10 //0b00010000		// read/request
#define PACKET_TYPE_POS				0x04
#define PACKET_HAS_SECONDARY_HDR	0x01 //0x08 //0b00001000
#define PACKET_SEC_HDR_POS			0x03
#define PACKET_NO_SECONDARY_HDR		0x00 //0b00000000

#define PACKET_SEQ_FLAG_CONTINUE	0x00 //0x00 //0b00000000
#define PACKET_SEQ_FLAG_FIRST		0x01 //0x40 //0b01000000
#define PACKET_SEQ_FLAG_LAST		0x02 //0x80 //0b10000000
#define PACKET_SEQ_FLAG_UNSEGMENTED	0x03 //0xc0  //0b11000000
#define PACKET_SEQ_FLAG_POS			0x06 // bit position
 
#define PACKET_DATA_POS				0x06 // byte position

#define PACKET_PRIMARY_HDR_SIZE		0x06
#define MAX_DATA_SIZE				26
#define MAX_PACKET_SIZE				PACKET_PRIMARY_HDR_SIZE+MAX_DATA_SIZE

int16_t create_primary_packet_header(
		uint8_t* buf,
		uint8_t packet_type, 
		uint16_t data_length, 
		uint8_t secondary_header, 
		uint16_t app_id, 
		uint8_t sequence_flags, 
		uint16_t sequence_num);


uint8_t get_packet_type(uint8_t* packet);
uint16_t get_data_length(uint8_t* packet);
uint8_t has_secondary_hdr(uint8_t* packet);
uint16_t get_app_id(uint8_t* packet);
uint8_t get_sequence_flags(uint8_t* packet);
uint16_t get_sequence_number(uint8_t* packet);


uint16_t test_function(uint8_t num);


/*

class Packet {

public:
    Packet();
    
    uint8_t* get_header_buf();
    
	int16_t set_packet_header(
		uint8_t packet_type, 
		uint16_t data_length, 
		uint8_t secondary_header, 
		uint16_t app_id, 
		uint8_t sequence_flags, 
		uint16_t sequence_num);


private:
    uint8_t header[6];
    
	uint8_t get_packet_type(uint8_t* packet);
	uint16_t get_data_length(uint8_t* packet);
	uint8_t has_secondary_hdr(uint8_t* packet);
	uint16_t get_app_id(uint8_t* packet);
	uint8_t get_sequence_flags(uint8_t* packet);
	uint16_t get_sequence_number(uint8_t* packet);

};
*/


#endif
