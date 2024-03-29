#include <WiFi.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <DS3231.h>
#include "esp32-hal-ledc.h"
 #include <AsyncTCP.h>
 
#include <ESPAsyncWebServer.h>


#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define SEALEVELPRESSURE_HPA (1013.25)
#define COUNT_LOW 0 // Minimum degree for the servo motor
#define COUNT_HIGH 4000 // Maximum degree for the servo motor
#define TIMER_WIDTH 16 // Servo motor 16-bit width
#define CHANNEL 1 // Communication channel for the servo motor
#define Hz 50 // 50 wave cycles per second for the servo motor
#define SERVO_PIN 25 // Servo motor is attached to pin 25
#define ONE_WIRE_BUS 4 // Data wire is plugged into digital pin 3 on the Arduino
#define LED_BUILTIN 2 // LED for testing is attached to pin 2
#define AI 34 // Analog input on port 34 for soil moisture sensor
#define SERIAL_9600 9600 // Serial port 9600
#define SERIAL_115200 115200 // Serial port 115200

Adafruit_BME280 bme280;
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire); // Pass oneWire reference to DallasTemperature library
DS3231 Clock;

//Servo
void valve_close(){
  Serial.println("Valve is closing");
  for (int i = COUNT_LOW; i < COUNT_HIGH; i += 100){
    ledcWrite(CHANNEL, i); // sweep open servo 1
    delay(50);
  }
}

void valve_open(){
  Serial.println("Valve is opening");
  for (int i = COUNT_HIGH; i > COUNT_LOW; i -= 100){
    ledcWrite(CHANNEL, i); // sweep close servo 1
    delay(50);
  }
}

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
int soil_sensor_value;
int soilPercentage;

void soil_sensor_read(int &soil_sensor_value, int &soilPercentage){
  soil_sensor_value = analogRead(AI);
//  Serial.println(soil_sensor_value);
  soil_sensor_value = constrain(soil_sensor_value, 1800, 4095);
  soilPercentage = map(soil_sensor_value, 1800, 4095, 100, 0);

  Serial.print("Soil humidity: ");
  Serial.print(soilPercentage);
  Serial.println("%");
}

//Plant
float avg_temperature;
byte Year_last;
byte Month_last;
byte Date_last;
byte DoW_last;
byte Hour_last;
byte Minute_last;
byte Second_last;

void plant_watering(float &avg_temperature, Adafruit_BME280 &bme280, float &pressure_bme280, float &temperature_bme280, float &altitude_bme280, float &humidity_bme280,
                    DallasTemperature &sensors, float &temperature_ds18b20, int &soil_sensor_value, int &soilPercentage,
                    DS3231 &Clock, byte& Year_last, byte& Month_last, byte& Date_last, byte& Hour_last, byte& Minute_last, byte& Second_last, bool &Century, bool &h12, bool &PM){
  
  bme280_read(bme280, pressure_bme280, temperature_bme280, altitude_bme280, humidity_bme280, "get");
  ds18b20_read(sensors, temperature_ds18b20, "get");
  soil_sensor_read(soil_sensor_value, soilPercentage);
  
  avg_temperature = (temperature_bme280 + temperature_ds18b20) / 2;
  Serial.print("Average temperature: ");
  Serial.print(avg_temperature);
  Serial.println(" C");
  
  if(avg_temperature >= 35.0){
    if(soilPercentage < 50){
      Serial.println("Watering the plant");
      valve_open();
       delay(10000);
      valve_close();
      ds3231_read(Clock, Year_last, Month_last, Date_last, Hour_last, Minute_last, Second_last, Century, h12, PM, "get");
      //send to internet function here
    }
  }
  else{
    if(soilPercentage < 30){
      Serial.println("Watering the plant");
      valve_open();
       delay(10000);
      valve_close();
      ds3231_read(Clock, Year_last, Month_last, Date_last, Hour_last, Minute_last, Second_last, Century, h12, PM, "get");
      //send to internet function here
    }
  }
}

