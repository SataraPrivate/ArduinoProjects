#include <Arduino.h>
#include <TM1637Display.h>

#define MINUTE_5 300

#define ALERT_TIME 5

#define KEY_MODE_UNPRESSED 0
#define KEY_MODE_DOWN 1
#define KEY_MODE_PRESSED 2
#define KEY_MODE_UP 3
#define KEY_MODE_UP_TIME 300

#define KEY_NONE 0
#define KEY_PLUS_1 1
#define KEY_MINUS_1 2
#define KEY_START_1 3
#define KEY_PLUS_2 4
#define KEY_MINUS_2 5
#define KEY_START_2 6
#define KEY_PLUS_3 7
#define KEY_MINUS_3 8
#define KEY_START_3 9

#define KEY_SIZE 10

#define KEYBOARD_ANALOG_PIN A0

using ushort = unsigned short;
using uchar = unsigned char;

unsigned long seconds = millis() / 1000;

class Keyboard {
  private:
    static    uchar keys[KEY_SIZE];
    static unsigned long upTime;
  public:
    static void init() {
      for (int i = 0; i < KEY_SIZE; i++) {
        Keyboard::keys[i] = KEY_MODE_UNPRESSED;
      }
    }

    static void update() {
      if (millis() - Keyboard::upTime <= KEY_MODE_UP_TIME) {
        delay(KEY_MODE_UP_TIME - (millis() - Keyboard::upTime));
      }
      int value = analogRead(KEYBOARD_ANALOG_PIN);
      uchar currentButton = KEY_NONE;
      if (Keyboard::inRange(value, 500, 20)) {
        currentButton = KEY_PLUS_1;
      } else if (Keyboard::inRange(value, 560, 20)) {
        currentButton = KEY_MINUS_1;
      } else if (Keyboard::inRange(value, 643, 15)) {
        currentButton = KEY_START_1;
      } else if (Keyboard::inRange(value, 685, 15)) {
        currentButton = KEY_PLUS_2;
      } else if (Keyboard::inRange(value, 730, 15)) {
        currentButton = KEY_MINUS_2;
      } else if (Keyboard::inRange(value, 785, 15)) {
        currentButton = KEY_START_2;
      } else if (Keyboard::inRange(value, 850, 15)) {
        currentButton = KEY_PLUS_3;
      } else if (Keyboard::inRange(value, 920, 15)) {
        currentButton = KEY_MINUS_3;
      } else if (Keyboard::inRange(value, 1010, 15)) {
        currentButton = KEY_START_3;
      }
      for (uchar i = 0; i < 10; i++) {
        if (currentButton == i) {
          //UNPRESSED -> UP -> PRESSED
          if (Keyboard::keys[i] == KEY_MODE_UNPRESSED) {
            Keyboard::keys[i] = KEY_MODE_UP;
            Keyboard::upTime = millis();
          } else {
            Keyboard::keys[i] = KEY_MODE_PRESSED;
          }

        } else {
          //PRESSED/UP -> DOWN -> UNPRESSED
          Keyboard::keys[i] = Keyboard::keys[i] == KEY_MODE_PRESSED || Keyboard::keys[i] == KEY_MODE_UP ? KEY_MODE_DOWN : KEY_MODE_UNPRESSED;
        }
      }
    }
    static bool inRange(ushort value, ushort range, ushort tolerance) {
      return value >= range - tolerance && value <= range + tolerance;
    }

    static bool isUp(uchar key) {
      return Keyboard::keys[key] == KEY_MODE_UP;
    }

    static bool isDown(uchar key) {
      return Keyboard::keys[key] == KEY_MODE_DOWN;
    }

    static bool isPressed(uchar key) {
      return Keyboard::keys[key] == KEY_MODE_PRESSED;
    }

    static bool isUpOrPressed(uchar key) {
      return Keyboard::isUp(key) || Keyboard::isPressed(key);
    }
    static uchar getMode(uchar key) {
      return Keyboard::keys[key];
    }
};
uchar Keyboard::keys[10] = {KEY_MODE_UNPRESSED};
unsigned long Keyboard::upTime = 0;

class Clock {
    TM1637Display *display;
    uchar time = 30;
    bool running = false;
    long int startTime = seconds;
    bool alert = false;

    ushort incrementBtn = 0;
    ushort decrementBtn = 0;
    ushort startStopBtn = 0;

  public:

    void setDisplay(TM1637Display* display) {
      this->display = display;
    }

    void setKeyboard(ushort inc, ushort dec, ushort start) {
      this->incrementBtn = inc;
      this->decrementBtn = dec;
      this->startStopBtn = start;
    }

    void update() {
      this->checkControls();
      short time = this->time * 60;

      if (this->running) {
        time = time - (seconds - this->startTime);
        if (time <= 0 || this->alert) {
          this->alert = true;
          if (-time >= ALERT_TIME ) {
            this->alert = false;
            this->running = false;
            this->startTime = seconds;
          }
          time = 0;
        }
      }

      this->setDisplayTime(time, this->alert ? (millis() / 500 % 2 * 6 + 1) : (this->running ? 0x04 : 0x01));
    }

    void setDisplayTime(ushort time, uchar brightness) {
      uchar d12 = time / 60;
      uchar d34 = time % 60;

      if (d12 >= 60) {
        d34 = d12 % 60;
        d12 = d12 / 60;
      }
      this->display->setBrightness(brightness);
      this->display->showNumberDecEx(d34, 0, true, 2, 2);
      this->display->showNumberDecEx(d12, (0x40), true, 2, 0);
    }

    void checkControls() {
      if (!this->running && Keyboard::isUpOrPressed(this->incrementBtn)) {
        this->time = (this->time + 5 < 255) ? this->time + 5 : 255;
      }
      if (!this->running && Keyboard::isUpOrPressed(this->decrementBtn)) {
        this->time = (this->time - 5 > 5) ? this->time - 5 : 5;
      }
      if (Keyboard::isUp(this->startStopBtn)) {
        this->running = !this->running;
        this->startTime = seconds;
        this->alert = false;
      }
    }

    bool isAlerting() {
      return this->alert;
    }

};

class Pin {
    int pin;

  public:
    Pin(int pin) {
      this->pin = pin;
    }
    void write(bool on) {
      digitalWrite(this->pin, on ? LOW : HIGH);
    }
};

#define CLOCK_NUMBER 3

Clock clocks[CLOCK_NUMBER];
Pin alert(13);

void setup()
{
  Keyboard::init();
  clocks[0].setDisplay(new TM1637Display(2, 3));
  clocks[0].setKeyboard(KEY_PLUS_1, KEY_MINUS_1, KEY_START_1);
  clocks[1].setDisplay(new TM1637Display(4, 5));
  clocks[1].setKeyboard(KEY_PLUS_2, KEY_MINUS_2, KEY_START_2);
  clocks[2].setDisplay(new TM1637Display(7, 6));
  clocks[2].setKeyboard(KEY_PLUS_3, KEY_MINUS_3, KEY_START_3);
}

void loop()
{
  Keyboard::update();
  bool alerting = false;
  seconds = millis() / 1000;
  for (uchar i = 0; i < CLOCK_NUMBER; i++) {
    clocks[i].update();
    alerting = alerting || clocks[i].isAlerting();
  }
  alert.write(alerting);
}
