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