#include <DHT.h>
#include <MQ2.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Keypad.h>

#define DHTTYPE DHT11
#define DHTPIN 5
#define MQ2PIN A0
#define PIRPIN 4
#define SERVOPIN 9
#define RELAYPINBEC 11
#define RELAYPINFAN 7
#define LEDPIRPIN 26
#define ECHOPIN 13
#define TRIGPIN 12
#define LEDLUMINAPIN 24
#define SLUMINAPIN 8
#define BUZZERPIN 29

DHT dht(DHTPIN, DHTTYPE);
MQ2 mq2(MQ2PIN);
Servo myServo;
LiquidCrystal_I2C lcd(0x27,16,2); // adresa lcd 0x27
Adafruit_BMP280 bmp;

const byte ROWS = 4; // 4 randuri pt tastatura
const byte COLS = 4; // 4 coloane pt tastatura

char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {53, 51, 49, 47}; // pini randuri
byte colPins[COLS] = {45, 43, 41, 39}; // pini coloane
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

unsigned long lastSerialBt = 0;     // timp ultima trimitere pe seriala la bt
unsigned long lastDisplayLCD = 0;   // timp ultimul switch pe lcd
unsigned long lastServoMove = 0;    // timp ultima miscare la servo
unsigned long buzzerStart = 0;      // timp ultima activare buzzer
const long intervalSerialBt = 500;  // interval trimitere seriala bt 0.5 secunde
const long intervalLCD = 3000;      // interval update lcd 3 secunde
const long intervalBuzzer = 3000;   // interval buzzer activ 3 secunde
const long intervalUltra = 10000;   // interval usa deschisa 10 secunde

int displayState = 0;               // starea lcd 1 - dht, 2 - mq2, 3 - bmp
String inputTemp = "";
long unsigned int temperaturaSetata;
float duration; 
float distance;
bool buzzer_on_off = false;

void setup() {
  dht.begin();
  mq2.begin();
  myServo.attach(SERVOPIN);
  pinMode(PIRPIN, INPUT);
  pinMode(LEDPIRPIN, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(SLUMINAPIN, INPUT);
  pinMode(LEDLUMINAPIN, OUTPUT);
  pinMode(RELAYPINBEC, OUTPUT);
  pinMode(RELAYPINFAN, OUTPUT);
  myServo.write(0);
  lcd.init();
  lcd.clear();
  lcd.backlight();
  bmp.begin(0x76) ; //adresa bmp 0x76
  

  Serial.begin(9600);   //serial bt
  Serial3.begin(9600);  //serial nodemcu

  lcd.setCursor(0, 1);
  lcd.print("Acum:");
  lcd.print(dht.readTemperature());
  lcd.setCursor(6, 1);
  Serial.print("Temperatura curenta este ");
  Serial.println(dht.readTemperature());
  bool completed = false;
  while (!completed) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      if (key == '#') {
        temperaturaSetata = inputTemp.toFloat(); 
        Serial.println("Temperatura setata:");
        Serial.println(temperaturaSetata);
        completed = true; 
      } else if (isdigit(key)) {
        inputTemp += key;
        Serial.print("Continuati introducerea temperaturii sau apasati tasta # pentru oprire: ");
        Serial.println(inputTemp); 
        lcd.setCursor(0, 0);
        lcd.print("Setati la: ");
        lcd.print(inputTemp);
      }
    }
    delay(100); 
  }
}

void loop() {
//***************MQ2/Buzzer****************************
checkForGas();
//***************Senz Lumina****************************
checkForLight();
//***************Servo si ultrasonic****************************
checkForUltra();
//***************Releu fan si far****************************
controlTemperature();
//***************PIR*******************************
checkForPIR();
//***************LCD***************
displayOnLCD();
//***************TRANSMISIE BT****************************
sendDataToBT();
//***************TRANSMISIE NODEMCU***************
sendDataToESP();
}

String readDHTSensor(){
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)){
    return "Nu se poate citi DHT11";
  }

  return "Temperature: " + String(temperature) + "\nHumidity: " + String(humidity); 
}

String readMQ2Sensor(){

  float co = mq2.readCO()*(5/1023.0);
  float lpg = mq2.readLPG()*(5/1023.0);
  float smoke = mq2.readSmoke()*(5/1023.0);

  if(isnan(co) || isnan(lpg) || isnan(smoke)){
    return "Nu se poate citi MQ2";
  }

  return "CO2: " + String(co) + "\nLPG: " + String(lpg) + "\nSMOKE: " + String(smoke); 
}

