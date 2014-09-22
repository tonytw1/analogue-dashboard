#include <PubSubClient.h>
#include <SPI.h>
#include <HashMap.h>
#include "Ethernet.h"

byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetClient ethClient;
byte server[] = { 
  192, 168, 1, 10 };
PubSubClient client(server, 1883, callback, ethClient);

const byte NUMBER_OF_DEVICES = 6; 

char* devices[] = {"voltmeter1", "ammeter1", "master-green", "master-red", "secondary-green", "secondary-red"}; 
char* deviceTypes[]= {"gauge", "gauge", "indicator", "indicator", "indicator", "indicator"};
int scales[] = {80, 100, 100, 100, 100, 100};
int pwmMax[] = {232, 250, 255, 255, 255, 255};

int panDelays[] = {50, 100, 20, 20, 20, 20};

int present[] = {0, 0, 0, 0, 0, 0};
int destinations[] = {0, 0, 0, 0, 0, 0};
unsigned long nextSteps[] = {0, 0, 0, 0, 0, 0};
int pins[] = {8, 9, 7, 6, 5, 4};

unsigned long nextAdvertisement = 0;

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < NUMBER_OF_DEVICES; i = i + 1) {
    if (pins[i] > 0) {
      pinMode(pins[i], OUTPUT);
      analogWrite(pins[i], 0);       
    }
  }

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    // failed to dhcp no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }

  if (client.connect("gauges")) {
    client.publish("gauges","Gauges connected");
    client.subscribe("gauges");         
  }

}

void advertiseDevices() {  
  for (int i = 0; i < NUMBER_OF_DEVICES; i = i + 1) {
    char buf[50];
    char* scale = itoa(scales[i], buf, 10);

    String description = String(devices[i]);
    description = "gauge:" + description + "," + deviceTypes[i] + "," + scale;

    description.toCharArray(buf, 50);
    client.publish("gauges", buf);
  }
}

void loop() {
  client.loop();

  if (millis() > nextAdvertisement) {
    advertiseDevices();
    nextAdvertisement = millis() + 60000; 
  }

  for (int i = 0; i < NUMBER_OF_DEVICES; i = i + 1) {
    // TODO if gauges and counters only
        
    if (millis() > nextSteps[i]) {

       if (deviceTypes[i] == "gauge" || deviceTypes[i] == "indicator") {
          if (present[i] < destinations[i]) {         
            int nextStep = present[i] + 1;
            present[i] = nextStep;
            analogWrite(pins[i], nextStep);
            
            // Fading lamps need to be constantly pinged to remain lite
            if (deviceTypes[i] == "indicator" && nextStep == destinations[i]) {
              destinations[i] = 0;
              panDelays[i] = 200;
            }
            
    
          } 
          else if (present[i] > destinations[i]) {
            int nextStep = present[i] - 1;
            present[i] = nextStep;
            analogWrite(pins[i], nextStep);
          }          
       }
                    
       nextSteps[i] = millis() + panDelays[i];
    }     
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  //check for paylaod; is it there at all ?
  if (length > 0) {    
    // create character buffer with ending null terminator (string)
    int i = 0;
    char message_buff[100];
    for(i=0; i<length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';

    String payLoadString = String(message_buff);   
    Serial.println("Received message: " + payLoadString);

    for (int i = 0; i < NUMBER_OF_DEVICES; i = i + 1) {
      String deviceName = devices[i];
      if(payLoadString.startsWith(deviceName)) {
        String valueString = payLoadString.substring(deviceName.length() + 1, payLoadString.length());
        Serial.println("Message is a value for device: " + deviceName + ": " + valueString);
        
        if (deviceTypes[i] == "gauge") {
          float value = stringToFloat(valueString);
          setMeterTo(i, value);
        }

        if (deviceTypes[i] == "indicator") {
           panDelays[i] = 20;
          if (valueString == "true") {
            destinations[i] = scales[i];            
          } else {
            destinations[i] = 0;            
          }  
        }
        
      }
    } 
  }
}

// Calculate the correct PWM voltage required to move the a meter needle to the desired position and set this as the required destination
void setMeterTo(int deviceNumber, float dest) {
  if (dest >= scales[deviceNumber]) {
    dest = scales[deviceNumber];
  }

  int zeroedPWMValue = 0;  // TODO
  int fsdPWMValue = pwmMax[deviceNumber];

  double ratioOfFSD = dest / scales[deviceNumber];
  int pwmDestination = ((ratioOfFSD) * (fsdPWMValue - zeroedPWMValue)) + zeroedPWMValue;

  destinations[deviceNumber] = pwmDestination;
}

float stringToFloat(String valueString) {
  char numbers[100];
  valueString.toCharArray(numbers, valueString.length() + 1);
  return atof(numbers);
}


