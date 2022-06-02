#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoMqttClient.h>

/**
 * Sleep Configuration
 */
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */


WiFiServer server(80);

/**
 * MQTT Config
 */
// MQTT Configuration
const char broker[] = "10.0.1.5";  
int        port     = 1883; 
const char topic[]  = "temp/plant";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

/**
 * Sensor Config
 */
// pin where sensor is connected
// const int DATA_PIN = A0; // ESP8266
const int DATA_PIN = 34;

// change for each unique sensorNumber
//int sensorNumber = 1; // bedroom workbench
int sensorNumber = 2; // living room, battery test
int soil_moisture_value = 0;
int soil_moisure_percent = 0;

// calibration values
// in order to get these values
// place sensor in open air = reading will be Value_1
// place sensor in water = reading will be Value_2
const int air_value = 3400;   //you need to replace this value with Value_1
const int water_value = 1600;   //you need to replace this value with Value_2

/**
 * Display Config
 */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void sendMessageToMqtt(){
  // send message to mqtt  
  Serial.println(F("Sending message to MQTT"));
  mqttClient.beginMessage(topic);
  mqttClient.print(soil_moisure_percent); 
  mqttClient.endMessage();
}


void displayVals(){
// Clear the buffer.
  Serial.println("In displayIP.");
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.print(F("IP: "));
  display.println(WiFi.localIP());

  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setCursor(0,20);             // Start at top-left corner
  display.print(soil_moisure_percent);
  display.println(" %");
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor

  pinMode(DATA_PIN,INPUT);
  
  Serial.println("Setting WIFI STA");
  WiFi.mode(WIFI_STA);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  
  server.begin();
  
 
  // Start I2C Communication SDA = 5 and SCL = 4 on Wemos Lolin32 ESP32 with built-in SSD1306 OLED
  Wire.begin(5, 4);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000); // Pause for 2 seconds
 
  // Clear the buffer.
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Welcome!! Starting things up...."));

//  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
//  display.println(3.141592);
//
//  display.setTextSize(2);             // Draw 2X-scale text
//  display.setTextColor(SSD1306_WHITE);
//  display.print(F("0x")); 
//  display.println(0xDEADBEEF, HEX);

  display.display();

  //displayVals();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);  
  if (!mqttClient.connect(broker, port)) {  
    Serial.print("MQTT connection failed! Error code = ");  
    Serial.println(mqttClient.connectError());  
    while (1);  
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
  
  //sendMessageToMqtt();

}

float movingAverage(float value) {
  const byte nvalues = 8;             // Moving average window size

  static byte current = 0;            // Index for current value
  static byte cvalues = 0;            // Count of values read (<= nvalues)
  static float sum = 0;               // Rolling sum
  static float values[nvalues];

  sum += value;

  // If the window is full, adjust the sum by deleting the oldest value
  if (cvalues == nvalues)
    sum -= values[current];

  values[current] = value;          // Replace the oldest with the latest

  if (++current >= nvalues)
    current = 0;

  if (cvalues < nvalues)
    cvalues += 1;

  return sum/cvalues;
}

void loop() {
  Serial.println("Main Loop.");
    // call poll() regularly to allow the library to send MQTT keep alive which
  // avoids being disconnected by the broker
  mqttClient.poll();

  int x = movingAverage(analogRead(DATA_PIN));
  Serial.print("Moving Average: ");
  Serial.println(x); 

  soil_moisture_value=0;
  soil_moisture_value = analogRead(DATA_PIN);
  soil_moisure_percent = map(soil_moisture_value, air_value, water_value, 0, 100);

  Serial.print("Soild Moisture Sensor Value: ");
  Serial.print(soil_moisture_value); // read the value from the sensor
  Serial.print(", Soild Moisture Percentage: ");
  Serial.println(soil_moisure_percent); 
  
  displayVals();

  sendMessageToMqtt();
  
  delay(TIME_TO_SLEEP * 1000);

}