using NetMQ;
using NetMQ.Sockets;
using System.Collections;

namespace XN297Decoder_V3
{
    class BitShiftArray
    {
        public static byte[] xn297_scramble = {
            0xE3, 0xB1, 0x4B, 0xEA, 0x85, 0xBC, 0xE5, 0x66,
            0x0D, 0xAE, 0x8C, 0x88, 0x12, 0x69, 0xEE, 0x1F,
            0xC7, 0x62, 0x97, 0xD5, 0x0B, 0x79, 0xCA, 0xCC,
            0x1B, 0x5D, 0x19, 0x10, 0x24, 0xD3, 0xDC, 0x3F,
            0x8E, 0xC5, 0x2F, 0xAA, 0x16, 0xF3, 0x95 };

        BitArray _bits;
        BitArray? _prevBits;

        BitArray _preamble;
        int _nrOfBits;
        int _preambleIndex;
        int _preambleFoundIndex;
        bool _preambleAtMSB;
        uint _preambleOffset;
        //===================================================================================================
        // BitShiftArray
        //===================================================================================================
        public BitShiftArray(int Length, BitArray Preamble, uint preambleOffset = 0)
        {
            _bits = new BitArray(Length);
            _prevBits = null;
            _preamble = Preamble;
            _nrOfBits = 0;
            _preambleIndex = _preamble.Length - 1;
            _preambleFoundIndex = -1;
            _preambleOffset = 0;
            _preambleOffset = preambleOffset;
            //_preambleAtMSB = false;

        }


        //===================================================================================================
        // Clear
        //===================================================================================================
        public void Clear()
        {
            _bits.SetAll(false);

            _nrOfBits = 0;
            _preambleIndex = _preamble.Length - 1;
            _preambleFoundIndex = -1;
        }


        //===================================================================================================
        // AddBit
        //===================================================================================================
        public bool AddBit(bool bit)
        {
            bool preambleAtMSB = false;

            _bits.LeftShift(1);
            _bits[0] = bit;

            if (_nrOfBits < _bits.Length)
            {
                // Not full yet
                _nrOfBits++;
            }
            else
            {
                //full
            }

            //
            // Check preamble
            //
            if (_preambleFoundIndex < 0 && _preambleIndex >= 0)
            {
                if (bit == _preamble[_preambleIndex])
                {
                    _preambleIndex--;
                }
                else
                {
                    _preambleIndex = _preamble.Length - 1;
                }

                if (_preambleIndex <= 0)
                {
                    // store the current index into the found index.(We need to shift this!)
                    _preambleFoundIndex = _preamble.Length - 2;
                    //Console.WriteLine($"Preamble found @{_preambleFoundIndex}");
                    //PrintBits();
                }
            }
            else
            {
                _preambleFoundIndex++;
                //Check if the found preamble is at the end of the bits.
                if (_preambleFoundIndex == _bits.Length - 1 - _preambleOffset)
                {
                    preambleAtMSB = true;
                }
                else if (_preambleFoundIndex >= _bits.Length) // preamble is beyond _bits length
                {
                    //
                    // reset the preamble found
                    //
                    _preambleIndex = _preamble.Length - 1;
                    _preambleFoundIndex = -1;
                }
            }
            return preambleAtMSB;
        }

        //===================================================================================================
        // PrintBits
        //===================================================================================================
        public void PrintBits()
        {
            string line1 = "";
            string line2 = "";
            string line3 = "";
            byte[] dataBytes = new byte[_bits.Length / 8 + 1]; ;

            _bits.CopyTo(dataBytes, 0);

            Console.WriteLine($"Preamble@{_preambleFoundIndex}");

            int x = 7;
            int ibitsPerRow = 0;
            for (int i = _bits.Length - 1; i >= 0; i--)
            {
                ibitsPerRow++;
                line2 += _bits[i] ? "1" : "0";
                if (i == _preambleFoundIndex)
                {
                    line3 += "^";
                }
                else
                {
                    line3 += " ";
                }
                line1 += (x).ToString();


                x--;
                if (x < 0)
                {
                    x = 7;
                    if (ibitsPerRow >= 8 * 8)
                    {
                        ibitsPerRow = 0;
                        Console.WriteLine(line1);
                        Console.WriteLine(line2);
                        Console.WriteLine(line3);
                        line1 = "";
                        line2 = "";
                        line3 = "";
                    }
                    else
                    {
                        line1 += " ";
                        line2 += " ";
                        line3 += " ";
                    }
                }
            }
            Console.WriteLine(line1);
            Console.WriteLine(line2);
            Console.WriteLine(line3);
        }


