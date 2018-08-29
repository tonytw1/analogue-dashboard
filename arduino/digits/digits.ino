// Drives a 7595 latched 7 segment display

String serialInput = "";

int dataPin = 9;
int latchPin = 8;
int clockPin = 7;

int count = 0;

unsigned long countNextStep = 0;

void setup() {
    Serial.begin(115200);    
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);    
    pinMode(latchPin, OUTPUT); 
    loop();   
}

void loop()  {    
   boolean leadingBlank = true;           
   for (int i = 3; i >= 0; i--) {     
      int num = count%10;
       if (i == 1) {
         num = (count/10%10);
       }
       if (i == 2) {
         num = (count/100%10);
       }
       if (i == 3) {
         num = (count/1000%10);
       }
        
       if (num > 0 || i == 0) {
         leadingBlank = false; 
       }
                
       byte digit = num << 4;
       if (!leadingBlank) {
         bitSet(digit, i);
       }

       digitalWrite(latchPin, LOW);
       shiftOut(dataPin, clockPin, LSBFIRST, digit);
       digitalWrite(latchPin, HIGH);
    }

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
