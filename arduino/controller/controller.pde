#include <Servo.h>
#include <SPI.h>
#include "Ethernet.h"
#include <WebServer.h>  // http://code.google.com/p/webduino/
#include <HashMap.h>

// Ethernet config
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static uint8_t ip[] = { 192, 168, 1, 30 };
WebServer webserver("", 80);

// Pins which devices are attached to.
// The servo driver and Ethernet shield tend to disable PWM pins 9 and above.
// PWM doesn't seeem to be available on pin 4 ethier.
int gaugePin = 2;
int ampMeterPin = 3;
int voltMeterPin = 5;


// Remember the current positions on each device
const byte HASH_SIZE = 5; 
HashType<int,int> hashRawArray[HASH_SIZE]; 
HashMap<int,int> positions = HashMap<int,int>( hashRawArray , HASH_SIZE ); 

// The full scale deflection value on the face of the amp meter
int ampMeterFSD = 100;
int voltMeterFSD = 80;

// PWM voltage corresponding to zero and FSD on the amp meter
int ampMin = 0; 
int ampMax = 250;
int voltMin = 0; 
int voltMax = 250;

int panDelay = 100;

// Servo offsets for zero and FSD for the pressure gauge
int gaugeMin = 79;
int gaugeCenter = 105;
int gaugeMax = 136;

Servo servo;

#define NAMELEN 32
#define VALUELEN 32

void mainCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
  server.httpSuccess();
  
  if (type != WebServer::HEAD) {
    
    URLPARAM_RESULT rc;
    bool repeat;
    char name[NAMELEN], value[VALUELEN];
   
    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (!(rc == URLPARAM_EOS)) {
        if (strcmp(name, "gauge") == 0) {         
          int dest = stringToInt(value);                 
          if (dest >= gaugeMin && dest <= gaugeMax) {
             server.print(name);
             server.print(":");
             server.print(dest);
             moveGaugeTo(dest);
          }
        }
        
        if (strcmp(name, "amps") == 0) {         
          int dest = stringToInt(value);                 
          if (dest >= 0 && dest <= ampMeterFSD) {
             server.print(name);
             server.print(":");
             server.print(dest);
             server.print(setAmpMeterTo(dest));
          }
        }
                
      }
    }
  }
}


void setup() {
  
  // All devices are initially zeroed
  positions[0](ampMeterPin, 0);
  positions[1](voltMeterPin, 0);
  positions[2](gaugePin, 0);
       
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  pinMode(ampMeterPin, OUTPUT);
  pinMode(voltMeterPin, OUTPUT);
  
  // Start serial and Ethernet comms
  Serial.begin(9600);    
  Ethernet.begin(mac, ip);
  webserver.addCommand("index.html", &mainCmd);
  webserver.begin();
    
    
 
  setAmpMeterTo(20);
  setVoltMeterTo(20);
  delay(2000);
 
  setAmpMeterTo(0);
  setVoltMeterTo(0);
  moveGaugeTo(gaugeCenter);
}


void loop()  {
  webserver.processConnection();  
  delay(10);
}


// Calculate the correct PWM voltage required to move the amp meter needle to the desired position
int setAmpMeterTo(int dest) {   
  double ratioOfFSD =(double) dest / ampMeterFSD;  
  int offset = ampMin + ((ratioOfFSD) * (ampMax - ampMin));  
  if (offset >= ampMin && offset <= ampMax) {
    int currentAmpMeterPosition = positions.getValueOf(ampMeterPin);
    panMeterFromTo(ampMeterPin, offset);  
  }
  return offset;
}

int setVoltMeterTo(int dest) {
  double ratioOfFSD =(double) dest / voltMeterFSD;  
  int offset = voltMin + ((ratioOfFSD) * (voltMax - voltMin));  
  if (offset >= voltMin && offset <= voltMax) {
     panMeterFromTo(voltMeterPin, offset);  
  }
  return offset;
}

// Move the needle in an orderly fashion to avoid recoil.
// The needle has a fair amount of momentum and we don't want to damage the spring with sudden stops.
// Adding alot of capacitance across the meter would also be a good thing todo (say > 200mF)
void panMeterFromTo(int pin, int offset) { 
    int currentPosition = positions.getValueOf(pin);
    while (currentPosition < offset && offset <= ampMax) {
       currentPosition = currentPosition + 1;
       Serial.print("Current: ");
       Serial.println(currentPosition, DEC);
       analogWrite(ampMeterPin, currentPosition);
       delay(panDelay);
     }
   
     while (currentPosition > offset && offset >= ampMin) {
      currentPosition = currentPosition - 1;
      Serial.print("Current: ");
      Serial.println(currentPosition, DEC);
      analogWrite(ampMeterPin, currentPosition);
      delay(panDelay);
   }
   
   recordPosition(pin, currentPosition);
   return;
}

void recordPosition(int pin, int position) {
   //TODO this hash implementation has no put method!
}

// Slowly walk the gauge needle to the desired position to avoid stressing the mechanism with snappy movements.
void moveGaugeTo(int dest) {
   servo.attach(gaugePin, 0, 250);  // TODO define as constants
   int currentPos = servo.read(); 
   while (currentPos < dest && dest <= gaugeMax) {
       currentPos = currentPos + 1;
       servo.write(currentPos);
       delay(panDelay);
   }
   
   while (currentPos > dest && dest >= gaugeMin) {
      currentPos = currentPos - 1;
      servo.write(currentPos);
      delay(panDelay);
   }
   delay(2000);
   servo.detach();
}


int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}