        //===================================================================================================
        // HexDump
        //===================================================================================================
        public void HexDump(uint startToDescrambe)
        {

            int nrOfBytes = _bits.Length / 8;
            if (_bits.Length % 8 != 0)
            {
                nrOfBytes++;
            }

            int nrOfPreambleBytes = _preamble.Length / 8;
            if (_preamble.Length % 8 != 0)
            {
                nrOfPreambleBytes++;
            }


            byte[] dataBytes = new byte[nrOfBytes];
            byte[] prevDataBytes = new byte[nrOfBytes];

            byte[] preambleBytes = new byte[nrOfPreambleBytes];

            _bits.CopyTo(dataBytes, 0);
            if (_prevBits != null) _prevBits.CopyTo(prevDataBytes, 0);

            _preamble.CopyTo(preambleBytes, 0);
            string linePreamble = "";
            string lineChanges = "";
            string lineHexValues = "";
            string lineDeXValues = "";
            string lineIndex = "";

            int iX = 0;
            for (int i = dataBytes.Length - 1; i >= 0; --i)
            {
                lineIndex += i.ToString("d2") + " ";
                if (i < nrOfPreambleBytes)
                {
                    linePreamble += preambleBytes[i].ToString("X2") + " ";
                }
                if (_prevBits != null)
                {

                    if (dataBytes[i] != prevDataBytes[i])
                    {
                        lineChanges += "XX ";
                    }
                    else
                    {
                        lineChanges += "   ";
                    }
                    //lineChanges += prevDataBytes[i].ToString("X2") + " ";
                }
                lineHexValues += dataBytes[i].ToString("X2") + " ";
                if (i < nrOfBytes - startToDescrambe)
                {
                    byte by = (byte)(dataBytes[i] ^ xn297_scramble[iX++]);
                    lineDeXValues += by.ToString("X2") + " ";
                }
                else
                {
                    lineDeXValues += "   ";
                }
            }

            if (_preambleOffset > 0)
            {
                int x = (int)(linePreamble.Length + ((_preambleOffset / 8) * 3));
                linePreamble = linePreamble.PadLeft(x, ' ');
            }
            Console.WriteLine("P:" + linePreamble);     // Preamble
            Console.WriteLine("i:" + lineIndex);        // Index
            //Console.WriteLine("C:" + lineChanges);      // Changes
            Console.WriteLine("B:" + lineHexValues);    // Bytes
            //Console.WriteLine("X:" + lineDeXValues);    // Descrambled


            _prevBits = (BitArray)_bits.Clone();
        }


        //===================================================================================================
        // GetBytes
        //===================================================================================================
        byte[] GetBytes(BitArray bits)
        {
            int nrOfBytes = bits.Length / 8;
            if (bits.Length % 8 != 0)
            {
                nrOfBytes++;
            }

            byte[] dataBytes = new byte[nrOfBytes];

            bits.CopyTo(dataBytes, 0);
            return dataBytes;
        }

        public byte[] Bytes(int skip)
        {
            byte[] data = GetBytes(_bits);

            return data.Take(data.Length - skip).ToArray();
        }

        string BytesToHex(byte[] data)
        {
            string hex = "";
            foreach (byte b in data.Reverse())
            {
                hex += b.ToString("X2") + " ";
            }
            return hex;
        }

