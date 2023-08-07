/*
 * 
 * Arduino Modbus RS485 slave for Dragino RS485-BL
 *
 * Created by Luca Tringali (2023)
 *
 * Released under GNU LGPL. 
 * 
 */

/*
 * CONFIGURATION
 */
 
// MANDATORY CONFIGURATION
#define RS485_DERE 4  //Digital pin connected to DE & RE pin of RS-485
#define SLAVE_ID 0x01  //Modbus slave address 8bit
const unsigned long sample_interval = 60000; //milliseconds interval for sending data to Dragino, max value is 4294967295
uint16_t modbus_array[] = {0,0,0,0};    //Initialization for Modbus Holding registers: this array should have at least one element, for this example we define 4 different registers (each one will have with 0 as value at boot)

// OPTIONAL CONFIGURATION
//Comment this line to avoid using a relay to trigger Dragino LoRaWAN data transmission
#define RELAY 3        //Digital pin connected to the relay

//Comment this line to avoid using DS18B20 temperature sensor
#define ONE_WIRE_BUS 5  //Digital pin connected to the DS81B20 sensor

// Uncomment these lines to use Software Serial for RS-485 communication
//#define SWSERIAL_RX 6  //Digital pin used for software serial rx
//#define SWSERIAL_TX 7  //Digital pin used for software serial tx

/*
 * CONFIGURATION END
 */
 
/*
 * Global variables
 */

unsigned long oldms = 0;

//Many thanks to smarmengol: https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino
#include "src/Modbus-Master-Slave-for-Arduino-master/ModbusRtu.h"

const uint16_t int_min_value = -32768;  //signed 16 bit integer ranges from -32768 to 32767
#ifdef SWSERIAL_RX
#include <SoftwareSerial.h>
SoftwareSerial myserial(SWSERIAL_RX, SWSERIAL_TX);
//Modbus slave ID, RS-485 module comunication on Software Serial, and Arduino digital pin connected to both DE & RE pins of RS-485
Modbus slave(SLAVE_ID,myserial,RS485_DERE);
#else
//Modbus slave ID, RS-485 module comunication on Hardware Serial, and Arduino digital pin connected to both DE & RE pins of RS-485
Modbus slave(SLAVE_ID,Serial,RS485_DERE);
#endif

#ifdef ONE_WIRE_BUS
#include <OneWire.h> 
#include <DallasTemperature.h>
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
#endif

void setup()
{
  Serial.begin(9600);
  Serial.println("Arduino Modbus Slave");
  delay(5000);

  #ifdef RELAY
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  #endif
  
  Serial.begin(9600);  //Init serial at 9600 baud
  #ifdef SWSERIAL_RX
  myserial.begin(9600);
  #endif
  
  slave.start();
}

void loop()
{
  //Read commands from master
  slave.poll(modbus_array,sizeof(modbus_array)/sizeof(modbus_array[0]));

  //Process data stored into registers and write them to Hardware Serial for debugging
  
  if (modbus_array[0] == 0)//Depends upon value in modubus_array[0] written by Master Modbus
  {
    Serial.println("Register 1 (bool): FALSE");
  }
  else
  {
     Serial.println("Register 1 (bool): TRUE");
  } 

  Serial.print("Register 2 (uint): "); 
  Serial.println(modbus_array[1]);
  
  int num = (modbus_array[2]+int_min_value); //Convert from uint to int
  Serial.print("Register 3 (int): ");
  Serial.println(num);              

  Serial.print("Register 4 - ");  
  #ifdef ONE_WIRE_BUS
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  temperature = round(temperature*100);  //Reading Celsius temperature in cents (e.g.: 25.70 becomes 2570)
  uint16_t temp = temperature-int_min_value; //Manually converting from int to uint. To get back the actual value you should just subtract 32768 (and divide by 100)
  modbus_array[3] = temp;
  Serial.print("Temperature (Celsius*100):");
  Serial.println(int(temperature));
  #else
  uint16_t temp = random(0,65535);
  modbus_array[3] = temp;
  Serial.print("Random value:");
  Serial.println(temp);
  #endif
  
  //Trigger Dragino on interval
  #ifdef RELAY
  //Serial.println((millis() - oldms));
  //Serial.println(sample_interval);
  if (((millis() - oldms) >  sample_interval) || (millis() < oldms)) {  //millis will overflow to 0 every 50 days
    oldms = millis();
    Serial.println("Interval reached, triggering Dragino...");
    digitalWrite(RELAY, HIGH);
    delay(1000);
    digitalWrite(RELAY, LOW);
  }
  #endif

  delay(200); 
  
}
