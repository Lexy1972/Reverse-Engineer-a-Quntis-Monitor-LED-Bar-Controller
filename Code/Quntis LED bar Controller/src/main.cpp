//
//	main.cpp
//
//      Quntis Monitor Light Bar Remote Controller
//      Works with the RF-Nano (https://www.tinytronics.nl/shop/nl/development-boards/microcontroller-boards/arduino-compatible/rf-nano-v3.0-met-ingebouwde-nrf24l01)
//
//	Author(s)
//	ing. A. Vermaning
//
//=================================================================================================
//	(c) 2023 LEXYINDUSTRIES
//=================================================================================================
#include <Arduino.h>
#include <QuntisControl.h>
#include <Encoder.h>
#include <OneButton.h>

#define DIM_REPEAT 2

QuntisControl _quintisCtrl;
Encoder _encDim(6, 5);
OneButton btnOnOff(4, true);
long _posDim  = 0;

Encoder _encColor(3, 2);
long _posColor  = 0;


//=================================================================================================
// ShowOptions
//=================================================================================================
void ShowOptions()
{ 
    Serial.println("o: On/off");
    Serial.println("+: Dim up");
    Serial.println("-: Dim down");
    Serial.println("c: Color to colder");
    Serial.println("w: Color to warmer");
    Serial.println("f: Dim quickly up");
    Serial.println("F: Dim quickly down");
}


//=================================================================================================
// ClickOnOff
//=================================================================================================
void ClickOnOff()
{
     _quintisCtrl.OnOff();
    //Serial.println("ClickOnOff");
}


//=================================================================================================
// ClickOnOff
//=================================================================================================
void ClickOnOffDouble()
{
     for(int i=0;i<60;i++) _quintisCtrl.Dim(false);
    //Serial.println("ClickOnOff");
}


//=================================================================================================
// setup
//=================================================================================================
void setup() 
{
    Serial.begin(115200);
    Serial.println("Quntis LED Bar Remote Controller V1");

    //
    //  startup the controller
    //
    if ( !_quintisCtrl.begin())
    {
        Serial.println(F("radio hardware is not responding!!"));
        while(1);
    }

    btnOnOff.attachClick(ClickOnOff);
    btnOnOff.attachDoubleClick(ClickOnOffDouble);

    ShowOptions();
}


//=================================================================================================
// loop
//=================================================================================================
void loop() 
{
    do
    {
        //
        // Handle dim encoder
        //
        long newPos = _encDim.read();
        if (newPos != _posDim) 
        {
            if ( newPos > _posDim)
            {
                for(int i=0;i<DIM_REPEAT;i++)
                {
                    _quintisCtrl.Dim(true,false);
                    delay(1);
                }
            }
            else
            {
                for(int i=0;i<DIM_REPEAT;i++)
                {
                    _quintisCtrl.Dim(false,false);
                    delay(1);
                }
            }
            _posDim = newPos;
            
            //Serial.print("Dim:"); Serial.println(_posDim);
        }        

        //
        // Handle color encoder
        //
        long newCPos = _encColor.read();
        if (newCPos != _posColor) 
        {
            if ( newCPos > _posColor)
            {
                _quintisCtrl.Color(true,true);
            }
            else
            {
                _quintisCtrl.Color(false,true);
            }
            _posColor = newCPos;
            //Serial.print("Color:");Serial.println(_posColor);
        }        

        btnOnOff.tick(); //handle button
    } while( !Serial.available() );
    
    //
    // Handle Serial
    //
    char key = Serial.read();
    switch(key)
    {
    case 'o':
        _quintisCtrl.OnOff();
        Serial.println(key);
        break;
    case '+':
        _quintisCtrl.Dim(true);
        Serial.println(key);
        break;
    case '-':
        _quintisCtrl.Dim(false);
        Serial.println(key);
        break;
    case 'c':
        _quintisCtrl.Color(true);
        Serial.println(key);
        break;
    case 'w':
        _quintisCtrl.Color(false);
        Serial.println(key);
        break;
    case 'f':
        for(int i=0;i<40;i++) _quintisCtrl.Dim(true);
        Serial.println(key);
        break;
    case 'F':
        for(int i=0;i<40;i++) _quintisCtrl.Dim(false);
        Serial.println(key);
        break;
    case 'd':
        _quintisCtrl.ShowNrOfPacketsSend();
        break;
    case 'D':
        _quintisCtrl.ResetNrOfPacketsSend();
        break;
    
    default:
        Serial.println('?');
    }

    while( Serial.available())
        Serial.read();
}

