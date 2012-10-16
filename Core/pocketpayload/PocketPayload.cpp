//
// payload Openspace365/Qubduino/OrbitView
//

#include "Packet.h"
#include "PocketPayloadInterface.h"
#include "PocketPayload.h"

void send_test_packet_to_flight();

//  setup function
void setup() {

    Serial.begin(9600);
    
    // must call this to specify the address of the 
    // pocket payload on the Wire/I2C bus and to 
    // setup the timer.
    initialise_pocketpayloadinterface(MPQ_ORBIT_VIEW_ADDRESS, 1);
}


//  main application loop
void loop() {
    
    // OPTIONAL system call to get the time
    uint32_t t = get_time();
    
    // do stuff
    Serial.print("Time: ");
    Serial.println(t);
    
    delay(1000);
    
    send_test_packet_to_flight();
}

//  this function is called when a packet is received
void process_packet(
    uint8_t* data,
    uint16_t data_length,
    uint8_t has_data_header,
    uint8_t sequence_flags,
    uint16_t sequence_number)
{
    uint8_t comm = 0;
    
    Serial.println("got packet!");
    
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



//////////////////////////////////////////////////////
///  Example send packet usage
//////////////////////////////////////////////////////

void send_test_packet_to_flight()
{
    uint16_t packet_data_length = 16;   
    uint8_t packet_data[16];
    for(uint8_t i=0;i<16;i++)
        packet_data[i] = 0x30+i;
        
    packet_data[15] = '\0';
       
    uint16_t total_packets = 1;
    
    // try sending the packet, if error then retry
    send_packet(
            MPQ_FLIGHT_COMPUTER_ADDRESS,
            PACKET_TYPE_TELECOMMAND,
            &packet_data[0],
            packet_data_length,
            0,
            total_packets, 
            1);
            
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