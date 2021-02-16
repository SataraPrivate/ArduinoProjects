#include <Arduino.h>
#include <TM1637Display.h>

using ushort = unsigned short;
using uchar = unsigned char;

const ushort MINUTE_5 = 300;
unsigned long seconds = millis() / 1000;

#define BUTTON_UNPRESSED 0
#define BUTTON_DOWN 1
#define BUTTON_PRESSED 2
#define BUTTON_UP 3

#define BUTTON_NONE 0
#define BUTTON_PLUS_1 1
#define BUTTON_MINUS_1 2
#define BUTTON_START_1 3
#define BUTTON_PLUS_2 4
#define BUTTON_MINUS_2 5
#define BUTTON_START_2 6
#define BUTTON_PLUS_3 7
#define BUTTON_MINUS_3 8
#define BUTTON_START_3 9


class Buttons {
  private:
    static    uchar buttons[10];
    static unsigned long upTime;
  public:
    static void init() {
      Buttons::buttons[BUTTON_PLUS_1] = BUTTON_UNPRESSED;
      Buttons::buttons[BUTTON_MINUS_1] = BUTTON_UNPRESSED;
      Buttons::buttons[BUTTON_START_1] = BUTTON_UNPRESSED;
      Buttons::buttons[BUTTON_PLUS_2] = BUTTON_UNPRESSED;
      Buttons::buttons[BUTTON_MINUS_2] = BUTTON_UNPRESSED;
      Buttons::buttons[BUTTON_START_2] = BUTTON_UNPRESSED;
      Buttons::buttons[BUTTON_PLUS_3] = BUTTON_UNPRESSED;
      Buttons::buttons[BUTTON_MINUS_3] = BUTTON_UNPRESSED;
      Buttons::buttons[BUTTON_START_3] = BUTTON_UNPRESSED;
    }

    static void update() {
      if (millis() - Buttons::upTime < 300) {
        delay(300 - (millis() - Buttons::upTime));
      }
      int value = analogRead(A0);
      uchar currentButton = BUTTON_NONE;
      if (Buttons::inRange(value, 500, 15)) {
        currentButton = BUTTON_PLUS_1;
      } else if (Buttons::inRange(value, 560, 15)) {
        currentButton = BUTTON_MINUS_1;
      } else if (Buttons::inRange(value, 643, 15)) {
        currentButton = BUTTON_START_1;
      } else if (Buttons::inRange(value, 685, 15)) {
        currentButton = BUTTON_PLUS_2;
      } else if (Buttons::inRange(value, 730, 15)) {
        currentButton = BUTTON_MINUS_2;
      } else if (Buttons::inRange(value, 785, 15)) {
        currentButton = BUTTON_START_2;
      } else if (Buttons::inRange(value, 850, 15)) {
        currentButton = BUTTON_PLUS_3;
      } else if (Buttons::inRange(value, 920, 15)) {
        currentButton = BUTTON_MINUS_3;
      } else if (Buttons::inRange(value, 1010, 15)) {
        currentButton = BUTTON_START_3;
      }
      for (uchar i = 0; i < 10; i++) {
        if (currentButton == i) {
          //UNPRESSED -> UP -> PRESSED
          if (Buttons::buttons[i] == BUTTON_UNPRESSED) {
            Buttons::buttons[i] = BUTTON_UP;
            Buttons::upTime = millis();
          } else {
            Buttons::buttons[i] = BUTTON_PRESSED;
          }

        } else {
          //PRESSED/UP -> DOWN -> UNPRESSED
          Buttons::buttons[i] = Buttons::buttons[i] == BUTTON_PRESSED || Buttons::buttons[i] == BUTTON_UP ? BUTTON_DOWN : BUTTON_UNPRESSED;
        }
      }
    }
    static bool inRange(ushort value, ushort range, ushort tolerance) {
      return value >= range - tolerance && value <= range + tolerance;
    }

    static bool isUp(uchar key) {
      return Buttons::buttons[key] == BUTTON_UP;
    }

    static bool isDown(uchar key) {
      return Buttons::buttons[key] == BUTTON_DOWN;
    }

    static bool isPressed(uchar key) {
      return Buttons::buttons[key] == BUTTON_PRESSED;
    }

    static bool isUpOrPressed(uchar key) {
      return Buttons::isUp(key) || Buttons::isPressed(key);
    }
    static uchar getMode(uchar key) {
      return Buttons::buttons[key];
    }
};
uchar Buttons::buttons[10] = {BUTTON_UNPRESSED};
unsigned long Buttons::upTime = 0;

class Uhr {
    TM1637Display *display;
    uchar time = 30;
    bool running = false;
    long int startTime = seconds;
    bool alert = false;

    ushort incrementBtn = 0;
    ushort decrementBtn = 0;
    ushort startStopBtn = 0;

  public:
    Uhr() {}

    void setDisplay(TM1637Display* display) {
      this->display = display;
    }

    void setButtons(ushort inc, ushort dec, ushort start) {
      this->incrementBtn = inc;
      this->decrementBtn = dec;
      this->startStopBtn = start;
    }

    void update() {
      this->checkControls();
      ushort time = this->time * 60;

      if (this->running) {
        time = time - (seconds - this->startTime);
        if (time <= 0 || this->alert) {
          time = 0;
          this->alert = true;
        }
      }

      this->setDisplayTime(time, this->alert ? (millis() / 500 % 2 * 6 + 1) : 0x04);
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
      if (!this->running && Buttons::isUpOrPressed(this->incrementBtn)) {
        this->time = (this->time + 5 < 255) ? this->time + 5 : 255;
      }
      if (!this->running && Buttons::isUpOrPressed(this->decrementBtn)) {
        this->time = (this->time - 5 > 5) ? this->time - 5 : 5;
      }
      if (Buttons::isUp(this->startStopBtn)) {
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

Uhr uhren[3];
Pin alert(13);


void setup()
{
  Buttons::init();
  uhren[0].setDisplay(new TM1637Display(2, 3));
  pinMode(13, OUTPUT);
  uhren[0].setButtons(BUTTON_PLUS_1, BUTTON_MINUS_1, BUTTON_START_1);
  uhren[1].setDisplay(new TM1637Display(4, 5));
  uhren[1].setButtons(BUTTON_PLUS_2, BUTTON_MINUS_2, BUTTON_START_2);
  uhren[2].setDisplay(new TM1637Display(7, 6));
  uhren[2].setButtons(BUTTON_PLUS_3, BUTTON_MINUS_3, BUTTON_START_3);
}

void loop()
{
  Buttons::update();
  bool alerting = false;
  seconds = millis() / 1000;
  for (uchar i = 0; i < 3; i++) {
    uhren[i].update();
    alerting = alerting || uhren[i].isAlerting();
  }
  alert.write(alerting);
}