        string BitsToString(BitArray bits)
        {
            string sBits = "";
            for (int i = bits.Length - 1; i >= 0; i--)
            {
                sBits += bits[i] ? "1" : "0";
                if (i % 8 == 0) sBits += " ";

            }

            return sBits;
        }

        //===================================================================================================
        // DecodeXN297
        //===================================================================================================
        public void DecodeXN297()
        {
            int addressLength = 5;
            int dataLength = 6 + 1;//+1 => we need to shift this due to the PCF is 9 bits
            int iAddress = 0;
            byte[] dataBytes = GetBytes(_bits);

            int preambleIndex = Array.IndexOf(dataBytes, (byte)0x55);
            if (preambleIndex >= 0)
            {
                //Descramble
                int iX = 0;
                for (int i = preambleIndex - 1; i >= 0; i--)
                {
                    dataBytes[i] = (byte)(dataBytes[i] ^ xn297_scramble[iX++]);
                }


                iAddress = preambleIndex - addressLength;
                byte[] address = dataBytes.Skip(iAddress).Take(addressLength).ToArray();

                Console.WriteLine("Address:" + BytesToHex(address));

                int idata = iAddress - dataLength - 1;

                int iRest = 0x14;

                byte[] restData = dataBytes.Take(iRest).ToArray();

                BitArray restDataBits = new BitArray(restData);

                Console.WriteLine(BytesToHex(restData));
                Console.WriteLine(BitsToString(restDataBits));

                //byte[] data = dataBytes.Skip(idata).Take(dataLength).ToArray();
                //Console.WriteLine("dataNS :" + BytesToHex(data));

                //for (int i = data.Length -1; i > 1; i--) 
                //{
                //    data[i] = (byte)(((data[i] << 1)) & 0xFF | (data[i - 1] >> 7));
                //}

                //Console.WriteLine("data   :" + BytesToHex(data));

            }


        }

    }

    internal class Program
    {
        public static byte[] xn297_scramble = {
            0xE3, 0xB1, 0x4B, 0xEA, 0x85, 0xBC, 0xE5, 0x66,
            0x0D, 0xAE, 0x8C, 0x88, 0x12, 0x69, 0xEE, 0x1F,
            0xC7, 0x62, 0x97, 0xD5, 0x0B, 0x79, 0xCA, 0xCC,
            0x1B, 0x5D, 0x19, 0x10, 0x24, 0xD3, 0xDC, 0x3F,
            0x8E, 0xC5, 0x2F, 0xAA, 0x16, 0xF3, 0x95 };

        public static ushort[] xn297_crc_xorout_scrambled = {
            0x0000, 0x3448, 0x9BA7, 0x8BBB, 0x85E1, 0x3E8C,
            0x451E, 0x18E6, 0x6B24, 0xE7AB, 0x3828, 0x814B,
            0xD461, 0xF494, 0x2503, 0x691D, 0xFE8B, 0x9BA7,
            0x8B17, 0x2920, 0x8B5F, 0x61B1, 0xD391, 0x7401,
            0x2138, 0x129F, 0xB3A0, 0x2988, 0x23CA, 0xC0CB,
            0x0C6C, 0xB329, 0xA0A1, 0x0A16, 0xA9D0 };

        static ushort _prevCRC = 0;

        //
        // crc16_update
        //
        public static UInt16 crc16_update(UInt16 crc, byte a, byte bits)
        {
            const UInt16 polynomial = 0x1021;
            crc ^= (UInt16)(a << 8);

            while (bits-- > 0)
            {
                if ((UInt16)(crc & 0x8000) != 0)
                {
                    crc = (ushort)((crc << 1) ^ polynomial);
                }
                else
                {
                    crc = (ushort)(crc << 1);
                }
            }
            return crc;
        }


        //
        // Descramble
        //
        public static byte[] Descramble(byte[] packet, int iScrambleLookupStart)
        {
            byte[] desc = new byte[packet.Length];

            for (int i = 0; i < packet.Length; i++)
            {
                desc[i] = (byte)(packet[i] ^ xn297_scramble[iScrambleLookupStart + i]);
            }

            return desc;
        }

