#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <DS3231.h>
#include <Servo.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define ONE_WIRE_BUS 2 // Data wire is plugged into digital pin 2 on the Arduino

Adafruit_BME280 bme280;
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire); // Pass oneWire reference to DallasTemperature library
DS3231 Clock;
Servo myservo;  // create servo object to control a servo

//DS3231
byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;
bool Century = false;
bool h12;
bool PM;

void setup(){
  Serial.begin();
  sensors.begin();  // Start up the library
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  if (!bme280.begin(0x76)){
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while(1);
  }
}

void loop(){
  bme280_read(bme280);
  ds18b20_read(sensors);
  ds3231_set(Clock, Year, Month, Day, DoW, Hour, Minute, Second);
  ds3231_read(Clock, Century, h12, PM);
  valve_open(myservo);
  valve_close(myservo);
}

void valve_open(Servo &myservo){
  for (int pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}

void valve_close(Servo &myservo){
  for (int pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}

void bme280_read(Adafruit_BME280 &bme){
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println("*C");

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println("hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println("m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println("%");

  Serial.println();
  delay(1000);
}

void ds18b20_read(DallasTemperature &sensors){
  // Send the command to get temperatures
  sensors.requestTemperatures(); 

  //print the temperature in Celsius
  Serial.print("Temperature: ");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.print((char)176);//shows degrees character
  Serial.print("C  |  ");
  
  //print the temperature in Fahrenheit
  Serial.print((sensors.getTempCByIndex(0) * 9.0) / 5.0 + 32.0);
  Serial.print((char)176);//shows degrees character
  Serial.println("F");
  
  delay(500);
}

//DS3231
void GetDateStuff(byte& Year, byte& Month, byte& Day, byte& DoW, byte& Hour, byte& Minute, byte& Second){
  // Call this if you notice something coming in on 
  // the serial port. The stuff coming in should be in 
  // the order YYMMDDwHHMMSS, with an 'x' at the end.
  boolean GotString = false;
  char InChar;
  byte Temp1, Temp2;
  char InString[20];

  byte j=0;
  while (!GotString) {
    if (Serial.available()) {
      InChar = Serial.read();
      InString[j] = InChar;
      j += 1;
      if (InChar == 'x') {
        GotString = true;
      }
    }
  }
  Serial.println(InString);
  // Read Year first
  Temp1 = (byte)InString[0] -48;
  Temp2 = (byte)InString[1] -48;
  Year = Temp1*10 + Temp2;
  // now month
  Temp1 = (byte)InString[2] -48;
  Temp2 = (byte)InString[3] -48;
  Month = Temp1*10 + Temp2;
  // now date
  Temp1 = (byte)InString[4] -48;
  Temp2 = (byte)InString[5] -48;
  Day = Temp1*10 + Temp2;
  // now Day of Week
  DoW = (byte)InString[6] - 48;   
  // now Hour
  Temp1 = (byte)InString[7] -48;
  Temp2 = (byte)InString[8] -48;
  Hour = Temp1*10 + Temp2;
  // now Minute
  Temp1 = (byte)InString[9] -48;
  Temp2 = (byte)InString[10] -48;
  Minute = Temp1*10 + Temp2;
  // now Second
  Temp1 = (byte)InString[11] -48;
  Temp2 = (byte)InString[12] -48;
  Second = Temp1*10 + Temp2;
}

void ds3231_read(DS3231 &Clock, bool &Century, bool &h12, bool &PM){
  if (Serial.available()){
    // Give time at next five seconds
    for (int i=0; i<5; i++){
        delay(1000);
        Serial.print(Clock.getYear(), DEC);
        Serial.print("-");
        Serial.print(Clock.getMonth(Century), DEC);
        Serial.print("-");
        Serial.print(Clock.getDate(), DEC);
        Serial.print(" ");
        Serial.print(Clock.getHour(h12, PM), DEC); //24-hr
        Serial.print(":");
        Serial.print(Clock.getMinute(), DEC);
        Serial.print(":");
        Serial.println(Clock.getSecond(), DEC);
    }
  }
  delay(1000);
}

void ds3231_set(DS3231 &Clock, byte& Year, byte& Month, byte& Day, byte& DoW, byte& Hour, byte& Minute, byte& Second){
  // If something is coming in on the serial line, it's
  // a time correction so set the clock accordingly.
  
  if(Serial.available()){
    GetDateStuff(Year, Month, Date, DoW, Hour, Minute, Second);
    
    Clock.setClockMode(false);  // set to 24h
    //setClockMode(true); // set to 12h
    
    Clock.setSecond(Second);
    Clock.setMinute(Minute);
    Clock.setHour(Hour);
    Clock.setDate(Date);
    Clock.setMonth(Month);
    Clock.setYear(Year);
    Clock.setDoW(DoW);
    
    // Flash the LED to show that it has been received properly
//    pinMode(LEDpin, OUTPUT);
//    digitalWrite(LEDpin, HIGH);
//    delay(10);
//    digitalWrite(LEDpin, LOW);
  }
  delay(1000);
}
