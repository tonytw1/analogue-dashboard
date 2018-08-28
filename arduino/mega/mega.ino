unsigned int announcementInterval = 10000;
unsigned long updateTTL = 90000;

unsigned long nextAnnouncement = 0;

String inputString = "";     // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

unsigned int lampPins[] = {8, 9, 10, 11, 6, 7};
String lampNames[] = {"green", "red", "green2", "red2", "amps", "volts"};
unsigned long lampNextPan[] = {0, 0, 0, 0, 0, 0};
unsigned int lampPanSpeeds[] = {5, 5, 5, 5, 100, 100};
unsigned int lampFSDs[] = {1, 1, 1, 1, 100, 80};

int lampTargets[] = {0, 0, 0, 0, 0, 0};
int lampValues[] = {0, 0, 0, 0, 0, 0};
long lampLastUpdated[] = {0, 0, 0, 0, 0, 0};

void setup() {
  inputString.reserve(200);
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  int i;
  int numberOfElements = sizeof(lampPins) / sizeof(lampPins[0]);
  for (i = 0; i < numberOfElements; i = i + 1) {
    int lampPin = lampPins[i];
    pinMode(lampPin, OUTPUT);
    digitalWrite(lampPin, LOW);
  }
}

void loop() {
  if (stringComplete) {
    processInput();
  }

  pan();
  
  expireStaleLamps();
  
  if (millis() > nextAnnouncement) {
    announce();
  }   
}

void announce() {
  int i;
  int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
  for (i = 0; i < numberOfElements; i = i + 1) {
    Serial.println("lamp:" + lampNames[i] + "[" + lampFSDs[i] + "]");
  }
  nextAnnouncement = millis() + announcementInterval;
}

void pan() {
  int i;
  int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
  for (i = 0; i < numberOfElements; i = i + 1) {
    if (lampNextPan[i] < millis()) {
      int lampPin = lampPins[i];
      int value = lampValues[i];
      int target = lampTargets[i];

      int next = value;
      if (target < value) {
        next = next - 1;
        analogWrite(lampPin, next);
        lampValues[i] = next;
      
      } else if (target > value) {
        next = next + 1;
        analogWrite(lampPin, next);
        lampValues[i] = next;
      }
     
      lampNextPan[i] = millis() + lampPanSpeeds[i];
    }
  }
}

void processInput() {
  if (inputString.length() > 0 ) {
    int i;
    int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
    for (i = 0; i < numberOfElements; i = i + 1) {
      String lampName = lampNames[i];
      String prefix = lampName + ":";

      if (inputString.startsWith(prefix)) {
        String value = inputString.substring(prefix.length());
        int pin = lampPins[i];

        double ratioOfFSD =(double) stringToInt(value) / lampFSDs[i];
  
        int zeroedPWMValue = 0;
        int fsdPWMValue = 255;
  
        int offset = zeroedPWMValue + ((ratioOfFSD) * (fsdPWMValue - zeroedPWMValue));
        lampTargets[i] = offset;
        lampLastUpdated[i] = millis() + updateTTL;
      }
    }
 }

 inputString = "";
 stringComplete = false;
}

void expireStaleLamps() {
  int i;
  int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
  for (i = 0; i < numberOfElements; i = i + 1) {
      long lastUpdated = lampLastUpdated[i];
      if (lastUpdated < millis()) {
        lampTargets[i] = 0;
      }
  }  
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char) Serial.read();
    // add it to the inputString:
    if (inChar != '\n') {
      inputString += inChar;
    }
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}

