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
int ampMeterPin = 3;
int voltMeterPin = 5;

// Remember the current positions on each device
const byte HASH_SIZE = 5; 

HashType<int,int> positionRawArray[HASH_SIZE];
HashMap<int,int> positions = HashMap<int,int>(positionRawArray, HASH_SIZE ); 

HashType<int,int> fsdRawArray[HASH_SIZE];
HashMap<int,int> fsds = HashMap<int,int>(fsdRawArray, HASH_SIZE ); 

HashType<int,int> zeroedPinoutsArray[HASH_SIZE];
HashMap<int,int> zeroedPinouts = HashMap<int,int>(zeroedPinoutsArray, HASH_SIZE );

HashType<int,int> fsdPinoutsArray[HASH_SIZE];
HashMap<int,int> fsdPinouts = HashMap<int,int>(fsdPinoutsArray, HASH_SIZE );

int panDelay = 100;

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
        
        if (strcmp(name, "amps") == 0) {         
          int dest = stringToInt(value);                 
          if (dest >= 0 && dest <= fsds.getValueOf(ampMeterPin)) {
             server.print(name);
             server.print(":");
             server.print(dest);
             server.print(setMeterTo(ampMeterPin, dest));
          }
        }
                
      }
    }
  }
}



void setup() {  
  // PWM voltage corresponding to zero and FSD for meters
  zeroedPinouts[0](ampMeterPin, 0);
  zeroedPinouts[1](voltMeterPin, 0);
  
  fsdPinouts[0](ampMeterPin, 250);
  fsdPinouts[1](voltMeterPin, 250);
  
  // The full scale deflection value on the face of the amp meter
  fsds[0](ampMeterPin, 100);
  fsds[1](voltMeterPin, 80);
  
  // All devices are initially zeroed  TODO - need to be able to define rest locations for all - ie. gauges should reset in center - 0 could be of scale 
  positions[0](ampMeterPin, 0);
  positions[1](voltMeterPin, 0);
  
  // Power up meter pins
  pinMode(ampMeterPin, OUTPUT);
  pinMode(voltMeterPin, OUTPUT);
  
  // Onboard LED pin used to signal activity only
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  
  // Start serial and Ethernet comms
  Serial.begin(9600);    
  Ethernet.begin(mac, ip);
  webserver.addCommand("index.html", &mainCmd);
  webserver.begin();
  
  setMeterTo(ampMeterPin, 20);
  setMeterTo(voltMeterPin, 20);
  delay(2000);
 
  setMeterTo(ampMeterPin, 0);
  setMeterTo(voltMeterPin, 0);
}

void loop()  {
  webserver.processConnection();  
  delay(10);
}


// Calculate the correct PWM voltage required to move the a meter needle to the desired position
int setMeterTo(int pin, int dest) {   
  double ratioOfFSD =(double) dest / positions.getValueOf(pin);  
  
  int zeroedPWMValue = zeroedPinouts.getValueOf(pin);
  int fsdPWMValue = fsdPinouts.getValueOf(pin);
  
  int offset = zeroedPWMValue + ((ratioOfFSD) * (fsdPWMValue - zeroedPWMValue));
  if (offset >= zeroedPWMValue && offset <= fsdPWMValue) {
    int currentMeterPosition = positions.getValueOf(pin);
    panMeterFromTo(pin, offset);
  }
  return offset;
}


// Move the needle in an orderly fashion to avoid recoil.
// The needle has a fair amount of momentum and we don't want to damage the spring with sudden stops.
// Adding alot of capacitance across the meter would also be a good thing todo (say > 200mF)
void panMeterFromTo(int pin, int offset) { 
    int currentPosition = positions.getValueOf(pin);
    while (currentPosition < offset && offset <= fsdPinouts.getValueOf(pin)) {
       currentPosition = currentPosition + 1;
       Serial.print("Current: ");
       Serial.println(currentPosition, DEC);
       analogWrite(ampMeterPin, currentPosition);
       delay(panDelay);
     }
   
     while (currentPosition > offset && offset >=  zeroedPinouts.getValueOf(pin)) {
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

int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}
