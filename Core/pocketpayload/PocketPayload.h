



#ifndef POCKETPAYLOAD_H_
#define POCKETPAYLOAD_H_

#include "WProgram.h"

    void send_photo(uint16_t photoid);
    uint32_t get_photo_size(uint16_t photoid);
    int16_t get_photo_chunk(uint8_t* packet_data, uint32_t offset, uint16_t photo_packet_size);

#endif