//Web Server
const char* WIFI_NAME= "Xperia XZ";
const char* WIFI_PASSWORD = "7121941abc";
//WiFiServer server(8888);
//class ServerStack{
//  public:
//    WiFiServer server;
//    String header;
//    String valveState;
//  
//  ServerStack(WiFiServer outServer){
//    server = outServer;
//    valveState = "open";
//  }
//};
//ServerStack serverStack(server);
//
//void webServer(WiFiServer &server, String &header, String &valveState){
//  WiFiClient client = server.available();
//
//  if(client){ 
//    Serial.println("New Client is requesting web page"); 
//    String current_data_line = ""; 
//    while(client.connected()){
//      if (client.available()){ 
//        char new_byte = client.read(); 
//        Serial.write(new_byte);
//        header += new_byte;
//        if (new_byte == '\n'){
//          if (current_data_line.length() == 0){
//            client.println("HTTP/1.1 200 OK");
//            client.println("Content-type:text/html");
//            client.println("Connection: close");
//            client.println();
//            
//            if (header.indexOf("Valve=OPEN") != -1){
//              Serial.println("Valve is OPEN");
//              valveState = "open";
//              valve_open();
//            }
//            
//            if(header.indexOf("Valve=CLOSED") != -1){
//              Serial.println("Valve is CLOSED");
//              valveState = "closed";
//              valve_close();
//            }
//
//            // Web page heading
//            client.println("<!DOCTYPE html><html>");
//            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
//            client.println("<link rel=\"icon\" href=\"data:\">");
//            client.println("<style>html {font-family:Helvetica; display:inline-block; margin:0px auto; text-align:center;}");
//            client.println(".button {background-color:#4CAF50; border:2px solid #4CAF50; color:white; padding:10px 20px; text-align:center; text-decoration:none; display:inline-block; font-size:16px; margin:4px 2px; cursor:pointer;}");
//            // client.println("text-decoration:none; font-size:30px; margin:2px; cursor:pointer;}");
//            client.println("</style></head>");
//            
//            // Web page body
//            client.println("<body><center><h1>Ame Dougu Web Server</h1></center>");
//            // client.println("<center><h2>Press on button to turn on led and off button to turn off LED</h3></center>");
//            client.println("<form><center>");
//            client.println("<p>Valve is " + valveState + "</p>");
//            client.println("<center><button class=\"button\" name=\"Valve\" value=\"OPEN\" type=\"submit\">Valve OPEN</button>") ;
//            client.println("<button class=\"button\" name=\"Valve\" value=\"CLOSED\" type=\"submit\">Valve CLOSED</button></center>");
//            client.println("</center></form></body></html>");
//            client.println();
//            break;
//          }
//          else{
//              current_data_line = "";
//          }
//        }
//        else if (new_byte != '\r'){
//            current_data_line += new_byte; 
//        }
//      }
//    }
//    // Clear the header variable
//    header = "";
//    // Close the connection
//    client.stop();
//    Serial.println("Client disconnected.");
//    Serial.println("");
//  }
//}
//
////Multithread
//TaskHandle_t serverTask;
//
//void webServerTask(void *serverStack){
//  ServerStack localServerStack = *((ServerStack*)serverStack);
//  
//  while(1){
//    webServer(localServerStack.server, localServerStack.header, localServerStack.valveState);
//    Serial.println("Running on web server task");
//    delay(1000);
//  }
//}

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
    <head><meta name="viewport" content="width=device-width, initial-scale=1">
           <script src="https://code.highcharts.com/highcharts.js"></script>
            <link rel="icon" href="data:">
            <style>html {font-family:Helvetica; display:inline-block; margin:0px auto; text-align:center;}
            .button {background-color:#4CAF50; border:2px solid #4CAF50; color:white; padding:10px 20px; text-align:center; text-decoration:none; display:inline-block; font-size:16px; margin:4px 2px; cursor:pointer;}
            </style>
  </head>
            
           
    <body>
  
  <center><h1>Ame Dougu Web Server</h1></center>
            <form action="/get"><center>
            <center><button class="button" name="Valve" value="OPEN" type="submit">Valve OPEN</button>
            <button class="button" name="Valve" value="CLOSED" type="submit">Valve CLOSED</button></center>
            </center></form>
      <br>
      <br>
      <br>
            <div id='chart-temperature' class='container'></div>
            <div id='chart-humidity' class='container'></div>
            <div id='chart-pressure' class='container'></div>
            <div id='chart-soil' class='container'></div>
          </body>
            
          <script>
var chartT = new Highcharts.Chart({
  chart:{ renderTo : 'chart-temperature' },
  title: { text: 'BME280 Temperature' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#059e8a' }
  },
  xAxis: { type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Temperature (Celsius)' }
    //title: { text: 'Temperature (Fahrenheit)' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartT.series[0].data.length > 40) {
        chartT.series[0].addPoint([x, y], true, true, true);
      } else {
        chartT.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

var chartH = new Highcharts.Chart({
  chart:{ renderTo:'chart-humidity' },
  title: { text: 'BME280 Humidity' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Humidity (%)' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartH.series[0].data.length > 40) {
        chartH.series[0].addPoint([x, y], true, true, true);
      } else {
        chartH.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;

var chartP = new Highcharts.Chart({
  chart:{ renderTo:'chart-pressure' },
  title: { text: 'BME280 Pressure' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#18009c' }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Pressure (hPa)' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartP.series[0].data.length > 40) {
        chartP.series[0].addPoint([x, y], true, true, true);
      } else {
        chartP.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/pressure", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>
)rawliteral";



String readBME280Temperature() {
  // Read temperature as Celsius (the default)
  float t = bme280.readTemperature();
  // Convert temperature to Fahrenheit
  //t = 1.8 * t + 32;
  if (isnan(t)) {    
    Serial.println("Failed to read from BME280 sensor!");
    return "";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

String readBME280Humidity() {
  float h = bme280.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from BME280 sensor!");
    return "";
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

String readBME280Pressure() {
  float p = bme280.readPressure() / 100.0F;
  if (isnan(p)) {
    Serial.println("Failed to read from BME280 sensor!");
    return "";
  }
  else {
    Serial.println(p);
    return String(p);
  }
}


void setup(){
  Serial.begin(SERIAL_115200);
  Wire.begin();
  sensors.begin(); // Start up the the DS18B20
  ledcSetup(CHANNEL, Hz, TIMER_WIDTH); // channel 1, 50 Hz, 16-bit width
  ledcAttachPin(SERVO_PIN, CHANNEL);   // GPIO 25 assigned to channel 1
  pinMode(LED_BUILTIN, OUTPUT);
  
  if(!bme280.begin(0x76)){
    // Start up the the BME280
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while(1);
  }

  Serial.print("Connecting to ");
  Serial.println(WIFI_NAME);
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Trying to connect to Wifi Network");
  }
  
  Serial.println("Successfully connected to WiFi network");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
//  serverStack.server.begin();
  server.begin();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // String inputMessage;
    // String inputParam;
    // if (request->hasParam(webYear) && request->hasParam(webMonth) && request->hasParam(webDay)){
    //   scheduledYear = request->getParam(webYear)->value();
    //   scheduledMonth = request->getParam(webMonth)->value();
    //   scheduledDay = request->getParam(webDay)->value();
    // }
    // // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    // else if (request->hasParam(webYear)) {
    //   inputMessage = request->getParam(webYear)->value();
    //   inputParam = webYear;
    // }
    // // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    // else if (request->hasParam(webMonth)) {
    //   inputMessage = request->getParam(webMonth)->value();
    //   inputParam = webMonth;
    // }
    // // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    // else if (request->hasParam(webDay)) {
    //   inputMessage = request->getParam(webDay)->value();
    //   inputParam = webDay;
    // }
    if (request->hasParam("Valve")) {
      if (request->getParam("Valve")->value() == "OPEN"){
        valve_open();
        delay(1000);
      }
      else if (request->getParam("Valve")->value() == "CLOSED")
      {
        valve_close();
        delay(1000);
      }
    }
    request->send(200, "text/html", "<br><a href=\"/\">Return to Home Page</a>");
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readBME280Temperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readBME280Humidity().c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readBME280Pressure().c_str());
  });
  
//  xTaskCreatePinnedToCore(webServerTask, "serverTask", 4096, (void*)&serverStack, 1, &serverTask, ARDUINO_RUNNING_CORE);
}

void loop(){
  bme280_read(bme280, pressure_bme280, temperature_bme280, altitude_bme280, humidity_bme280);
  ds18b20_read(sensors, temperature_ds18b20);
  soil_sensor_read(soil_sensor_value, soilPercentage);
  plant_watering(avg_temperature, bme280, pressure_bme280, temperature_bme280, altitude_bme280, humidity_bme280,
                sensors, temperature_ds18b20, soil_sensor_value, soilPercentage,
                Clock, Year_last, Month_last, Date_last, Hour_last, Minute_last, Second_last, Century, h12, PM);
  
  char InChar;
  if(Serial.read()){
    InChar = Serial.read();
//    Serial.println(InChar);
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
      valve_open();
      delay(10000);
      valve_close();
    }
  }
  
  Serial.println();
  delay(5000);
}
