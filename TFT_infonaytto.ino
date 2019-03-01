/*
  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
*/

#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();  // Invoke library
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include "settings.h"
#include <Arduino.h>
#include <TM1637Display.h>

FASTLED_USING_NAMESPACE
#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    D6
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    4
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          32
#define FRAMES_PER_SECOND  120


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
uint8_t verstasStatus = 0;
uint8_t lastVerstasStatus;
uint8_t lunchTimer = 30;
uint8_t awayTimer = 5;

#define CLK D1
#define DIO D2
int sensorPin = A0;
int sensorValue = 0;
uint8_t button;
uint8_t oldButton = 0;
TM1637Display display(CLK, DIO);

void setup() {
  tft.init();
  tft.setRotation(3);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  display.setBrightness(0x0f);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}

void setup_wifi() {
  delay (10);
  Serial.println();
  Serial.print("Connecting ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP adress:");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {

  String message;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }
  Serial.println(message);

  if (message == "auki") {
    verstasStatus = 0;
  }
  if (message == "kiinni") {
    verstasStatus = 2;
  }
  if (message == "lounas") {
    verstasStatus = 1;
  }
  if (message == "hetki") {
    verstasStatus = 4;
  }
  if (message == "5min") {
    verstasStatus = 3;
  }
  if (message == "10min") {
    verstasStatus = 3;
    lastVerstasStatus = 3;
    awayTimer = 10;
    tft.fillScreen(TFT_BLACK);
  }
  if (message == "suljettu tilapäisesti") {
    verstasStatus = 5;
  }
}

