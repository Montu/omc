#include "WProgram.h"
#include "Packet.h"


int16_t create_primary_packet_header(
        uint8_t* primary_header,
		uint8_t packet_type, 
		uint16_t data_length, 
		uint8_t secondary_header, 
		uint16_t app_id,
		uint8_t sequence_flags, 
		uint16_t sequence_num)  {

	if (primary_header == 0)
		return -1;
        
	// AppID MAX SIZE is 11 bits - 0x7FF or 2047
	if (app_id >= 2047)
		return 1;

	// sequence_num MAX is 14 bits - 0x3FFF or 16383
	if (sequence_num >= 16383)
		return 2;

	// set packet version
	primary_header[0] |= PACKET_VERSION_1;

	// set packet type
	primary_header[0] |= (( packet_type & 0x01 ) << PACKET_TYPE_POS);/*0b00010000*/ 

	// set secondary header
	primary_header[0] |= (( secondary_header & 0x01) << PACKET_SEC_HDR_POS ); /*0b00001000*/ 

	// set top 3 bits of AppId
	uint8_t top_bits = 0;
	top_bits = ((app_id >> 8) & 0x07 /*0b00000111*/);
	primary_header[0] |= top_bits;

	// set lower 8 bits
	primary_header[1] |= (app_id & 0xff);

	// set sequence flags 
	primary_header[2] |= ((sequence_flags & 0x03 ) << PACKET_SEQ_FLAG_POS); /*0b11000000*/

	// set sequence number high bits
	top_bits = 0;
	top_bits = ((sequence_num >> 8) & 0x3f /*0b00111111*/);
	primary_header[2] |= top_bits;

	// set lower 8 bits
	primary_header[3] |= (sequence_num & 0xff);

	// set packet data length
	primary_header[4] = (data_length >> 8);
	primary_header[5] = primary_header[5] + (data_length & 0xff);

	return 0;
}

uint8_t get_packet_type(uint8_t* packet)
{
	if (packet == 0)
		return 0;

	uint8_t packet_type = ((packet[0] >> PACKET_TYPE_POS) & 0x01);

	return packet_type;
}

uint16_t get_data_length(uint8_t* packet)
{
	if (packet == 0)
		return 0;

	uint16_t length = (packet[4] << 8);
	length += packet[5];

	return length;
}

uint8_t get_sequence_flags(uint8_t* packet)
{
	if (packet == 0)
		return 0;

	uint8_t seq_flags = ((packet[2] & 0xc0) >> PACKET_SEQ_FLAG_POS);

	return seq_flags;
}

uint16_t get_sequence_number(uint8_t* packet)
{
	if (packet == 0)
		return 0;

	uint16_t seq_num = ((packet[2] <<  8) & 0x3f);
	seq_num += packet[3];

	return seq_num;
}

uint8_t has_secondary_hdr(uint8_t* packet)
{
	if (packet == 0)
		return 0;

	uint8_t has_sec_hdr = 0;

	has_sec_hdr = ((packet[0] >> PACKET_SEC_HDR_POS) & 0x01);

	return has_sec_hdr;
}

uint16_t get_app_id(uint8_t* packet)
{
	if (packet == 0)
		return 0;

	uint16_t app_id = ((packet[0] & 0x07 ) << 8); // 0b00000111
	app_id += packet[1];

	return app_id;
}


uint16_t test_function(uint8_t num) 
{
    return num*2;
}