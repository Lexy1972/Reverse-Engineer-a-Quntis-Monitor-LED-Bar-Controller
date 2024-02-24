//
//	xn297.cpp
//
//	    Emulate the simple/normal XN297 protocol using an RF24L01
//
//          Based on the https://github.com/goebish/nrf24_multipro : XN297_emu.ino
//          But uses the RF24 arduino library (https://github.com/nRF24/RF24) to comunicate 
//          with the RF24L01.
//      
//        !!! Note !!!!
//        We need the RF24::write_register() private functions, so we need to modify the RF24.h
//        to change it to protected. (there are two versions of write_register):
//
//              protected: 
//              void write_register(uint8_t reg, const uint8_t* buf, uint8_t len);
//              void write_register(uint8_t reg, uint8_t value, bool is_cmd_only = false);
//              private:
//
//	Author(s)
//	ing. A. Vermaning
//
//=================================================================================================
//	(c) 2023 LEXYINDUSTRIES
//=================================================================================================
#include <xn297.h>


static uint8_t xn297_addr_len;
static uint8_t xn297_tx_addr[5];
static uint8_t xn297_rx_addr[5];
static uint8_t xn297_crc = 1;

static const uint8_t xn297_scramble[] = {
    0xe3, 0xb1, 0x4b, 0xea, 0x85, 0xbc, 0xe5, 0x66,
    0x0d, 0xae, 0x8c, 0x88, 0x12, 0x69, 0xee, 0x1f,
    0xc7, 0x62, 0x97, 0xd5, 0x0b, 0x79, 0xca, 0xcc,
    0x1b, 0x5d, 0x19, 0x10, 0x24, 0xd3, 0xdc, 0x3f,
    0x8e, 0xc5, 0x2f};
    
static const uint16_t xn297_crc_xorout[] = {
    0x0000, 0x3448, 0x9BA7, 0x8BBB, 0x85E1, 0x3E8C, 
    0x451E, 0x18E6, 0x6B24, 0xE7AB, 0x3828, 0x814B,
    0xD461, 0xF494, 0x2503, 0x691D, 0xFE8B, 0x9BA7,
    0x8B17, 0x2920, 0x8B5F, 0x61B1, 0xD391, 0x7401, 
    0x2138, 0x129F, 0xB3A0, 0x2988};

static const uint16_t polynomial = 0x1021;

static const uint16_t initial    = 0xb5d2;


//=================================================================================================
// XN297
//=================================================================================================
XN297::XN297(rf24_gpio_pin_t _cepin, rf24_gpio_pin_t _cspin) : RF24(_cepin,_cspin )
{
    _nrOfPackets = 0;
}


//=================================================================================================
// bit_reverse
//=================================================================================================
uint8_t  XN297::bit_reverse(uint8_t b_in)
{
    uint8_t b_out = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        b_out = (b_out << 1) | (b_in & 1);
        b_in >>= 1;
    }
    return b_out;
}


//=================================================================================================
// crc16_update
//=================================================================================================
uint16_t XN297::crc16_update(uint16_t crc, unsigned char a)
{
    crc ^= a << 8;
    for (uint8_t i = 0; i < 8; ++i) 
    {
        if (crc & 0x8000) 
        {
            crc = (crc << 1) ^ polynomial;
        } 
        else 
        {
            crc = crc << 1;
        }
    }
    return crc;
}


//=================================================================================================
// XN297_SetTXAddr
//=================================================================================================
void XN297::XN297_SetTXAddr(const uint8_t* addr, uint8_t len)
{
    if (len > 5) len = 5;
    if (len < 3) len = 3;
    uint8_t buf[] = { 0x55, 0x0F, 0x71, 0x0C, 0x00 }; // bytes for XN297 preamble 0xC710F55 (28 bit)
    xn297_addr_len = len;
    if (xn297_addr_len < 4) {
        for (uint8_t i = 0; i < 4; ++i) {
            buf[i] = buf[i+1];
        }
    }

    //NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
    write_register(SETUP_AW, len-2);

    //NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, buf, 5);
    write_register(TX_ADDR,buf,5);

    
    
    // Receive address is complicated. We need to use scrambled actual address as a receive address
    // but the TX code now assumes fixed 4-byte transmit address for preamble. We need to adjust it
    // first. Also, if the scrambled address begins with 1 nRF24 will look for preamble byte 0xAA
    // instead of 0x55 to ensure enough 0-1 transitions to tune the receiver. Still need to experiment
    // with receiving signals.
    memcpy(xn297_tx_addr, addr, len);
}


//=================================================================================================
//
//=================================================================================================
uint8_t XN297::XN297_WritePayload(uint8_t* msg, uint8_t len)
{
    uint8_t buf[32];
    uint8_t res;
    uint8_t last = 0;
    if (xn297_addr_len < 4) {
        // If address length (which is defined by receive address length)
        // is less than 4 the TX address can't fit the preamble, so the last
        // byte goes here
        buf[last++] = 0x55;
    }
    for (uint8_t i = 0; i < xn297_addr_len; ++i) {
        buf[last++] = xn297_tx_addr[xn297_addr_len-i-1] ^ xn297_scramble[i];
    }

    for (uint8_t i = 0; i < len; ++i) {
        // bit-reverse bytes in packet
        uint8_t b_out = bit_reverse(msg[i]);
        buf[last++] = b_out ^ xn297_scramble[xn297_addr_len+i];
    }
    if (xn297_crc) {
        uint8_t offset = xn297_addr_len < 4 ? 1 : 0;
        uint16_t crc = initial;
        for (uint8_t i = offset; i < last; ++i) {
            crc = crc16_update(crc, buf[i]);
        }
        crc ^= xn297_crc_xorout[xn297_addr_len - 3 + len];
        //Serial.println(crc,16);
        buf[last++] = crc >> 8;
        buf[last++] = crc & 0xff;
    }
    //res = NRF24L01_WritePayload(buf, last);
    res = write(buf, last);
    //Serial.print("XN297 input:");HexDump(msg,len);Serial.println();
    //Serial.print("XN297 write:");HexDump(buf,last);Serial.println();
    _nrOfPackets++;
    return res;
}


//=================================================================================================
//
//=================================================================================================
void XN297::HexDump(byte* buf, byte len)
{
    for(int i=0; i<len;i++)
    {
        Serial.print(buf[i],16); Serial.print(" ");
    }
}