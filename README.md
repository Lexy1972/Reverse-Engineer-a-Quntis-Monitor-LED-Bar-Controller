# What

What is it? It's a DIY remote controller for the Quntis Monitor Light Bar PRO+ (Typenr.: LI-HY-208-BK). This light bar can be used to lit the desk in front of your monitor. It comes with a remote controller to turn it on/off and control the dimming and light color:  

![Screenshot](Images/Quntis%20LED%20bar.jpg)

I've made a replacmend for the remote controller, based on a RFNano (Arduino nano with a NRF24L01 on board) and a custom 3D printed case with two encoders to control the brightness and color:

![3D Printed Remote](Images/3D%20Printed%20Remote.jpeg)
	

The top dial (with the dip) controls the dimming, the outer dial controls the color temperature.


# Why?

The Quntis Lightbar comes with a remote controller, so why make one? Well, I didn't like the original one. For one, the dim control is the outer ring and you have to turn it too much to dim for a reasonable amount. And I think the upper control should dim and the outer ring can be used for the color temperature. 

The main reason though, is that it was a good opportunity to learn how to reverse engineer such a RF device with an SDR tool like the HackRF.


# How

Note: the sequency of steps written down here are not necessarily how the steps actually where preformed, because this kind of reverse engineering is a iterative process and for every piece of information you find you sometimes have to go back and do something again.

The first step is take it apart and see what's inside. There is a PCB with two unmarked IC's:
![](Images/Top%20PCB.jpeg)


Although the IC's don't have markings, we can guess that U1 (larger IC to the left) is probably some sort of microcontroller and U2 (to IC next to the wiggly PCB trace) is some sort of transmitter. The wiggly traces are the antenna.

When we zoom in and reverse the PCB a little bit, we see how the RF transmitter is connected to the microcontroller:
![](Images/Top%20PCB%20Detail.png)


The pin one location of U2 is upper right.

We see that 3 lines (A,B,C in green, pins 1-3 of U2) are connected directly to the uC. We also identify the power connections (marked in red (4) and blue (7)).
The Xtal (Y1) is connected to U2 at pins 5 and 6 (via R1). The RF is outputted at pin 8 to the antenna via C5.

