# Modbus slave test with Dragino RS485-BL
This sketch is supposed to test Dragino RS485-BL (Modbus master), using an Arduino UNO as a Modbus slave.

The Dragino will be configured to read data stored in Arduino variables (an array), when triggered by an interrupt. The interrupt can be sent manually with a button, or periodically by Arduino itself using a relay.

## Hardware setup
The minimal hardware needed is:
* Dragino RS485-BL (http://wiki.dragino.com/xwiki/bin/view/Main/User%20Manual%20for%20LoRaWAN%20End%20Nodes/test/#H1.Introduction)
* Arduino UNO (https://docs.arduino.cc/hardware/uno-rev3)
* MAX485 RS485 to TTL adapter pcb board
![Alt text](/doc/bare_minimum_bb.png "Bare minimum setup")
Connections:

| From        | To           |
| ------------- |:-------------:|
| Arduino pin 4      | MAX485 pin DE |
| Arduino pin 4      | MAX485 pin RE |
| Arduino pin 1      | MAX485 pin DI |
| Arduino pin 0      | MAX485 pin R0 |
| Arduino 5V         | MAX485 pin VCC |
| Arduino GND         | MAX485 pin GND |
| MAX485 pin A       | Dragino pin A |
| MAX485 pin B       | Dragino pin B |

You can also add these other optional devices to Arduino:
* 5V Relay pcb
* DS18B20 temperature sensor
![Alt text](/doc/full_example_bb.png "Full setup")

Connections:

| From        | To           |
| ------------- |:-------------:|
| Arduino pin 3      | Relay pin S |
| Arduino 5V         | Relay pin VCC |
| Arduino GND         | Relay pin GND |
| Arduino pin 5      | DS18B20 pin DATA |
| Arduino 3.3V or 5V      | DS18B20 pin VCC |
| Arduino GND         | DS18B20 pin GND |

Please remember to place a 4.7kOhm resistor between DS18B20 data and vcc pins. If you have a DS18B20 breakout pcb the resistor is probably already in place.

And, only for debugging:
* TTL to USB module for debugging Dragino
* RS485 to USB adapter for debugging RS485 bus

![Alt text](/doc/test-setup.png "Debugging setup")

## Basic concepts
In modbus protocol, a slave keeps data (usually numbers) in registers. The master can ask slaves for their register's content.

Arduino has an array of 16bit unsigned integers, which can hold 4 values. These will be the registers for Modbus protocol. Some of these array values will be written by the master using a Modbus command, others will be generated randomly by Arduino itself or set as the reading from a sensor (a DS18B20 thermometer).

### Data Type
All values in modbus are stored as 16bit unsigned integers (in brief: uint), because that's the most common data type in modbus use cases. This does not mean we can not represent other data types, they just need to be encoded as "uint". In this example, the first register is treated as a boolean, the second one as an unsigned integer, the third one as a signed integer (which allows negative numbers) and the third one as a float with 2 decimal digits.

## Configuring Arduino (upload code)
This sketch should work out of the box, but you can change a few settings before uploading it to Arduino.
```
// OPTIONAL CONFIGURATION
//Comment this line to avoid using a relay to trigger Dragino LoRaWAN data transmission
#define RELAY 3        //Digital pin connected to the relay

//Comment this line to avoid using DS18B20 temperature sensor
#define ONE_WIRE_BUS 5  //Digital pin connected to the DS81B20 sensor

// Uncomment these lines to use Software Serial for RS-485 communication
//#define SWSERIAL_RX 6  //Digital pin used for software serial rx
//#define SWSERIAL_TX 7  //Digital pin used for software serial tx
```
By default this sketch uses Arduino UNO single HardwareSerial (digital pin 0 and 1) for both serial console debugging and RS485 modbus protocol: these two signals can coexist. Anyway, if you prefer to use a SoftwareSerial, you just have to set SWSERIAL_RX and SWSERIAL_TX numbers.

## Configuring Dragino (first time)
Connect PC to Dragino using the UART pins and a USB-to-TTL adapter. Then, connect to it from your computer. Example using "screen" for GNU/Linux:
```
screen /dev/ttyUSB0 9600
```
and write
```
123456
```
as password.
```
AT+MOD=1
ATZ
AT+BAUDR=9600
AT+PARITY=0
AT+STOPBIT=0
AT+DATABIT=8
```

## Setting values to slave registers from master
To test the RS485 connection between Dragino and Arduino, you can try this command from Dragino UART console:
```
AT+CFGDEV=01 10 00 00 00 03 06 00 01 01 0E 80 B4 DA C8,1
```
As soon as you run this command, you should see Arduino registers value changing. You can check this out on [Arduino Serial Monitor](#debug-with-arduino-serial-monitor).

### Explaination
To write registers on the slave, we use function Code: 16 (hex: 0x10, Writes multiple holding registers)
```
[Device ID 01] [Function Code 10] [Starting Register Address HI Byte 00] [Starting Register Address LO Byte 00] [Quantity of Registers to Write HI Byte 00] [Quantity of Registers to Write HI Byte 03] [Quantity of Data Bytes to Write 06] [Data bytes to write 00 01 01 0E 80 B4] [Two Byte CRC DA C8]
```
This means we are writing 3 registers, starting from register number 0. The first one will hold the number 0x0001 (uint: 1), the second one will have 010E (uint: 270) value and the last one will store 80B4 (uint: 32948).

## Setup read command (designed to work with this Arduino sketch)
```
AT+COMMAND1=01 03 00 00 00 04 44 09,1
AT+DATACUT1=15,2,3~15
AT+CMDDL1=1000
```
### Explaination
To read registers we use function code 03 (Read holding registers). Register numbers start from 0, and need to read 4 register.
```
[Device ID 01] [Function Code 03] [Starting Register Address HI Byte 00] [Starting Register Address LO Byte 00] [HI Number of Registers 00] [LO Number of Registers 04] [Two Byte CRC 44 09]
```
### Testing
Using Dragino UART interface:
```
AT+GETSENSORVALUE=0
```
You should see this output:
```
CMD1     = 01 03 00 00 00 03 05 cb 00 00 
RETURN1  = 01 03 06 00 01 00 00 00 1E 21 75 00 00 00 00 
Payload  = 0d 31 01 06 00 01 00 00 00 1E 21 75 00 00 00 00
```

## Testing with a USB-RS485 module
### Modpoll
Modpoll (https://www.modbusdriver.com/modpoll.html) is a master simulator, usefult to test a RS485 slave.
Command to write 3 values as 16bit integers (data type 4):
```
./modpoll/x86_64-linux-gnu/modpoll -b 9600 -p none -m rtu -d 8 -s 1 -a 1 -t 4 -c 3 /dev/ttyUSB0 1 0 180
```
This writes values 1, 0, and 180 in registers 1, 2 and 3. This is the full hex command:
```
01 10 00 00 00 03 06 00 01 00 00 01 0E 5B 14
```

To read 3 hex values from a RS485 slave, you can use this command:
```
./modpoll/x86_64-linux-gnu/modpoll -b 9600 -p none -m rtu -d 8 -s 1 -a 1 -t 4:hex -c 3 /dev/ttyUSB0
```
This is the full hex command:
```
01 03 00 00 00 03 05 CB
```

### Diagslave
Diagslave (https://www.modbusdriver.com/diagslave.html) simulates a slave, and can be usefult to see commands issued from Dragino, to check if it is working:
```
./diagslave/i686-linux-gnu/diagslave -a 1 -m rtu -b 9600 -d 8 -s 1 -p none /dev/ttyUSB1
```

### RTUmaster

RTUmaster (https://www.simplymodbus.ca/RTUmaster.htm) is a Windows software with a GUI to simulate a master and check if Arduino is working. It can also be used on GNU/Linux with Wine:
```
rm ~/.wine/dosdevices/com1                 
ln -s /dev/ttyUSB0 ~/.wine/dosdevices/com1 
wine regedit
```
Creare nuovo valore in HKEY_LOCAL_MACHINE\Software\Wine\Ports
con nome COM1 e valore /dev/ttUSB0
```
wineserver -k
```



# Debug with Arduino Serial Monitor

At every reboot you should have these values in the register:
```
Register 1 (bool): FALSE
Register 2 (uint): 0
Register 3 (int): -32768
Register 4 - Temperature (Celsius*100):2575
```

After issuing registers init command:
```
Register 1 (bool): TRUE
Register 2 (uint): 270
Register 3 (int): 180
Register 4 - Temperature (Celsius*100):2556
```

# References
https://dghcorp.com/modbus-overview/
https://logicio.com/HTML/ioext-modbuscommands.htm
https://npulse.net/en/online-modbus
