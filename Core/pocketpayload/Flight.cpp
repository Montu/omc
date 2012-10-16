#include "Flight.h"
#include "Packet.h"
#include "PocketPayloadInterface.h"

// private function prototypes
int16_t send_data_to_pocket_payload(uint8_t device_address, uint8_t* data, uint16_t data_length);


void setup() {
    
    Serial.begin(9600);
    
    initialise_pocketpayloadinterface(MPQ_FLIGHT_COMPUTER_ADDRESS, 0);
    
}


void loop() {

    char data[] = "Hi there!";
    uint16_t data_length = 9;
   
    send_data_to_pocket_payload(MPQ_ORBIT_VIEW_ADDRESS, (uint8_t*) &data, data_length);
    
    delay(1000);
}

int16_t send_data_to_pocket_payload(uint8_t device_address, uint8_t* data, uint16_t data_length)
{

    int16_t rv = send_packet(
                    device_address,
                    PACKET_TYPE_TELECOMMAND,
                    &data[0],
                    data_length,
                    0,
                    1, 
                    1);
    
    return rv;
}


void process_packet(
    uint8_t* data, 
    uint16_t data_length, 
    uint8_t has_data_header, 
    uint8_t sequence_flags, 
    uint16_t sequence_number) 
{
    uint8_t comm = 0;
    
    if (has_data_header)
    {
        // get header data
        comm = data[0];
    }
    else 
    {
        // no secondary header
        // so just process data
        Serial.print("getting data: ");
        for(uint16_t i=0;i<data_length;i++) {
            Serial.print(data[i]);
        }
            
        Serial.println("");
    }
    if (comm == 0x01) {
        // process command 1
    }
    
    if (comm == 0x02) {
        // process command 2
    }
    
    if (comm == MPQ_DATA_PACKET) {
        // store packet
        // 1. get sequence number
        // 2. store packet data into SD - SD offset derived from sequence number
    }
}



#ifdef TEXTEDITOR


int main(void)
{
    init();

	setup();

    for (;;) {
       loop();
    }

	return 0;
}

#endif