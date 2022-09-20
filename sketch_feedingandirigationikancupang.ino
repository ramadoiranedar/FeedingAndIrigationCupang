#define SERIAL_BAUD 9600
#define ESP8266_BAUD 9600
#define PIN_RX 3
#define PIN_TX 2
#define PIN_TRIG 4
#define PIN_ECHO 5
#define PIN_SERVO 6
#define PIN_RELAY1_WATERPUMP 7
#define PIN_RELAY2_WATERPUMP 8
#define PIN_LED_INDICATOR_CONNECTION 13
#define RELAY_ON LOW #define RELAY_OFF HIGH

#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h> #include <SoftwareSerial.h>
#include <TimeLib.h> #include <WidgetRTC.h> #include <Servo.h>

char auth[] = "RAHASIA_SAYA"; char ssid[] = "WIFI_SAYA"; char pass[] = "PASSWORD_SAYA"; char tz[] = "Asia/Jakarta"; String tjp1 = "";
String tjp2 = ""; String wjpa = ""; String tjpa = ""; int rowIndex = 0;
int posServoFeeding = 0;
int timeoutFeeding = 0, timeoutIrigating = 0;

SoftwareSerial EspSerial(PIN_TX, PIN_RX); ESP8266 wifi(&EspSerial);
BlynkTimer timer; WidgetRTC rtc; Servo servoFeeding;

BLYNK_CONNECTED() {
  rtc.begin(); Blynk.syncAll();
}

BLYNK_WRITE(V1) { // time jadwal pakan 1 TimeInputParam t(param);
  tjp1 = format2DigitNumber(String(t.getStartHour())) + ":" + format2D igitNumber(String(t.getStartMinute())) + ":" + format2DigitNumber(Stri ng(t.getStartSecond()));
}

BLYNK_WRITE(V2) { // time jadwal pakan 2 TimeInputParam t(param);
  tjp2 = format2DigitNumber(String(t.getStartHour())) + ":" + format2D igitNumber(String(t.getStartMinute())) + ":" + format2DigitNumber(Stri ng(t.getStartSecond()));
}

BLYNK_WRITE(V3) { // weekday jadwal penggantian air 
  wjpa = param.asInt() - 1;
}

BLYNK_WRITE(V4) { // time jadwal penggantian air TimeInputParam t(param);
  tjpa = format2DigitNumber(String(t.getStartHour())) + ":" + format2D igitNumber(String(t.getStartMinute())) + ":" + format2DigitNumber(Stri ng(t.getStartSecond()));
}

BLYNK_WRITE(V6) { // timeout pemberian pakan 
  timeoutFeeding = param.asInt();
}

BLYNK_WRITE(V7) { // timeout penggantian air 
  timeoutIrigating = param.asInt();
}

void setup() { 
  Serial.begin(SERIAL_BAUD); 
  EspSerial.begin(ESP8266_BAUD); delay(10);
  Blynk.begin(auth, wifi, ssid, pass, "blynk-cloud.com", 8442); 
  setSyncInterval(10 * 60);
  setPinsMode();
  timer.setInterval(1100L, checkPercentageFoodScraps); timer.setInterval(1000L, checkJPandJPA); timer.setInterval(1000L, checkConnection);
}

void loop() { 
  timer.run();
  Blynk.run();
}

void setPinsMode() { 
  pinMode(PIN_TRIG, OUTPUT); 
  pinMode(PIN_ECHO, INPUT);

  posCloseServoFeeding();

  pinMode(PIN_RELAY1_WATERPUMP, OUTPUT); 
  pinMode(PIN_RELAY2_WATERPUMP, OUTPUT);
  turnOffWaterpump();

  pinMode(PIN_LED_INDICATOR_CONNECTION, OUTPUT); digitalWrite(PIN_LED_INDICATOR_CONNECTION, LOW);
}


String format2DigitNumber(String num) { 
  if (num.length() > 1) {
    return num;
  } else {
    return "0" + num;
  }
}

String weekdayNumToString(int num) { 
  if (num == 1) {
    return "Minggu";
  } else if (num == 2) { 
    return "Senin";
  } else if (num == 3) { 
    return "Selasa";
  } else if (num == 4) { 
    return "Rabu";
  } else if (num == 5) { 
    return "Kamis";
  } else if (num == 6) { 
    return "Jumat";
  } else if (num == 7) { 
    return "Sabtu";
  }
}

void turnOffWaterpump() { 
  digitalWrite(PIN_RELAY1_WATERPUMP, RELAY_OFF); 
  digitalWrite(PIN_RELAY2_WATERPUMP, RELAY_OFF);
}

void turnOnWaterpump() { 
  digitalWrite(PIN_RELAY1_WATERPUMP, RELAY_ON); 
  digitalWrite(PIN_RELAY2_WATERPUMP, RELAY_ON);
}