void displayDHTReadings(){
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print("%");
  
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print((char)223); // simbol grad Celsius
  lcd.print("C");
}

void displayMQ2Readings(){
  float co = mq2.readCO();      
  float lpg = mq2.readLPG();    
  float smoke = mq2.readSmoke();
  
  lcd.setCursor(0, 0);
  lcd.print("CO:");
  lcd.print(co);
  lcd.print(" LPG:");
  lcd.print(lpg);
  
  
  lcd.setCursor(0, 1);
  lcd.print("Smoke: ");
  lcd.print(smoke);
}

void displayBMPReadings(){
  float pressure = bmp.readPressure() * 0.000145038; //transformare Pa-> psi
  float altitude = bmp.readAltitude(1020); 

  lcd.setCursor(0, 0);
  lcd.print("Pres: ");
  lcd.print(pressure);
  lcd.print(" PSI");

  lcd.setCursor(0, 1);
  lcd.print("Alt: ");
  lcd.print(altitude);
  lcd.print(" m");
}

String readBMPSensor(){
  return "Altitdue: " + String(bmp.readAltitude(1020)) + "\nPressure: " + String(bmp.readPressure() * 0.000145038);
}

void checkForGas(){
  if(mq2.readCO() || mq2.readLPG() || mq2.readSmoke() > 100){
    buzzerStart = millis();
    tone(BUZZERPIN, 1000); 
    buzzer_on_off = true;
  }
  else
    if(buzzer_on_off &&(millis() - buzzerStart >= intervalBuzzer)){
      noTone(BUZZERPIN);
      buzzer_on_off = false;
  }
}

void checkForLight(){
  if(digitalRead(SLUMINAPIN) == 0){
    digitalWrite(LEDLUMINAPIN,LOW);
  }
  else{
    digitalWrite(LEDLUMINAPIN,HIGH);
  }
}

void checkForUltra(){
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);

  duration = pulseIn(ECHOPIN, HIGH);
  distance = (duration * 0.0343) / 2; // 0.0343 cm/micros (343 m/s) = viteza sunetului in aer
  
  if (distance < 10 && millis() - lastServoMove >= intervalUltra){ 
    myServo.write(90);
    lastServoMove = millis(); 
  }

  if (millis() - lastServoMove < intervalUltra){
    myServo.write(90); 
  }
  else{
    myServo.write(0);
  }
}

void controlTemperature(){
  if(dht.readTemperature() >= temperaturaSetata){
  digitalWrite(RELAYPINFAN, HIGH);
  digitalWrite(RELAYPINBEC, LOW);
  }
  else{
    digitalWrite(RELAYPINFAN, LOW);
    digitalWrite(RELAYPINBEC, HIGH);
  }
}

void sendDataToBT(){
  if (millis() - lastSerialBt >= intervalSerialBt){
    lastSerialBt = millis();
    String mesaj = readDHTSensor() + "\n" + readMQ2Sensor() + "\n" + readBMPSensor() + "$";// transformam datele de la senori intru-un singur string
    Serial.println(mesaj); //trimitem pe seriala la bt
  }
}

void checkForPIR(){
  if(digitalRead(PIRPIN)==1){
    digitalWrite(LEDPIRPIN,HIGH);
  }
  else{
    digitalWrite(LEDPIRPIN,LOW);
  }
}

void displayOnLCD(){
  if (millis() - lastDisplayLCD >= intervalLCD){
    lastDisplayLCD = millis(); 
    lcd.clear(); 

    if (displayState == 0){
      displayDHTReadings();
    }else if (displayState == 1){
      displayMQ2Readings();
    }else if (displayState == 2){
      displayBMPReadings();
    }

    displayState = (displayState + 1) % 3; //ca sa trecem circular prin cele trei stari pt lcd
  }
}

void sendDataToESP(){
  String date_esp= String(dht.readHumidity())+" "+String(dht.readTemperature())+" "+String(mq2.readCO()*(5/1023.0))+" "+String(mq2.readLPG()*(5/1023.0))+" "+String(mq2.readSmoke()*(5/1023.0))+" "+String(bmp.readAltitude(1020))+" "+String(bmp.readPressure()* 0.000145038); // string sa trimitem catre nodemcu
  Serial3.println(date_esp);
}
