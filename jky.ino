#include "Arduino.h"
#include <ESP8266WiFi.h>

#define NUM_MAX  4
#define DIN_PIN 13  // D7
#define CLK_PIN 14  // D5
#define CS_PIN   0  // D3

#define HOSTNAME "ESP-Clock"
const char* ssid     = "MobileX";
const char* password = "jiangkaiyue";

//===FORNT==================DigNum:
const uint8_t dig3x5[] PROGMEM = { 4,
                                   0x03, 0xF8, 0x88, 0xF8,
                                   0x02, 0x10, 0xF8, 0x00,
                                   0x03, 0xE8, 0xA8, 0xB8,
                                   0x03, 0x88, 0xA8, 0xF8,
                                   0x03, 0x38, 0x20, 0xF8,
                                   0x03, 0xB8, 0xA8, 0xE8,
                                   0x03, 0xF8, 0xA8, 0xE8,
                                   0x03, 0x08, 0x08, 0xF8,
                                   0x03, 0xF8, 0xA8, 0xF8,
                                   0x03, 0xB8, 0xA8, 0xF8,
                                 };
const uint8_t dig3x6[] PROGMEM = { 4,
                                   0x03, 0xFC, 0x84, 0xFC,
                                   0x03, 0x10, 0x08, 0xFC,
                                   0x03, 0xF4, 0x94, 0x9C,
                                   0x03, 0x84, 0x94, 0xFC,
                                   0x03, 0x3C, 0x20, 0xF8,
                                   0x03, 0x9C, 0x94, 0xF4,
                                   0x03, 0xFC, 0x94, 0xF4,
                                   0x03, 0x04, 0xE4, 0x1C,
                                   0x03, 0xFC, 0x94, 0xFC,
                                   0x03, 0xBC, 0xA4, 0xFC,
                                 };
const uint8_t dig3x7[] PROGMEM = { 4,
                                   0x03, 0xFE, 0x82, 0xFE,
                                   0x03, 0x08, 0x04, 0xFE,
                                   0x03, 0xF2, 0x92, 0x9E,
                                   0x03, 0x82, 0x92, 0xFE,
                                   0x03, 0x3E, 0x20, 0xFC,
                                   0x03, 0x9E, 0x92, 0xF2,
                                   0x03, 0xFE, 0x92, 0xF2,
                                   0x03, 0x02, 0xE2, 0x1E,
                                   0x03, 0xFE, 0x92, 0xFE,
                                   0x03, 0x9E, 0x92, 0xFE,
                                 };

//===MAX7219==================commands=========================
#define CMD_NOOP   0
#define CMD_DIGIT0 1
#define CMD_DECODEMODE  9
#define CMD_INTENSITY   10
#define CMD_SCANLIMIT   11
#define CMD_SHUTDOWN    12
#define CMD_DISPLAYTEST 15
byte scr[NUM_MAX * 8 + 8];

//===MAX7219==================cmdMAX7219=======================
void cmdMAX7219(byte cmd, byte data)
{
  digitalWrite(CS_PIN, LOW);
  for (int i = NUM_MAX - 1; i >= 0; i--) {
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, cmd);
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, data);
  }
  digitalWrite(CS_PIN, HIGH);
}

//===MAX7219==================refMAX7219=======================
void refMAX7219() {
  byte mask = 0x80;
  for (int c = 0; c < 8; c++) {
    digitalWrite(CS_PIN, LOW);
    for (int i = NUM_MAX - 1; i >= 0; i--) {
      byte bt = 0;
      for (int b = 0; b < 8; b++) {
        bt >>= 1;
        if (scr[i * 8 + b] & mask) bt |= 0x80;
      }
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, CMD_DIGIT0 + c);
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, bt);
    }
    digitalWrite(CS_PIN, HIGH);
    mask >>= 1;
  }
}

//===MAX7219==================clrMAX7219=======================
void clrMAX7219()
{
  for (int i = 0; i < NUM_MAX * 8; i++) scr[i] = 0;
}

