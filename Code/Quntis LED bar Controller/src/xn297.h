#include <Arduino.h>
#include <RF24.h>
#include <nRF24L01.h>

class XN297: public RF24
{
public:
    XN297(){};
    XN297(rf24_gpio_pin_t _cepin, rf24_gpio_pin_t _cspin);

    void XN297_SetTXAddr(const uint8_t* addr, uint8_t len);
    uint8_t XN297_WritePayload(uint8_t* msg, uint8_t len);

    static void HexDump(byte* buf, byte len);
    long GetPacketCount() { return _nrOfPackets;}
    void ResetPacketCount() {_nrOfPackets = 0;}
private:
    uint16_t crc16_update(uint16_t crc, unsigned char a);
    uint8_t bit_reverse(uint8_t b_in);

    long _nrOfPackets;
};