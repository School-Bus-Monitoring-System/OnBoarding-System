// Libraries required for wifi connection
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"

// Libraries required for RFID Reader
#include <SPI.h>
#include <MFRC522.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     8 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;  // your network SSID (name)
char pass[] = SECRET_PASS;  // your network password (use for WPA, or use as key for WEP)

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "broker.hivemq.com";
int port = 1883;
const char topic[] = "chitkara-university/school-bus-monitoring-system/students";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;

#define RST_PIN 9    // Configurable, see typical pin layout above
#define SS_1_PIN 10  // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 2
#define BUZZER 2
#define LED 4

MFRC522 mfrc522;


const int NUMS_STUDENTS_ON_BUS = 2;

String studentsonBus[NUMS_STUDENTS_ON_BUS] = { "empty", "empty" };


#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);

    display.clearDisplay();

  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Connecting To"));
  display.println(F("Internet"));
  display.display();      // Show initial text
  delay(100);

  // attempt to connect to WiFi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    display.print(F("."));
  display.display();      // Show initial text
    delay(5000);
  }
display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Connected!"));
  display.display();      // Show initial text
  delay(100);
  Serial.println("You're connected to the network");
  Serial.println();

  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId("clientId");

  // You can provide a username and password for authentication
  // mqttClient.setUsernamePassword("username", "password");
display.clearDisplay();

  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Connecting To"));
  display.println(F("Broker"));
  display.display();      // Show initial text
  delay(100);
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1)
      ;
  }

display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Connected!"));
  display.display();      // Show initial text
  delay(1000);
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  SPI.begin();  // Init SPI bus

  mfrc522.PCD_Init(SS_1_PIN, RST_PIN);  // Init each MFRC522 card
  Serial.print(F("Reader "));
  Serial.print(0);
  Serial.print(F(": "));
  mfrc522.PCD_DumpVersionToSerial();
}

void loop() {
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  mqttClient.poll();
 display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("Tap Card!"));
  display.display();      // Show initial text

  mqttClient.beginMessage(topic);
    for(int i = 0; i < NUMS_STUDENTS_ON_BUS; i++){
      mqttClient.print(studentsonBus[i]);
      if (i < NUMS_STUDENTS_ON_BUS - 1)
      mqttClient.print(",");
    }
    mqttClient.endMessage();
    delay(100);

  // to avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
  // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
  unsigned long currentMillis = millis();

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print(F("Reader "));
    Serial.print(0);
    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F(": Card UID:"));
    char str[32] = "";
    array_to_string(mfrc522.uid.uidByte, mfrc522.uid.size, str);

    // dump_byte_array(mfrc522[0].uid.uidByte, mfrc522[0].uid.size);
    Serial.println(str);
    beep_and_blink();
    Serial.println("Message Sent!");

    // Update the studentsOnBus array.
    updateStudentsOnBus(str);

    printStudentsOnBus();

    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
  }  //if (mfrc522[reader].PICC_IsNewC
}

void array_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
    buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
  }
  buffer[len * 2] = '\0';
}

void beep_and_blink() {
  digitalWrite(LED, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
  digitalWrite(BUZZER, LOW);
}


void updateStudentsOnBus(String uidString) {
     // Check if student is on bus or not:
  for (int i = 0; i < NUMS_STUDENTS_ON_BUS; i++) {
    if(studentsonBus[i] == uidString) {
      studentsonBus[i] = "empty";
       display.clearDisplay();
      display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("Leaving!"));
  
  display.display();      // Show initial text
  delay(2000);
      return;
    }
  }
  for (int i = 0; i < NUMS_STUDENTS_ON_BUS; i++){
    if(studentsonBus[i] != uidString) {
      if(studentsonBus[i] == "empty") {
        studentsonBus[i] = uidString;
         display.clearDisplay();
        display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Onboarding!"));
 
  display.display();      // Show initial text
   delay(2000);
        return;
      }
    }
  }
}

void printStudentsOnBus(){
  for (int i = 0; i < NUMS_STUDENTS_ON_BUS; i++)
  {
    Serial.println(studentsonBus[i]);
  }
}
