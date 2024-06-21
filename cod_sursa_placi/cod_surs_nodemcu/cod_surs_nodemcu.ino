#define BLYNK_TEMPLATE_ID "TMPL465Q3GZHH"
#define BLYNK_TEMPLATE_NAME "TestLicenta"
#define BLYNK_AUTH_TOKEN "6RdHnB30w0kViYnNFmK2d7ZLWidR_YOs"
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char ssid[] = "Radu’s iPhone";
char pass[] = "parola123";

void setup() {

  Serial.begin(9600);
  delay(10);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();
  if (Serial.available()) {

    String line = Serial.readStringUntil('\n');
    float valuesFromSen[7]; 
    int start = 0;
    for (int i = 0; i < 7; i++) {
      int spaceIndex = line.indexOf(' ', start);
      if (spaceIndex == -1) spaceIndex = line.length(); // cand nu mai sunt spatii merge pana la sfrasit
      String subStr = line.substring(start, spaceIndex);
      valuesFromSen[i] = subStr.toFloat();
      start = spaceIndex + 1;
    }

    // trimite date pe pinii virtuali din blynk
    for (int i = 0; i < 7; i++) {
      Blynk.virtualWrite(V0 + i, valuesFromSen[i]);
    }

    //afisare pe seriala
    const char* sensorNames[] = {
      "Umiditate: ", "Temperatura: ", "CO2: ", "LPG: ", "SMOKE: ", "Altitudine: ", "Presiune: "
    };
    for (int i = 0; i < 7; i++) {
      Serial.print(sensorNames[i]);
      Serial.println(valuesFromSen[i]);
    }
    Serial.println(line); 
  }
}


