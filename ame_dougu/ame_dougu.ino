#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <DS3231.h>
//#include <Servo.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define ONE_WIRE_BUS 4 // Data wire is plugged into digital pin 3 on the Arduino
#define LED_BUILTIN 2 // LED for testing is attached to pin 2
#define SERIAL_9600 9600 // Serial port 9600
#define SERIAL_115200 115200 // Serial port 115200

Adafruit_BME280 bme280;
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire); // Pass oneWire reference to DallasTemperature library
DS3231 Clock;
//Servo myservo;  // create servo object to control a servo

//Servo
int servo_pin = 9;

//void valve_open(Servo &myservo){
//  for (int pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
//    // in steps of 1 degree
//    myservo.write(pos);              // tell servo to go to position in variable 'pos'
//    delay(15);                       // waits 15ms for the servo to reach the position
//  }
//}
//
//void valve_close(Servo &myservo){
//  for (int pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
//    myservo.write(pos);              // tell servo to go to position in variable 'pos'
//    delay(15);                       // waits 15ms for the servo to reach the position
//  }
//}

//Memory card

//DS3231
byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;

byte Year_read;
byte Month_read;
byte Date_read;
byte DoW_read;
byte Hour_read;
byte Minute_read;
byte Second_read;

bool Century = false;
bool h12;
bool PM;

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
    if (Serial.available()){
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

void ds3231_read(DS3231 &Clock, byte& Year_read, byte& Month_read, byte& Date_read, byte& Hour_read, byte& Minute_read, byte& Second_read, bool &Century, bool &h12, bool &PM, String mode = "read"){
  if(mode == "read"){
    // Give time at next five seconds
//    for (int i=0; i<5; i++){
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
    Serial.println("DS3231 has been read");
//    }
  }
  else if(mode == "get"){
    Year_read = Clock.getYear(), DEC;
    Month_read = Clock.getMonth(Century), DEC;
    Date_read = Clock.getDate(), DEC;
    Hour_read = Clock.getHour(h12, PM), DEC; //24-hr
    Minute_read = Clock.getMinute(), DEC;
    Second_read = Clock.getSecond(), DEC;
    Serial.println("DS3231 has been fetched");
  }
}

void ds3231_set(DS3231 &Clock, byte& Year, byte& Month, byte& Date, byte& DoW, byte& Hour, byte& Minute, byte& Second){
  // If something is coming in on the serial line, it's
  // a time correction so set the clock accordingly.
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
  Serial.println("DS3231 has been set");
    
  // Flash the LED to show that it has been received properly
//  pinMode(LEDpin, OUTPUT);
//  digitalWrite(LEDpin, HIGH);
//  delay(10);
//  digitalWrite(LEDpin, LOW);
}

//BME280
float pressure_bme280;
float temperature_bme280;
float altitude_bme280;
float humidity_bme280;

void bme280_read(Adafruit_BME280 &bme, float &pressure_bme280, float &temperature_bme280, float &altitude_bme280, float &humidity_bme280, String mode = "read"){
  if(mode == "read"){
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
  }
  else if(mode == "get"){
    temperature_bme280 = bme.readTemperature();
    pressure_bme280 = bme.readPressure() / 100.0F;
    altitude_bme280 = bme.readAltitude(SEALEVELPRESSURE_HPA);
    humidity_bme280 = bme.readHumidity();
    Serial.println("BME280 has been fetched");
  }
}

//DS18B20
float temperature_ds18b20;

void ds18b20_read(DallasTemperature &sensors, float &temperature_ds18b20, String mode = "read"){
  // Send the command to get temperatures
  sensors.requestTemperatures();

  if(mode == "read"){
    //print the temperature in Celsius
    Serial.print("Temperature: ");
    Serial.print(sensors.getTempCByIndex(0));
    Serial.print((char)176);//shows degrees character
    Serial.print("C  |  ");
    
    //print the temperature in Fahrenheit
    Serial.print((sensors.getTempCByIndex(0) * 9.0) / 5.0 + 32.0);
    Serial.print((char)176);//shows degrees character
    Serial.println("F");
  }
  else if(mode == "get"){
    temperature_ds18b20 = sensors.getTempCByIndex(0);
    Serial.println("DS18B20 has been fetched");
  }
}

//Soil moisture sensor

//Plant
float avg_temperature;
byte Year_last;
byte Month_last;
byte Date_last;
byte DoW_last;
byte Hour_last;
byte Minute_last;
byte Second_last;

void plant_watering(float &avg_temperature, Adafruit_BME280 &bme, float &pressure_bme280, float &temperature_bme280, float &altitude_bme280, float &humidity_bme280,
                    DallasTemperature &sensors, float &temperature_ds18b20,
                    DS3231 &Clock, byte& Year_last, byte& Month_last, byte& Date_last, byte& Hour_last, byte& Minute_last, byte& Second_last, bool &Century, bool &h12, bool &PM){
  
  bme280_read(bme280, pressure_bme280, temperature_bme280, altitude_bme280, humidity_bme280, "get");
  ds18b20_read(sensors, temperature_ds18b20, "get");
  //soil humidity function here
  
  avg_temperature = (temperature_bme280 + temperature_ds18b20) / 2;
  Serial.println(avg_temperature);
  
  if(avg_temperature >= 35.0){
    if("soil humidity" < 50){
      Serial.println("Watering the plant");
      ds3231_read(Clock, Year_last, Month_last, Date_last, Hour_last, Minute_last, Second_last, Century, h12, PM, "get");
      //send to internet function here
    }
  }
  else{
    if("soil humidity" < 30){
      Serial.println("Watering the plant");
      ds3231_read(Clock, Year_last, Month_last, Date_last, Hour_last, Minute_last, Second_last, Century, h12, PM, "get");
      //send to internet function here
    }
  }
}

void setup(){
  Serial.begin(SERIAL_115200);
  Wire.begin();
//  myservo.attach(servo_pin);  // attaches the servo on pin 9 to the servo object
  sensors.begin(); // Start up the the DS18B20
  pinMode(LED_BUILTIN, OUTPUT);
  
  if(!bme280.begin(0x76)){
    // Start up the the BME280
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while(1);
  }
}

void loop(){
  bme280_read(bme280, pressure_bme280, temperature_bme280, altitude_bme280, humidity_bme280);
  ds18b20_read(sensors, temperature_ds18b20);
  //plant watering function here
  
  char InChar;
  if(Serial.read()){
    InChar = Serial.read();
    Serial.println(InChar);
    if(InChar == '1'){
      Serial.println("Set DS3231");
      ds3231_set(Clock, Year, Month, Date, DoW, Hour, Minute, Second);
    }
    else if(InChar == '2'){
      Serial.println("Read DS3231");
      ds3231_read(Clock, Year_read, Month_read, Date_read, Hour_read, Minute_read, Second_read, Century, h12, PM);
    }
    else if(InChar == '3'){
      Serial.println("Fetch DS3231");
      ds3231_read(Clock, Year_read, Month_read, Date_read, Hour_read, Minute_read, Second_read, Century, h12, PM, "get");
    }
    else if(InChar == '4'){
      Serial.println("Fetch BME280");
      bme280_read(bme280, pressure_bme280, temperature_bme280, altitude_bme280, humidity_bme280, "get");
    }
    else if(InChar == '5'){
      Serial.println("Fetch DS18B20");
      ds18b20_read(sensors, temperature_ds18b20, "get");
    }
    else if(InChar == '6'){
      Serial.println("Activate Servo");
//      valve_open(myservo);
//      delay(10000)
//      valve_close(myservo);
    }
  }
  
//  digitalWrite(LED_BUILTIN, HIGH);
  delay(10000);
//  digitalWrite(LED_BUILTIN, LOW);
}
