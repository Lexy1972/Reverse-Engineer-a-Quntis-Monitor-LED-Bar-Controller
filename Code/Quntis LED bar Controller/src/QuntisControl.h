//
//	QuntisControl.cpp
//
//	    Remote controller for Quntis ScreenLinear Monitor LED lamp
//
//	Author(s)
//	ing. A. Vermaning
//
//=================================================================================================
//	(c) 2023 LEXYINDUSTRIES
//=================================================================================================
#include <xn297.h>

#define CE_PIN 7
#define CSN_PIN 8

#define TX_REPEAT       6
#define TX_REPEAT_DELAY 5
#define PAYLOAD_LENGTH  6
#define ADDRESS_LENGTH  5

// index in _payload
#define PL_INDEX       4
#define PL_CMD         5

//
// Quntis Commands
//
#define QUNTIS_CMD_ONOFF 0x20
#define QUNTIS_CMD_DIM   0x40
#define QUNTIS_CMD_COLOR 0x30
#define QUNTIS_CMD_DOWN  0x8  //  OR with cmd to go down
#define QUNTIS_CMD_UP    0x0

//=================================================================================================
//	QuntisControl
//=================================================================================================
class QuntisControl
{
public:
    QuntisControl();

    bool begin();

    void OnOff();
    void Dim(bool up, bool repeat = true);
    void Color(bool up, bool repeat = true);

    void ShowNrOfPacketsSend();
    void ResetNrOfPacketsSend();

private:
    void SendCommand(byte cmd, bool repeat);

private:
    XN297 _radio;

    byte _address[ADDRESS_LENGTH] = {0x20, 0x21, 0x01, 0x31, 0xAA};

    //                               |------ Fixed -------| |Idx|  |cmd|
    byte _payload[PAYLOAD_LENGTH] = {0x01, 0x04, 0xCF, 0x31, 0x00, 0x00};

    byte _index;

};