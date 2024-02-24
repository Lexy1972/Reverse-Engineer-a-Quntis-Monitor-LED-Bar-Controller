#include <QuntisControl.h>


//=================================================================================================
// QuntisControl
//=================================================================================================
QuntisControl::QuntisControl()
{

}


//=================================================================================================
// begin
//=================================================================================================
bool QuntisControl::begin()
{
    _index = 0;
      // initialize the transceiver on the SPI bus
    if (!_radio.begin(CE_PIN, CSN_PIN)) 
    {
        return false;
    }

 
    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    _radio.setPALevel(RF24_PA_MAX);  // RF24_PA_MAX is default.

    _radio.setChannel(2);
    //radio.enableDynamicPayloads();

 
    _radio.setPayloadSize(11+2);
    _radio.disableAckPayload();
    _radio.disableDynamicPayloads();

    _radio.setDataRate(RF24_1MBPS);
 
    // set the TX address of the RX node into the TX pipe
    _radio.openWritingPipe(_address);  // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    //radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1

    _radio.setAutoAck(false);
    _radio.setAddressWidth(5);

    _radio.XN297_SetTXAddr(_address, ADDRESS_LENGTH);

    // // For debugging info
    // printf_begin();             // needed only once for printing details
    // // radio.printDetails();       // (smaller) function that prints raw register values
    // _radio.printPrettyDetails(); // (larger) function that prints human readable data
    return true;
}


//=================================================================================================
// OnOff
//=================================================================================================
void QuntisControl::OnOff()
{
    SendCommand(QUNTIS_CMD_ONOFF,true);
}


//=================================================================================================
// Dim
//=================================================================================================
void QuntisControl::Dim(bool up, bool repeat)
{
    SendCommand(QUNTIS_CMD_DIM | (byte)(up ?  QUNTIS_CMD_UP: QUNTIS_CMD_DOWN), repeat);
}


//=================================================================================================
// Color
//=================================================================================================
void QuntisControl::Color(bool up, bool repeat)
{
    SendCommand(QUNTIS_CMD_COLOR | (byte)(up ?  QUNTIS_CMD_UP: QUNTIS_CMD_DOWN), repeat);
}


//=================================================================================================
// SendCommand
//=================================================================================================
void QuntisControl::SendCommand(byte cmd, bool repeat)
{
    _payload[PL_CMD] = cmd;
    _payload[PL_INDEX] = _index++;
    if( repeat)
    {
        for(int i=0;i<TX_REPEAT;i++)
        {
            _radio.XN297_WritePayload(_payload,PAYLOAD_LENGTH);     
            delay(TX_REPEAT_DELAY);
        }
    }
    else
    {
        _radio.XN297_WritePayload(_payload,PAYLOAD_LENGTH); 
    }
}


//=================================================================================================
// ShowNrOfPacketsSend
//=================================================================================================
void QuntisControl::ShowNrOfPacketsSend()
{
    Serial.print("Packets send:#");Serial.println(_radio.GetPacketCount());
}


//=================================================================================================
// ResetNrOfPacketsSend
//=================================================================================================
void QuntisControl::ResetNrOfPacketsSend()
{
    _radio.ResetPacketCount();
}
