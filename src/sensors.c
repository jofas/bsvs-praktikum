#include<time.h>

#include "grovepi.h"

#define ULTRA 5
#define TEMPHUM 3
#define LIGHT 0
#define LCDTIMEDIFF 5 * 60
#define STEMPNAME  "/s_temp"
#define SHUMNAME   "/s_hum"
#define SDISTNAME  "/s_dist"
#define SLIGHTNAME "/s_light"

// initialize GrovePi, ports
void init_sensors() {
  init();
  pinMode(ULTRA, INPUT);
  pinMode(TEMPHUM, INPUT);
  pinMode(LIGHT, INPUT);

  connectLCD();
  setText("NOTHING TO RECOGNIZE");
  setRGB(0,255,0);
}

enum { green, yellow, red };

// A 2.2 SET LCD DISPLAY
// {{{

int last_priority = 0;
unsigned long last_msg = 0;

void set_lcd(char *txt, int priority) {
  unsigned long ts = (unsigned long)time(0);

  if (
    priority >= last_priority ||
    ts - last_msg > LCDTIMEDIFF
  ) {
    setText(txt);
    switch(priority) {
      case green : setRGB(0, 255, 0); break;
      case yellow: setRGB(255,255,0); break;
      case red   : setRGB(255,0,0);   break;
      default    : printf("ERROR: wrong priority\n");
    }
    last_priority = priority;
    last_msg = ts;
  }
}

// }}}

float get_temperature() {
  float ret;
  getTemperature(&ret, TEMPHUM);
  return ret;
}

float get_humidity() {
  float ret;
  getHumidity(&ret, TEMPHUM);
  return ret;
}

int get_distance() {
  int ret;
  ret = ultrasonicRead(ULTRA);
  return ret;
}

int get_light() {
  int ret;
  ret = analogRead(LIGHT);
  return ret;
}
