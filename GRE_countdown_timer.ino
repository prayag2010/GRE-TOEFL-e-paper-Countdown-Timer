//http://www.timestampconvert.com/
//IT TAKES IST OFFSET INTO ACCOUNT!!

#include <ESP8266WiFi.h>

#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <Ticker.h>


// include library, include base class, make path known
#include <GxEPD.h>

// select the display class to use, only one
#include <GxGDEP015OC1/GxGDEP015OC1.cpp>    // 1.54" b/w

#include GxEPD_BitmapExamples

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/Open_Sans_Regular_15.h>
#include <Fonts/Dialog_bold_16.h>
#include <Fonts/Dialog_bold_20.h>
#include <Fonts/Century_Schoolbook_L_Bold_30.h>


#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>


//ESP8266
GxIO_Class io(SPI, SS, 0, 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io); // default selection of D4(=2), D2(=4)

const char *ssid     = "R7";
const char *password = "Internet701";

WiFiUDP udp;

EasyNTPClient ntpClient(udp, "time.google.com", ((5 * 60 * 60)+(30 * 60)));

time_t currentTimestamp;

Ticker timekeepingTicker;
Ticker syncNtpTicker;
Ticker displayUpdateTicker;

time_t GREtimestamp = 1537209001 + ((5 * 60 * 60) + (30 * 60));
time_t GREcompletedStamp = 1537286400 + ((5 * 60 * 60) + (30 * 60));

time_t TOEFLtimestamp = 1538159401 + ((5 * 60 * 60) + (30 * 60));
time_t TOEFLcompletedStamp = 1538193600 + ((5 * 60 * 60) + (30 * 60));

int curGreDays;
int prevGreDays;
int hoursUntilGre;
int minUntilGre;
int prevMin;

int curToeflDays;
int prevToeflDays;
int hoursUntilToefl;
int minUntilToefl;
int prevMinToefl;

boolean greDay = false;
boolean greCompleted = false;

boolean toeflDay = false;
boolean toeflCompleted = false;

boolean initial = false;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");

  display.init();
  wifiReconnect();

  Serial.println("NTP Time is");
  syncNtp();
  Serial.println(currentTimestamp);

  timekeepingTicker.attach(1, timekeeper);
  syncNtpTicker.attach(8640000, syncNtp);
  displayUpdateTicker.attach(60, displayUpdate);
}


void loop() {

  time_t untilGRE = GREtimestamp - currentTimestamp;
  time_t untilTOEFL = TOEFLtimestamp - currentTimestamp;

  prevGreDays = curGreDays;
  curGreDays = untilGRE / (60 * 60 * 24);
  Serial.println(curGreDays);

  hoursUntilGre = untilGRE / (60 * 60) - (curGreDays * 24);
  Serial.println(hoursUntilGre);

  prevMin = minUntilGre;
  minUntilGre = untilGRE / (60) - (curGreDays * 24 * 60) - (hoursUntilGre * 60);
  Serial.println(minUntilGre);

  //TOEFL

  curToeflDays = untilTOEFL / (60 * 60 * 24);
  Serial.println(curGreDays);

  hoursUntilToefl = untilTOEFL / (60 * 60) - (curToeflDays * 24);
  Serial.println(hoursUntilToefl);

  prevMinToefl = minUntilToefl;
  minUntilToefl = untilTOEFL / (60) - (curToeflDays * 24 * 60) - (hoursUntilToefl * 60);
  Serial.println(minUntilToefl);

  if (minUntilGre == 0 && hoursUntilGre == 0 && curGreDays == 0)
    greDay = true;

  if (currentTimestamp >= GREcompletedStamp)
    greCompleted = true;

  if (minUntilToefl == 0 && hoursUntilToefl == 0 && curToeflDays == 0)
    toeflDay = true;

  if (currentTimestamp >= TOEFLcompletedStamp)
    toeflCompleted = true;

  if (!initial) {
    showFont("Century_Schoolbook_L_Bold_30", &Century_Schoolbook_L_Bold_30, false);
    initial = true;
  }

}

void showFont(const char name[], const GFXfont * f, boolean partial)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(5, 25);
  display.println("Countdown");
  display.setCursor(65, 70);
  display.println("GRE");

  display.setFont(&Century_Schoolbook_L_Bold_30);
  //  display.drawFastHLine(0, 0, 200, GxEPD_BLACK);
  display.fillRect(0, 32, 200, 3, GxEPD_BLACK);

  if (!greDay) {
    String displayString = ((curGreDays / 10 == 0) ? "0" : "") + String(curGreDays) + "d" + " " + ((hoursUntilGre / 10 == 0) ? "0" : "") + String(hoursUntilGre) + "h" + " " + ((minUntilGre / 10 == 0) ? "0" : "") + String(minUntilGre) + "m";
    display.setCursor(0, 105);
    display.println(displayString);
  }
  else if (greDay && !greCompleted) {
    display.setCursor(50, 105);
    display.println("Today!!");
  }
  else if (greDay && greCompleted) {
    display.setCursor(40, 105);
    display.println("--Done--");
  }

  display.fillRect(0, 118, 200, 3, GxEPD_BLACK);
  display.setCursor(40, 153);
  display.println("TOEFL");

  if (!toeflDay) {
    String displayStringT = ((curToeflDays / 10 == 0) ? "0" : "") + String(curToeflDays) + "d" + " " + ((hoursUntilToefl / 10 == 0) ? "0" : "") + String(hoursUntilToefl) + "h" + " " + ((minUntilToefl / 10 == 0) ? "0" : "") + String(minUntilToefl) + "m";
    display.setCursor(0, 190);
    display.println(displayStringT);
  }
  else if (toeflDay && !toeflCompleted) {
    display.setCursor(50, 190);
    display.println("Today!!");
  }
  else if (toeflDay && toeflCompleted) {
    display.setCursor(40, 190);
    display.println("--Done--");
  }

  if (partial)
    display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
  else
    display.update();
}

void displayUpdate()
{
  showFont("Century_Schoolbook_L_Bold_30", &Century_Schoolbook_L_Bold_30, (minUntilGre == 0 || minUntilToefl == 0) ? false : true);
}

void timekeeper()
{
  currentTimestamp += 1;
}

void syncNtp()
{
  wifiReconnect();
  Serial.println("NTP Synced");
  currentTimestamp = ntpClient.getUnixTime();
}

void wifiReconnect()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(ssid, password);

  showFontWifi("Century_Schoolbook_L_Bold_30", &Century_Schoolbook_L_Bold_30);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  showFont("Century_Schoolbook_L_Bold_30", &Century_Schoolbook_L_Bold_30, false);
}

void showFontWifi(const char name[], const GFXfont * f)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 25);
  display.println("Connecting");
  display.setCursor(0, 55);
  display.println("To WiFi");
  display.update();
}