From the size of the antenna we can estimate that the transmit frequency is high, some ware in the GHz range, probably the 2.4-2.5 GHz ISM band.
A good start to find the used frequencies is to find the FCC ID and search in the FCC database, but this device does not have a FCC id (although it is 'FCC compliant).

I also did a VNA measurement on the antenna by removing C5 and adding a coax to it:
![](Images/VNA%20Measurement%20setup.jpg)

![](Images/VNA%20measurement%20result.png)

We observe that there is a dip at 2.14GHz, not quite 2.4, but maybe this is not the correct way to measure such a PCB antenna. But I'm convinced that the RF transmitter operates in the 2.4GHz band.

Now we have to find what for transmitter is used. We do this by searching the internet for transmitter IC's in a SO8 package and look for the ones that have the same pinout.
This is a tedious process and took some time (a span of a few day's), but eventually I found a match in the XN297:

![](Images/XN297%20pinout.png)
	
[XN297 datasheet](https://www.panchip.com/static/upload/file/20190916/1568621331607821.pdf)
	
From the PDF:
> XN297L is a single chip 2.4GHz RF transceiver designed for operation in the world wide ISM frequency band from 2.400 to 2.483GHz. XN297L with an embedded baseband protocol engine, is suitable for ultra-low power wireless applications. 

So I'm pretty sure this is the one that is used here.

Side note on the XN297:

It's a Chinese clone of the Nordic NRF24L01 and is used in many Chinese remote controls for RC cars and drones. This comes in handy later on!
There are also other clones of this chip.

Now we can see that the pins 1-3 are SPI lines and we can connect a Logic analyzer to it. We can now correlate it with the XN297 datasheet to see if it is the same protocol.
Note: the SO8 version of the XN297 only has a data out pin (MOSI), so there is one way communication!

Here we see the data when pressing the on/off button:
![](Images/Logic%20Analyzer%20overview.png)
	

The first part is the configuration of the XN297, the second part is sending the RF data 6x. We can see that data has been send at the RF channel, which is connected to a 2.4GHz antenna and a RF diode detector. This signal is amplified (~2000x) so the Logic Analyzer can see it.

This is the config part:
![](Images/Logic%20Analyzer%20Config.png)
	

The data burst (one block):
![](Images\Logic%20Analyzer%20Config%20data%20burst.png)
	

I'll highlight the interesting parts of the config like data length, mode and channel (freq).

First we need to see how the SPI protocol is used (see XN297 datasheet: https://www.panchip.com/static/upload/file/20190916/1568621331607821.pdf)

In the XN297, the SPI commands are one command byte and some data bytes depending on the command.
![](Images/XN297%20SPI%20Commands.png)
	

The write command byte is identified by the upper tree bits being 001. 

The XN297 can operate in different modes, but this implementation uses the normal mode:
![](Image/XN297%20Normal%20Burst.png)

We can see this by looking at the config and see where the above mentioned registers are written.

EN_AA => cmd 0x21: 0x00 written

![](Images/Logic%20Analyzer%20cmd%200x21.png)
	
SETUP_RETR => cmd 0x24: 0x00 written

![](Images/Logic%20Analyzer%20cmd%200x24.png)

DYNPD => cmd 0x3C: 0x00 written

![](Images/Logic%20Analyzer%20cmd%200x3c.png)

FEATURE => cmd 0x3D: 0x20 written

![](Images/Logic%20Analyzer%20cmd%200x3d.png)
	
Let's look for the address length command (cmd:0x23):

![](Images/SPI%20cmd%200x23.png)
	
Data is 0x03:
![](Images/Logic%20Analyzer%20cmd%200x03.png)	

So, there are 5 address bytes configured.
	
Now see the transmit channel (cmd 0x25):
![](Images/SPI%20cmd%200x25.png)
So, the data is 0x02, so channel 2 @ 2402 MHz is used.


Now the data rate (cmd 0x26):
![](Images/SPI%20cmd%200x26.png)
	
![](Images/Logic%20Analyzer%20cmd%200x26.png)
	
The data here is 0x2c (00101100). Bits 7:6 are 00, so the data rate is 1 Mbps. We can also see the TX power is set to 101100 => 5dBm.



There is also a CONFIG register (cmd 0x20):
![](Images/SPI%20cmd%200x20.png)
	
![](Images/Logic%20Analyzer%20cmd%200x20.png)
	
Data is 0x8E (1000 1110). Bits 1:3 are 1, so CRC is enabled and has 2 bytes.
	
So, we have the following configuration:

| Param     | Value       |
| ---       | ---         |
| Mode      | Normal mode |
| TX freq   | 2402 MHz    |
| Data rate | 1 Mbps      |
| Address   | 5 bytes     |
| CRC       | 2 Bytes     |

Note that there is no data length configuration for the transmitter. The data sheet mentions that the transmitted data length is set by the number of data bytes that are clocked in to the chip.
	
Let's have a look at the data:

![](Images/Logic%20Analyzer%20config%20detail.png)
	
The first three commands are to flush the buffers and interrupts, than the data is clocked in via command 0xA0. We can see that there are 6 data bytes (this is for pressing the on/off button press):

	01 04 CF 31 61 20

When pressing on/off a few times, we can see a pattern:

	01 04 CF 31 61 20
	01 04 CF 31 62 20
	01 04 CF 31 63 20
	01 04 CF 31 65 20
	01 04 CF 31 66 20
	
Turning the dim up:

	01 04 CF 31 81 48
	01 04 CF 31 82 48
	01 04 CF 31 84 48
	01 04 CF 31 86 48

Turning the dim down:

	01 04 CF 31 A3 40
	01 04 CF 31 A5 40
	01 04 CF 31 A6 40
	01 04 CF 31 A9 40
	
For the color to cold we see:

	01 04 CF 31 03 30
	01 04 CF 31 04 30
	01 04 CF 31 06 30
	
And for dial to warm:

	01 04 CF 31 13 38
	01 04 CF 31 14 38
	01 04 CF 31 16 38

We see that the first 4 bytes are constant: 01 04 CF 31.

The next byte is incremented every send. This is probably used by the receiver to determine if a packed is already received, because the same packed is transmitted 6 times to make sure the receiver gets it.

The last byte is the specific command. We now can deduce the following commands:


| Command       | Byte |
| - | - |
| On/off        | 0x20 |
| Color to cold | 0x30 |
| Color to warm | 0x38 |
| Dim up	| 0x40 |
| Dim down	| 0x48 |

Now we have a complete insight of the used protocol to communicate via a XN297 to the Quntis light bar, but I really want also use a SDR to get the data, so let's do that.


# SDR

So, I bought a HackRF and started messing around with the known SDR tools as sdr++ and Universal Radio hacker to see if I could catch a glimpse of the spectrum of the XN297. But that was harder than it seems. This is mainly because I don't know exactly what to look for and the frequency that the remote uses (2402MHz) is quit a busy region of the spectrum in my environment. I managed eventually to record a signal of the On/Off press with the Universal Radio Hacker tool and when replayed, the light bar responded by turning off. 

So I started with GNU Radio Companion (GRC) to see if I could receive and decode the signal. The XN297 uses a GFSK modulation and the GRC has a block to demodulate that. I never had used GNU radio or similar software, so it took some fiddling around and watching a lot of YouTube about GNU Radio.

To make things a little easier for me, I decided to use a RFNano (which is an Arduino nano with a NRF24L01 chip on board) with some simple test code that transmitted fixed data, to see if I could make a receiver in GNU radio. And eventually I got it working. With that I came up with this flow diagram that demodulates the RF into a bit stream and that bitstream gets send to a separate process, written in C#, that takes the bit stream for decoding:

![](Images/GRC%20flow.png)

This flow can be found here: [XN297 NRF24 recv for HackRF.](GNU-Radio/XN297%20NRF24%20recv%20for%20HackRF.grc)

The Low Pass Filter filters the output of the HackRF at 1MHz, then the signal gets passed to a Power Squelch. This adds a tag (squelch_sob: Start Of Block) to the stream if the power of the signal reaches a certain threshold and the QT GUI stuff can trigger on this tag.

The signal gets demodulated by the GFSK Demod block. The important parameter here is the Samples/Symbol. We need to set it according to the following: samp_rate/data_rate. The sample_rate is set to 8MSP/sec and the datarate of the XN297 in the Quntis is 1Mbaud, so the Samples per Symbol is 8.

With the Correlate Access code, we can add a tag to the stream when a certain bit pattern is in the stream. We use 011100010000111101010101, 0x71 0x0F, 0x55, which is the preamble of the XN297 (Note: The preamble of a NRF24L01 is only a 0x55).

We the use a ZMQ PUB Sink block to send the bit stream to a C# process by the ZeroMQ protocol.

When run, the UI looks like:

![](Images/GRC%20Dialog.png)

Here we see the Signal and the IQ Diagram, which we can use to see if we receive the signal correctly.

In the Bit stream diagram we see the actual demodulated bit stream. We see also the tag 'preamble_end', which is inserted by the Correlate Access code block (note, the preamble is thus before this time and is not shown here!).

The C# side looks like this:

![](Images/C%23%20decoder%20main.png)

The C# project is here: [XN297Decoder_V3](Code/XN297Decoder_V3)

The NetMQ lib is used here to receive the data stream. The data is received as one bit per byte. I've made a shift register class (BitShiftArray) that is used to shift in the bits and determine if the preamble is in the data stream. The bits are added with bits.AddBit() (line 520). This function returns true if a preamble is at the end (MSB) of the shift register. This means that the rest of the shift register contains the data we want.

When we have received all bits, we decode them. 

Unfortunately, the XN297 scrambles the data before sending it out, so the received data was not what we expected. But thanks to the internet, we have made a descrambling function. The XN297 is apparently used as RC controller for many Drones and RC Cars, so people already made decoding for this (see for example: https://github.com/goebish/nrf24_multipro/tree/master/nRF24_multipro).  

The data received for pressing the on/off button once looks like this:

	|-------- received/scrambled --------|     |---------- descrambled/decoded ---------|
	|                                    |     |- address --|   |----- data ------| |crc|
	49 80 4A CB A5 3C C5 95 81 04 88 DD B0  >> AA 31 01 21 20 | 01 04 CF 31 55 20 | B0 DD
	49 80 4A CB A5 3C C5 95 81 04 88 DD B0  >> AA 31 01 21 20 | 01 04 CF 31 55 20 | B0 DD
	49 80 4A CB A5 3C C5 95 81 04 88 DD B0  >> AA 31 01 21 20 | 01 04 CF 31 55 20 | B0 DD

We see 3 times the same packet. The Quntis send 6x the same packet to make the change higher that receiver receives a good data packet (as we saw with the Logic Analyzer)

We also see that the data matches the data that we saw when using the Logic Analyzer, so de descrambling works!

Now let's make a controller using the RFNano!

# Controller using RFNano (NRF24L01)

I've already mentioned the XN297 emulation using a NRF24L01 example: https://github.com/goebish/nrf24_multipro : XN297_emu.ino.
This code is as base to make an own implementation using the 'standard' Arduino library for the NRF24L01 (https://github.com/nRF24/RF24).

The code is here: [Quntis LED bar Controller](Code/Quntis%20LED%20bar%20Controller)

I've made a new class XN297 with the RF24 as it's base:

![](Images/Class%20XN297.png)

This class handles all the scrambling and emulation stuff to make the nRF24L01 transmit like a XN297.
There is some modification to the RF24 library code needed, because we need to use the private marked function RF24::write_register() in our XN297_SetTXAddr() function.
To make this work, we need to mark it as protected in the RF24.h file:

![](Images/RF24%20modification.png)

Now we can use it in our XN297 class.

Another class is made to handle the Quntis protocol:

![](Images/Class%20QuntisControl.png)

Here are the address and the fixed part of the data defined. I don't know if this is universal for all devices (probably not).

The whole project is here: [link].

There are two EC11 encoders used as input control for the dimming, on/off and the color.

A simple serial control is also implemented:

	Quntis Monitor Light Remote Controller V1
	o: On/off
	+: Dim up
	-: Dim down
	c: Color to colder
	w: Color to warmer
	f: Dim quickly up
	F: Dim quickly down

The controller send back the letter it has received or a ? if it is unknown.

# 3D Design

The 3D design is done in Fusion 360, the design files are here:[link]

![](Images/3D%20explode%20view.png)

![](Images/3D%20section%20view.png)


It consists of two normal encoders (EC11) and a simple gear system is used to make it concentrical. One of the key design 'features' here is that the top dial is used to dim and the outer ring is used to change the light color/temperature.

