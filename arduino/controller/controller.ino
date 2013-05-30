#include <PubSubClient.h>
#include <SPI.h>
#include "Ethernet.h"

// Ethernet config
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetClient ethClient;
byte server[] = { 192, 168, 1, 10 };
PubSubClient client(server, 1883, callback, ethClient);

void setup() {
   Serial.begin(9600);
   
   // start the Ethernet connection:
   if (Ethernet.begin(mac) == 0) {
      // failed to dhcp no point in carrying on, so do nothing forevermore:
      for(;;)
      ;
  }
  
  if (client.connect("zabbix")) {
      client.publish("gauges","arduino connected");
      client.subscribe("zabbix");         
  }
  
}

void loop()  {
   client.loop(); 
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
  
      publishString("ping");


    String payLoadString = String(message_buff);   
    Serial.println(payLoadString);
    publishString(payLoadString);   
  }    
}

void publishString(String message) {
    char charBuf[100];
    message.toCharArray(charBuf, 100);
    client.publish("gauges", charBuf);
}
