// Drives a 7595 latched 7 segment display

String serialInput = "";

int dataPin = 53;
int latchPin = 52;
int clockPin = 51;

int count = 0;

unsigned long countNextStep = 0;

void setup() {
    Serial.begin(115200);    
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);    
    pinMode(latchPin, OUTPUT); 
    loop();   
}

void refreshCounter(int c, int dataPin, int latchPin, int clockPin) {

 boolean leadingBlank = true;           
   for (int i = 3; i >= 0; i--) {     
      int num = c%10;
       if (i == 1) {
         num = (c/10%10);
       }
       if (i == 2) {
         num = (c/100%10);
       }
       if (i == 3) {
         num = (c/1000%10);
       }
        
       if (num > 0 || i == 0) {
         leadingBlank = false; 
       }
                
       byte digit = num << 4;
       if (!leadingBlank) {
         bitSet(digit, i);  // TODO document the encoding of digit; looks like half of the digit is been used as address lines
       }

       digitalWrite(latchPin, LOW);
       shiftOut(dataPin, clockPin, LSBFIRST, digit);
       digitalWrite(latchPin, HIGH);
    }

}

void loop()  {    
   refreshCounter(count, dataPin, latchPin, clockPin);
   readSerial();    
}

void readSerial() {
     String payLoadString = "";
     if (Serial.available() > 0) {
         char incomingByte = Serial.read();
         if (incomingByte == '\n') {
            payLoadString = serialInput;
            serialInput = "";
       } else {
          serialInput.concat(incomingByte);
       }
       
    } else {
       return; 
    }

    Serial.println(payLoadString);
    if(payLoadString.startsWith("count:")) {
         String valueString = payLoadString.substring(6, payLoadString.length());
         count = stringToInt(valueString);
    }  
    
}

int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}