        //
        // ReverseBitsWith4Operations
        //
        public static byte ReverseBitsWith4Operations(byte b)
        {
            //See https://stackoverflow.com/questions/3587826/is-there-a-built-in-function-to-reverse-bit-order
            return (byte)(((b * 0x80200802ul) & 0x0884422110ul) * 0x0101010101ul >> 32);
        }


        //
        // BytesToHex
        //
        public static string BytesToHex(byte[] data)
        {
            string hex = "";
            foreach (byte b in data.Reverse())
            {
                hex += b.ToString("X2") + " ";
            }
            return hex;
        }

        //
        // DecodePacket XN297 'normal mode'
        //   This mode does not contain a PCF!
        //
        //     |----- adr ------------| |------------data-----------| |--crc--|   
        //     0x49 0x80 0x4A 0xCB 0xA5 0x3C 0xC5 0x95 0x81 0xFD 0x88 0x74 0xE9;
        //
        public static bool DecodePacketSimple(byte[] packet, int addresslen, int datalen, int crclen, bool descramble = false)
        {
            bool crcCheck = false;
            

            //
            // Get Address and descrambe
            //            
            byte[] address = Descramble(packet.Take(addresslen).ToArray(), 0);
            address = address.Reverse().ToArray();

            //
            // Get the data and descramble and reverse the bits
            //
            byte[] data = Descramble(packet.Skip(addresslen).Take(datalen).ToArray(), addresslen);
            for (int i = 0; i < data.Length; i++)
            {
                data[i] = ReverseBitsWith4Operations(data[i]);
            }
            data = data.Reverse().ToArray();

            //
            // Get the raw crc of the packet
            //
            byte[] crc_raw = packet.Skip(addresslen + datalen).Take(crclen).ToArray();
            ushort crc = (ushort)(crc_raw[1] + ((ushort)crc_raw[0] << 8));

            //if (_prevCRC != crc ) //Only output if crc is changed
            {
                //
                // output the data
                //
                // Complete data
                Console.Write(BytesToHex(packet.Reverse().ToArray()) + " >> ");

                // Descrambled address and data + crc
                string line = BytesToHex(address) + "| " + BytesToHex(data) + "| " + BytesToHex(crc_raw);
                Console.WriteLine(line);


                //Console.WriteLine("crc:" + crc.ToString("X4"));
            }
            _prevCRC = crc;

            return crcCheck;
        }


        //
        // Main
        //
        static void Main(string[] args)
        {
            BitArray preambleBits = new BitArray(new byte[] { 0x55, 0x0F, 0x71 });

            const int preambleOffset = 2 * 8; // 2 extra byte to capture before the preamble
            const int nrOfBits = 18 * 8;
            BitShiftArray bits = new BitShiftArray(nrOfBits, preambleBits, preambleOffset);

            var socket = new SubscriberSocket();
            socket.Connect("tcp://127.0.0.1:5555");
            socket.SubscribeToAnyTopic();

            int x = 0;
            var watch = new System.Diagnostics.Stopwatch();
            while (true)
            {
                byte[] data = socket.ReceiveFrameBytes();

                foreach (byte b in data)
                {

                    if (bits.AddBit(b != 0))
                    {
                        watch.Restart();

                        //bits.HexDump(5);

                        byte[] packet = bits.Bytes(5);

                        packet = packet.Reverse().ToArray();


                        DecodePacketSimple(packet, 5, 6, 2, true);

                        bits.Clear();
                        x++;

                    }
                    if (watch.IsRunning && watch.ElapsedMilliseconds > 1000)
                    {
                        watch.Stop();
                        Console.WriteLine("------------------------------------------------------------------------------------------------------------");
                        Console.WriteLine($"Packets recieved:#{x}");
                        Console.WriteLine();
                    }



                }
            }
        }
    }
}