static void mqtt_send(const char *topic, const char *message)
{
  // Make sure we have wifi and if not try to get some wifi. If we do not have saved wifi settings create accespoint with esp_id and wifi_pw ( at first run login to ap and save wifi settings ).
  if (!client.connected()) {
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.connect("TFTinfoDisplay", MQTT_USER, MQTT_PASSWORD);
  }
  if (client.connected()) {
    Serial.print("Publishing ");
    Serial.print(message);
    Serial.print(" to ");
    Serial.print(topic);
    Serial.print("...");
    int result = client.publish(topic, message, true);
    Serial.println(result ? "OK" : "FAIL");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe(MQTT_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

const uint8_t SEG_OPEN[] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,           // P
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_C | SEG_E | SEG_G,                           // n
};

const uint8_t SEG_CLOSED[] = {
  SEG_A | SEG_D | SEG_E | SEG_F,                   // C
  SEG_E | SEG_F | SEG_D,                           // L
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_A | SEG_C | SEG_D | SEG_G | SEG_F,           // S
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
};

const uint8_t SEG_LOUNAS[] = {
  SEG_E | SEG_F | SEG_D,                           // L
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,           // U
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_B | SEG_C | SEG_G | SEG_E | SEG_F,   // A
  SEG_A | SEG_C | SEG_D | SEG_G | SEG_F,           // S
};

const uint8_t SEG_IN[] = {
  SEG_B | SEG_C,                                   // I
  SEG_C | SEG_E | SEG_G,                           // n
};

const uint8_t SEG_IN99[] = {
  SEG_B | SEG_C,                                   // I
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_B | SEG_C | SEG_G | SEG_F,           // 9
  SEG_A | SEG_B | SEG_C | SEG_G | SEG_F,           // 9
};

const uint8_t SEG_SULJ[] = {


  SEG_A | SEG_C | SEG_D | SEG_G | SEG_F,           // S
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,           // U
  SEG_E | SEG_F | SEG_D,                           // L
  SEG_B | SEG_C | SEG_D | SEG_E,                   // J
};

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  display.setBrightness(0x0f);
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  sensorValue = analogRead(sensorPin);
  uint8_t buttonReading;

  if ((sensorValue > 500) and (sensorValue < 550)) buttonReading = 1;
  else if ((sensorValue > 315) and (sensorValue < 385)) buttonReading = 2;
  else if (sensorValue < 50) buttonReading = 3;
  else {
    buttonReading = 0;
  }

  if (buttonReading != oldButton) {
    button = buttonReading;
    oldButton = button;
  }
  else button = 0;

  if (button == 1 && verstasStatus != 0) verstasStatus = 0;
  else if (button == 1 && verstasStatus == 0) verstasStatus = 2;
  if (button == 2 && verstasStatus != 1) verstasStatus = 1;
  if (button == 3 && verstasStatus != 3) {
    verstasStatus = 3;
    button = 0;
  }

  tft.setCursor(20, 15);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2); tft.setTextSize(3);
  tft.println("Vekotinverstas");
  tft.println();

  if (verstasStatus == 0) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(80, 100);
    tft.setTextFont(2); tft.setTextSize(3);
    tft.println("Avoinna");
    display.setSegments(SEG_OPEN);
    fill_solid( leds, NUM_LEDS, CRGB(0, 255, 0));
  }

  if (verstasStatus == 1) {

    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(70, 85);
    tft.setTextFont(2); tft.setTextSize(3);
    tft.println("Lounaalla");
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2); tft.setTextFont(1);
    tft.setCursor(15, 200);
    tft.print("Palaamme"); tft.print(" ");
    tft.setTextColor(TFT_YELLOW);
    tft.print(lunchTimer); tft.print(" ");
    tft.setTextColor(TFT_RED);
    tft.print("min kuluttua");
    display.setSegments(SEG_LOUNAS);
    int pos = beatsin8(80, 8, 255, 0, 0);
    fill_solid( leds, NUM_LEDS, CRGB(pos, pos, 0));
    EVERY_N_SECONDS(66) {
      lunchTimer--;
      tft.fillScreen(TFT_BLACK);
    }
    if (lunchTimer < 1 ) verstasStatus = 4;
  }
  else lunchTimer = 30;

  if (verstasStatus == 2) {

    int pos2 = beatsin8(80, 1, 10, 0, 180);
    display.setBrightness(pos2);
    tft.setTextColor(TFT_RED);
    tft.setCursor(60, 85);
    tft.setTextFont(2); tft.setTextSize(4);
    tft.println("Suljettu");
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(20, 190);
    tft.setTextFont(2); tft.setTextSize(2);
    tft.println("Avoinna arkisin 9-16");
    display.setSegments(SEG_CLOSED);
    int pos = beatsin8(80, 8, 255, 0, 0);
    fill_solid( leds, NUM_LEDS, CRGB(pos, 0, 0));
  }

  if (verstasStatus == 3) {
    if (button == 3) {
      if (awayTimer < 5) {
        awayTimer = 5;
        button = 0;
        tft.fillScreen(TFT_BLACK);
      }
      else if ((awayTimer >= 5) && (awayTimer < 10)) {
        awayTimer = 10;
        button = 0;
        tft.fillScreen(TFT_BLACK);
      }
      else if ((awayTimer >= 10) && (awayTimer < 15)) {
        awayTimer = 15;
        button = 0;
        tft.fillScreen(TFT_BLACK);
      }
      else if ((awayTimer >= 15) && (awayTimer < 30)) {
        awayTimer = 30;
        button = 0;
        tft.fillScreen(TFT_BLACK);
      }
      else if ((awayTimer >= 30) && (awayTimer < 60)) {
        awayTimer = 60;
        button = 0;
        tft.fillScreen(TFT_BLACK);
      }
      else if (awayTimer >= 60) {
        verstasStatus = 5;
        awayTimer = 5;
        tft.fillScreen(TFT_BLACK);
      }
    }

    EVERY_N_SECONDS(60) {
      awayTimer--;
      tft.fillScreen(TFT_BLACK);
    }

    if (awayTimer == 0 ) {
      verstasStatus = 4;
      awayTimer = 5;
    }
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(55, 75);
    tft.setTextFont(2); tft.setTextSize(4);
    tft.println("Hetkinen");
    tft.setCursor(5, 170);
    tft.setTextSize(2);
    tft.print("Palaamme"); tft.print(" ");
    tft.setTextColor(TFT_RED);
    tft.print(awayTimer); tft.setTextSize(1); tft.print(" ");
    tft.setTextSize(2);
    tft.print("min"); tft.print(" "); tft.setTextColor(TFT_YELLOW);
    tft.println("kuluttua");
    display.setSegments(SEG_IN);
    display.showNumberDec(awayTimer, false);
    fadeToBlackBy( leds, NUM_LEDS, ( 5 + (300 / (awayTimer + 1))));
    int pos = beatsin16( 5 + (300 / (awayTimer + 1)), 0, NUM_LEDS - 1 );
    leds[pos] += CRGB( 255, 0, 0);
  }

  if (verstasStatus == 4) {

    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(60, 90);
    tft.setTextFont(2); tft.setTextSize(4);
    tft.println("Hetkinen");
    tft.setTextFont(2); tft.setTextSize(1);
    tft.setCursor(70, 180);
    tft.println("Kiirellisessa tapauksessa");
    tft.setCursor(52, 195);
    tft.println("soita numeroon: 0401842078");
    display.setSegments(SEG_IN99);
    int pos = beatsin8(80, 8, 255, 0, 0);
    fill_solid( leds, NUM_LEDS, CRGB(pos, 0, 0));
  }

  if (verstasStatus == 5) {

    tft.setTextColor(TFT_RED);
    tft.setCursor(60, 65);
    tft.setTextFont(2); tft.setTextSize(4);
    tft.println("Suljettu");
    tft.setCursor(25, 120);
    tft.println("tilapaisesti");
    tft.setTextSize(1);
    tft.setCursor(140, 125);
    tft.println(". .");
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(55, 200);
    tft.setTextFont(2); tft.setTextSize(1);
    tft.println("Normaalisti avoinna Ma-Pe 9-16");
    display.setSegments(SEG_SULJ);
    int pos = beatsin8(80, 8, 255, 0, 0);
    fill_solid( leds, NUM_LEDS, CRGB(pos, 0, pos));
  }

  if (verstasStatus != lastVerstasStatus) {
    tft.fillScreen(TFT_BLACK);
    awayTimer = 5;
  }
  lastVerstasStatus = verstasStatus;

}
