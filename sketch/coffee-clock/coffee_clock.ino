#include <DS3231.h>
#include <TM1637.h>

#define heater 0
#define pump 1
#define alarm 2
#define brewSwitch 3
#define brewButton 4
#define clockSetButton 5
#define alarmSetButton 6
#define clockPot A0
#define CLK 8
#define DIO 9

const int pumpDuration = 100;
const int heatDuration = 100;
const int timeBuffer = 10;

int brewSwitchState [2] = {0, 0};
int brewButtonState [2] = {0, 0};
int clockButtonState [2] = {0, 0};
int alarmButtonState [2] = {0, 0};
int clockPotValue = 0;

bool isBrewing = false;
bool pumpActive = false;
bool pumpCompleted = false;
bool heaterActive = false;
bool isEditingClock = false;
bool isEditingAlarm = false;

int editedClockValue = 0;
int alarmValue = 0;
int brewStartTime = 0;

TM1637 tm(CLK, DIO);
DS3231 rtc(SDA, SCL);

void setup(){
  pinMode(brewSwitch, INPUT);
  pinMode(brewButton, INPUT);
  pinMode(clockSetButton, INPUT);
  pinMode(alarmSetButton, INPUT);

  pinMode(heater, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(alarm, OUTPUT);
  
  rtc.begin();

  tm.init();
  tm.set(5);
}

void loop(){
  String timeString = rtc.getTimeStr();

  if (isEditingClock) { displayTime(getTimeString(editedClockValue)); }
  else if (isEditingAlarm) { displayTime(getTimeString(alarmValue)); }
  else { displayTime(timeString); }
  
  brewSwitchState[0] = brewSwitchState[1]; brewSwitchState[1] = digitalRead(brewSwitch);
  brewButtonState[0] = brewButtonState[1]; brewButtonState[1] = digitalRead(brewButton);
  clockButtonState[0] = clockButtonState[1]; clockButtonState[1] = digitalRead(clockSetButton);
  alarmButtonState[0] = alarmButtonState[1]; alarmButtonState[1] = digitalRead(alarmSetButton);
  clockPotValue = map(analogRead(clockPot), 0, 1023, 0, 720);
  
  if (checkPressed(clockButtonState)) {
    if (isEditingClock) rtc.setTime(int(editedClockValue / 60), editedClockValue % 60, 0);
    isEditingClock = !isEditingClock;
  }
  
  if (checkPressed(alarmButtonState)) isEditingAlarm = !isEditingAlarm;

  if (isEditingClock) editedClockValue = clockPotValue;
  if (isEditingAlarm) alarmValue = clockPotValue;

  if (checkPressed(brewButtonState) || timeToAlarm(timeString) <= pumpDuration + heatDuration + timeBuffer) {
    if (!isBrewing) brewStartTime = timeInSeconds(timeString);
    isBrewing = !isBrewing;
  }

  if (isBrewing) {
    if (timeInSeconds(timeString) - brewStartTime >= pumpDuration + heatDuration) { 
      heaterActive = false; 
      return;
    }
    
    if (timeInSeconds(timeString) - brewStartTime >= pumpDuration) {
      pumpActive = false;
      heaterActive = true;
      return;
    } 
    
    pumpActive = true;
  }
  
  if (pumpActive) { analogWrite(pump, 100); } else { analogWrite(pump, 0); }
  if (heaterActive) { analogWrite(heater, 255); } else { analogWrite(heater, 0); }
}

bool checkPressed(int state [2]) {
  if (state[0] == LOW && state[1] == HIGH) return true;
  
  return false;
}

void displayTime(String timeString) {
  int hours = timeString.substring(0, 2).toInt() % 12;
  int minuteTens = String(timeString.charAt(3)).toInt();
  int minuteOnes = String(timeString.charAt(4)).toInt();

  if (hours == 0) hours = 12;
  if (hours >= 10) tm.display(0, 1);
  
  tm.display(1, hours % 10);
  tm.display(2, minuteTens);
  tm.display(3, minuteOnes);
}

int timeToAlarm(String timeString) {
  int minutes = timeString.substring(0, 2).toInt() * 60 + timeString.substring(3, 5).toInt();
  if (minutes < alarmValue) return alarmValue - minutes;

  return 1440 - minutes + alarmValue;
}

int timeInSeconds(String timeString) {
  return timeString.substring(0, 2).toInt() * 3600 + timeString.substring(3, 5).toInt() * 60 + timeString.substring(6, 8).toInt();
}

String getTimeString(int minutes) {
  int hours = int(minutes / 60);
  int remainingMinutes = minutes % 60;
  String hourString = String(hours);
  String minuteString = String(remainingMinutes);
  
  if (hours < 10) hourString = "0" + hourString;
  if (remainingMinutes < 10) minuteString = "0" + minuteString;
  return hourString + ":" + minuteString;
}
