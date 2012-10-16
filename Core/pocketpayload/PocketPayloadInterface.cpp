#include "PocketPayloadInterface.h"
#include "Packet.h"
#include "../libraries/Wire.h"


uint32_t time = 0;  // last time received from ukube
uint32_t t = 0;     // one second timer increment
uint8_t packet[MAX_PACKET_SIZE];

// private prototypes
void setup_wire(uint8_t device_address);
void setup_timer();
void receiveEvent(int howMany);
void process_incoming_packet(uint8_t* packet);
void process_time(uint8_t* data, uint16_t data_length);

void initialise_pocketpayloadinterface(uint8_t device_address, uint8_t set_timer) {

    setup_wire(device_address);
    
    if (set_timer)
        setup_timer();
    
}

void setup_wire(uint8_t device_address) {

#ifdef DEBUG 
    Serial.print("device address: ");
    Serial.println(device_address);
#endif

    Wire.begin(device_address);     // join i2c bus with address #4
    Wire.onReceive(receiveEvent);   // register event

}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
#ifdef DEBUG
    Serial.print("How many: ");
    Serial.println(howMany);
#endif

    // Zero the packet (memset?)
    for (int i=0;i<MAX_PACKET_SIZE;i++) packet[i] = 0;

    //  Get the header
    uint8_t count = 0;
    while(Wire.available() && count < PACKET_PRIMARY_HDR_SIZE) 
    {
        byte b = Wire.receive(); // receive byte as a character
        packet[count] = b;
        count++;
    }

    // validate packet header??
    
    // get the data length in the packet 
    uint16_t data_length = get_data_length(&packet[0]);
    
    // Get the data
    count = 0;
    while(Wire.available() && count < data_length) 
    {
        byte b = Wire.receive(); // receive byte as a character
        packet[PACKET_DATA_POS + count] = b;
        count++;
    }
    
    // send the packet to the client
    process_incoming_packet(&packet[0]);
}

void process_incoming_packet(uint8_t* packet)
{
//    uint16_t app_id = get_app_id(&packet[0]);    
//    uint8_t p_type = get_packet_type(&packet[0]);
    uint16_t data_length = get_data_length(&packet[0]);
    uint8_t sec_hdr = has_secondary_hdr(&packet[0]);
    uint8_t seq_flags = get_sequence_flags(&packet[0]);
    uint16_t seq_num = get_sequence_number(&packet[0]);
    uint8_t* data = &packet[PACKET_DATA_POS];

#ifdef DEBUG
    for(int i=0;i<data_length;i++)
        Serial.print((char)packet[i]); 
    if (sec_hdr == PACKET_HAS_SECONDARY_HDR) {
        Serial.println("Has Secondary Header");
        uint8_t command = data[0];     
    }
#endif
    
    //
    //  If this is IQEA is sending us the time then store that and
    //  restart the timer
    //
    if (sec_hdr == 1)
    {
        // get command
        uint8_t command = packet[PACKET_DATA_POS];
        if (command == MPQ_SYSTEM_TIME)
        {
            process_time(&packet[PACKET_DATA_POS+1], data_length-1);
            return;
        }
    }
    
    // Send packet data to pocket payload code
    process_packet(data, data_length, sec_hdr, seq_flags, seq_num);  
}


//
// Call this function to send a packet of data/command/telemetry to anywhere
//
int16_t send_packet(
    uint16_t destination, 
    uint8_t type,
    uint8_t* data, 
    uint16_t data_length, 
    uint8_t has_header,
    uint16_t max_sequence_number, 
    uint16_t sequence_number)
{
    
	uint8_t packet[PACKET_PRIMARY_HDR_SIZE];
	for (int i = 0; i < PACKET_PRIMARY_HDR_SIZE; i++) packet[i] = 0;
   

    // set packet sequence type
    uint8_t seq_flags = PACKET_SEQ_FLAG_UNSEGMENTED;
    
    if (max_sequence_number > 1)
    {
        seq_flags = PACKET_SEQ_FLAG_CONTINUE;
        if (sequence_number == 0)
            seq_flags = PACKET_SEQ_FLAG_FIRST;
        if (sequence_number == max_sequence_number)
            seq_flags = PACKET_SEQ_FLAG_LAST;
    }

	int16_t rv = create_primary_packet_header(
			&packet[0], 
			type,
			data_length, 
			has_header, 
			destination, 
			seq_flags, 
			sequence_number);

    if (rv == -1) {
        //retry?? must be data error
    }

    uint8_t device_address = (uint8_t) destination;

    int16_t err = -1;
    uint8_t retry_count = 0;
    while(retry_count < 3 && err != 0)
    {
        // Send packet over I2C
        Wire.beginTransmission (device_address);
        Wire.send (&packet[0], 6); // send packet header
        Wire.send (&data[0], data_length); // send data
        err = Wire.endTransmission ();
    
        retry_count++;
    
#ifdef DEBUG
    Serial.println(err);
#endif

        /* 
            err: Output   
               0 .. success
               1 .. length to long for buffer
               2 .. address send, NACK received
               3 .. data send, NACK received
               4 .. other twi error (lost bus arbitration, bus error, ..)
        */   
    }

    return err;
}


////////////////////////////////////////////////////////
//      TIMER FUNCTIONS
////////////////////////////////////////////////////////

// interrupt
ISR(TIMER1_OVF_vect) {
    
    // this function is called once a second (hopefully)
    TCNT1 = 0x0BDC; // set initial value to remove time error (16bit counter register)
    t++;  
}

void setup_timer() {

    // enable one second timer
    TIMSK1 = 0x01;      // enabled global and timer overflow interrupt;
    TCCR1A = 0x00;      // normal operation page 148 (mode0);
    TCNT1 = 0x0BDC;     // set initial value to remove time error (16bit counter register)
    TCCR1B = 0x04;      // start timer/ set clock    
}

// update the time from the recently received time 
// from the flight computer
void process_time(uint8_t* data, uint16_t data_length)
{
    //
    //  TODO!
    //
    t = 0;
    time = data[0];
}

// pocket payload uses this function to receive get the time
uint32_t get_time()
{
    //
    //  TODO!
    //
    return time + t;
}