//===MAX7219==================initMAX7219=======================
void initMAX7219()
{
  pinMode(DIN_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  cmdMAX7219(CMD_DISPLAYTEST, 0);
  cmdMAX7219(CMD_SCANLIMIT, 7);
  cmdMAX7219(CMD_DECODEMODE, 0);
  cmdMAX7219(CMD_INTENSITY, 0);
  cmdMAX7219(CMD_SHUTDOWN, 0);
  clrMAX7219();
  refMAX7219();
  cmdMAX7219(CMD_SHUTDOWN, 1);
  cmdMAX7219(CMD_INTENSITY, 2);
}

//=============================================
//=============================================
//=============================================
void setup()
{
  Serial.begin(115200);
  initMAX7219();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("MyIP: ");
  Serial.println(WiFi.localIP());
  delay(1500);
}

// =============================DEFINE VARS==============================
// =============================DEFINE VARS==============================
#define MAX_DIGITS 10
byte dig[MAX_DIGITS] = {0};
byte digold[MAX_DIGITS] = {0};
byte digtrans[MAX_DIGITS] = {0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
byte del = 0;
int h, m, s;
long localEpoc = 0;
long localMillisAtUpdate = 0;
// =======================================================================
void loop()
{
  if (updCnt <= 0) { // every 10 scrolls, ~450s=7.5m
    updCnt = 60;
    Serial.println("Getting data ...");
    getTime();
    Serial.println("Data loaded");
    clkTime = millis();
  }
  if (millis() - clkTime > 60000 && !del && dots) {
    updCnt--;
    clkTime = millis();
  }
  if (millis() - dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  }
  updateTime();
  showAnimClock();
}

// =======================================================================
void showAnimClock()
{
  byte digPos[6] = {2, 6, 12, 16, 22, 26};
  int digHt = 12;
  int num = 6;
  int i;
  if (del == 0) {
    del = digHt;
    for (i = 0; i < num; i++) digold[i] = dig[i];
    dig[0] = h / 10 ? h / 10 : 10;
    dig[1] = h % 10;
    dig[2] = m / 10;
    dig[3] = m % 10;
    dig[4] = s / 10;
    dig[5] = s % 10;
    for (i = 0; i < num; i++)  digtrans[i] = (dig[i] == digold[i]) ? 0 : digHt;
  } else
    del--;

  clrMAX7219();
  for (i = 0; i < num; i++) {
    if (digtrans[i] == 0) {
      showDigit(0, dig[i], digPos[i], dig3x7);
    } else {
      showDigit(digHt - digtrans[i], digold[i], digPos[i], dig3x7);
      showDigit(-digtrans[i], dig[i], digPos[i], dig3x7);
      digtrans[i]--;
    }
  }
  setCol(10, dots ? B00100100 : 0);
  setCol(20, dots ? B00100100 : 0);
  refMAX7219();
  delay(30);
}

// =======================================================================
void showDigit(int dy, char ch, int col, const uint8_t *data)
{
  if (dy < -8 | dy > 8) return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  for (int i = 0; i < w; i++)
    if (col + i >= 0 && col + i < 8 * NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if (!dy) scr[col + i] = v; else scr[col + i] |= dy > 0 ? v >> dy : v << -dy;
    }
}

// =======================================================================
void setCol(int col, byte v)
{
  if (col >= 0 && col < 8 * NUM_MAX)
    scr[col] = v;
}

// =======================================================================
void getTime()
{
  WiFiClient client;
  if (!client.connect("www.baidu.com", 80)) return;
  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.baidu.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500); repeatCounter++;
  }

  client.setNoDelay(false);
  while (client.connected() && client.available()) {
    String line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      Serial.println(line);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }
  client.stop();
}

// =======================================================================
void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round( curEpoch - 3600 * 16 + 86400L ) % 86400L;
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

