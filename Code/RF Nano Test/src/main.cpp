#include <Arduino.h>
#include <RF24.h>
#include <printf.h>

#define CE_PIN 7
#define CSN_PIN 8
// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// Let these addresses be used for the pair
//uint8_t address[][6] = { "1Node", "2Node" };
//uint8_t address[][6] = { {0x00,0x00,0x00,0x00,0x00,0}, {0xAA,0xAA,0xAA,0xAA,0xAA,0} };
//                        MSBy-----------------LSBy
uint8_t address[][6] = { {0xFF,0x00,0xFF,0xFF,0x00,0}, {0xAA,0xAA,0xAA,0xAA,0xAA,0} };

// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1;  // 0 uses address[0] to transmit, 1 uses address[1] to transmit

// Used to control whether this node is sending or receiving
bool role = false;  // true = TX role, false = RX role

// For this example, we'll be using a payload containing
// a single float number that will be incremented
// on every successful transmission
// #define PAYLOAD_LENGTH 6
// //byte payload[PAYLOAD_LENGTH] = {0xFF, 0xAA, 0x80, 0x55, 0x01, 0x00};
// byte payload[2][PAYLOAD_LENGTH] = { {0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00},
//                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF} };
#define PAYLOAD_LENGTH 6
//byte payload[PAYLOAD_LENGTH] = {0xFF, 0xAA, 0x80, 0x55, 0x01, 0x00};
byte payload[2][PAYLOAD_LENGTH] = { {0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00},
                                    {0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF} };
//byte payload[PAYLOAD_LENGTH] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

void setup() 
{
    Serial.begin(115200);
    Serial.println("RF Nano Test");

    // initialize the transceiver on the SPI bus
    if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
    }

    // print example's introductory prompt
    Serial.println(F("RF24/examples/GettingStarted"));

    // To set the radioNumber via the Serial monitor on startup
    Serial.println(F("Which radio is this? Enter '0' or '1'. Defaults to '0'"));
    while (!Serial.available())
    {
    // wait for user input
    }
    char input = Serial.parseInt();
    radioNumber = input == 1;
    Serial.print(F("radioNumber = "));
    Serial.println((int)radioNumber);

    // role variable is hardcoded to RX behavior, inform the user of this
    Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

    radio.setChannel(120);
    radio.enableDynamicPayloads();

    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need to transmit a float
    //radio.setPayloadSize(sizeof(payload[0]));  // float datatype occupies 4 bytes
    radio.setPayloadSize(PAYLOAD_LENGTH);
    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]);  // always uses pipe 0
    radio.setDataRate(RF24_1MBPS);
    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1

    radio.setAutoAck(false);


    // additional setup specific to the node's role
    if (role) 
    {
        radio.stopListening();  // put radio in TX mode
    } 
    else 
    {
        radio.startListening();  // put radio in RX mode
    }

    // For debugging info
    printf_begin();             // needed only once for printing details
    // radio.printDetails();       // (smaller) function that prints raw register values
    radio.printPrettyDetails(); // (larger) function that prints human readable data

}
int iPayload = 0;
bool cont = false;
bool fast = false;
void loop() 
{
    
    if (role) 
    {
        // This device is a TX node
        
        Serial.print("Send payload:");
        for(int i=0; i < PAYLOAD_LENGTH; i++)
        {
            Serial.print(":");
            Serial.print(payload[iPayload][i], HEX);  
        }
        Serial.println();
        unsigned long start_timer = micros();                // start the timer
        bool report = radio.write(&payload[iPayload], PAYLOAD_LENGTH);  // transmit & save the report
        unsigned long end_timer = micros();                  // end the timer

    

        if (report) 
        {
            Serial.print(F("Transmission successful! "));  // payload was delivered
            Serial.print(F("Time to transmit = "));
            Serial.print(end_timer - start_timer);  // print the timer result
            Serial.print(F(" us. Sent "));
            for(int i=0; i < PAYLOAD_LENGTH; i++)
            {
                Serial.print(":");
                Serial.print(payload[iPayload][i], HEX);  
            }
            Serial.println();
            
           
        } 
        else 
        {
            Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
        }

        if ( cont == false)
        {
            Serial.println("Press key to transmit");
            while(!Serial.available());     
        }
        
        char key = Serial.read();
        if ( key == '1')
        {
            iPayload = 1;
        }
        else if (key == '0')
        {
            iPayload = 0;
        }
        else if (key == 'c')
        {
            cont = true;
            fast = false;
        }
        else if (key == 's')
        {
            cont = false;
        }
        else if (key == 'f')
        {
            fast = true;
        }
        // to make this example readable in the serial monitor
        if ( cont && !fast) delay(1000);  // slow transmissions down by 1 second

    } 
    else 
    {
        // This device is a RX node

        uint8_t pipe;
        if (radio.available(&pipe)) 
        {              // is there a payload? get the pipe number that recieved it
            uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
            radio.read(&payload[iPayload], bytes);             // fetch payload from FIFO
            Serial.print(F("Received "));
            Serial.print(bytes);  // print the size of the payload
            Serial.print(F(" bytes on pipe "));
            Serial.print(pipe);  // print the pipe number
            for(int i=0; i < PAYLOAD_LENGTH; i++)
            {
                Serial.print(":");
                Serial.print(payload[iPayload][i], HEX);  
            }
            Serial.println();
        }
    }  // role

    if (Serial.available()) 
    {
        // change the role via the serial monitor

        char c = toupper(Serial.read());
        if (c == 'T' && !role) 
        {
            // Become the TX node

            role = true;
            Serial.println(F("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK"));
            radio.stopListening();
        } 
        else if (c == 'R' && role) 
        {
            // Become the RX node

            role = false;
            Serial.println(F("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK"));
            radio.startListening();
        }
    }
}