void setWaterpumpIrigating() { 
  turnOnWaterpump();
  long timeOut = long(timeoutIrigating) * 1000L * 60L;
  // then turn off waterpump based on timeout irigating... 
  timer.setTimeout(timeOut, turnOffWaterpump);
}

int getDistanceSensorUltrasonic() { 
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);

  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH); // menerima suara ultrasoni c | (pulseIn) untuk mentotal waktu tunggu
  long distance = (duration / 2) / 29.1; // mengubah durasi menjadi ja rak (cm)
  return distance;
}

void checkPercentageFoodScraps() {
  // (distance / max distance) * 100; || 14cm jarak dari atas tutup to ples ke dasar;
  float percentFromBottomToTop = (getDistanceSensorUltrasonic() / 14.0) * 100; // persentase sisa makanan dari bawah ke atas;
  int percentFromTopToBottom = 100 - (int)floor(percentFromBottomToTop);	// persentase sisa makanan dari atas ke bawah;

  // check offset
  if (percentFromBottomToTop > 100 || percentFromTopToBottom < 0) {
    // Serial.println(F("Ada kesalahan pada sistem atau tutup tempat p akan sedang terbuka"));
    percentFromTopToBottom = 0;
  }

  Blynk.virtualWrite(V0, percentFromTopToBottom);
}

void posOpenServoFeeding() {
  servoFeeding.attach(PIN_SERVO);
  for (posServoFeeding; posServoFeeding <= 90; posServoFeeding++)
  {
    servoFeeding.write(posServoFeeding); delay(10);
    if (posServoFeeding == 90) {
      servoFeeding.detach();
    }
  }
}

void posCloseServoFeeding()
{
  servoFeeding.attach(PIN_SERVO);
  for (posServoFeeding; posServoFeeding >= 0; posServoFeeding--) {
    servoFeeding.write(posServoFeeding); delay(10);
    if (posServoFeeding == 0) {
      servoFeeding.detach();
    }
  }
}

void autoServoFeeding() { 
  posOpenServoFeeding();
  long timeOut = long(timeoutFeeding) * 1000L;
  // then close the servo feeding... 
  timer.setTimeout(timeOut, posCloseServoFeeding);
}

void checkJPandJPA() {
  String currTime = format2DigitNumber(String(hour())) + ":" + format2 DigitNumber(String(minute())) + ":" + format2DigitNumber(String(second ()));
  int currWeekdayNum = weekday();
  String currWeekdayStr = weekdayNumToString(currWeekdayNum); 
  String currDate = String(day()) + "/" + month() + "/" + year();

  if (tjp1 == "13398:28:15") { // fix user input reset time and set UI timeinput to reset
    tjp1 = "";
    Blynk.virtualWrite(V1, -1, -1, tz);
  } else if (tjp1 == currTime) {
    rowIndex++;
    Blynk.virtualWrite(V5, "add", rowIndex, currTime, "P1"); Blynk.virtualWrite(V5, "pick", rowIndex);
    autoServoFeeding();
  }

  if (tjp2 == "13398:28:15") { // fix user input reset time and set UI timeinput to reset
    tjp2 = "";
    Blynk.virtualWrite(V2, -1, -1, tz);
  } else if (tjp2 == currTime) {
    rowIndex++;
    Blynk.virtualWrite(V5, "add", rowIndex, currTime, "P2"); Blynk.virtualWrite(V5, "pick", rowIndex);
    autoServoFeeding();
  }

  if (tjpa == "13398:28:15") { // fix user input reset time and set UI timeinput to reset
    tjpa = "";
    Blynk.virtualWrite(V4, -1, -1, tz);
  } else if (tjpa == currTime) {
    if (wjpa == String(currWeekdayNum)) { rowIndex++;
      Blynk.virtualWrite(V5, "add", rowIndex, currTime, "PA"); Blynk.virtualWrite(V5, "pick", rowIndex);
      setWaterpumpIrigating();
    } else if (wjpa == "0") {
      // do nothing for fix user not selecting weekday the WJPA "--- Pilih Hari---"
      wjpa = ""; Blynk.virtualWrite(V3, 0);
    }
  }
}

void turnOnLedIndicatorConnectionBlynk() { 
  digitalWrite(PIN_LED_INDICATOR_CONNECTION, HIGH);
}

void turnOffLedIndicatorConnectionBlynk() { 
  digitalWrite(PIN_LED_INDICATOR_CONNECTION, LOW);
}

void checkConnection() { 
  if (Blynk.connected()) {
    Serial.println("Blynk Terkoneksi"); turnOffLedIndicatorConnectionBlynk();
  } else {
    Serial.println("Blynk Tidak Terkoneksi!!!"); turnOnLedIndicatorConnectionBlynk();
  }
}
