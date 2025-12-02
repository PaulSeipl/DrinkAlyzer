#include <DS18B20.h>

DS18B20 ds(2);

void setup() {
  Serial.begin(9600);
}

void loop() {
  while (ds.selectNext()) {
    Serial.println(ds.getTempC());
  }
}