#define HUM_SENS_PIN A0
#define MOISTURE_MEASURES 10
#define MOISTURE_READ_WAIT 2000

void setup() {
   float dry = 0;
   float read = 0;

   Serial.begin(19200);


   Serial.print(MOISTURE_MEASURES * MOISTURE_READ_WAIT / 60000);
   Serial.println("s");
   for (int k = 0; k < MOISTURE_MEASURES; k++) {
     read = analogRead(HUM_SENS_PIN);
     dry += read;
     Serial.println(read);

     delay(MOISTURE_READ_WAIT);
   }
   dry = dry / MOISTURE_MEASURES;

   Serial.print("AVERAGE: ");
   Serial.println(dry);
}

void loop (){

} 



// int sensorPin = A0;   // select the input pin for the potentiometer
// int ledPin = 13;      // select the pin for the LED
// int sensorValue = 0;  // variable to store the value coming from the sensor

// int milliVolts = 0;
// int temperature =0;


// void setup() {
//   // declare the ledPin as an OUTPUT:
//   pinMode(ledPin, OUTPUT);
//   Serial.begin(9600);
// }

// void loop() {
//   // read the value from the sensor:
//   sensorValue = analogRead(sensorPin);
//   // turn the ledPin on
//   digitalWrite(ledPin, HIGH);
//   // stop the program for <sensorValue> milliseconds:
//   delay(sensorValue);
//   // turn the ledPin off:
//   digitalWrite(ledPin, LOW);
//   // stop the program for <sensorValue> milliseconds:
//   delay(sensorValue);
//   // Serial.println(sensorValue);

//   milliVolts = sensorValue * 3300/1024;
//   temperature = (milliVolts - 500) / 10;
//   Serial.println(temperature);
// }