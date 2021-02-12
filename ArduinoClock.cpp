#include <Arduino.h>
#include <TM1637Display.h>

using ushort = unsigned short;
using uchar = unsigned char;

const ushort MINUTE_5 = 300;
unsigned long seconds = millis() / 1000;

class Uhr {
    TM1637Display *display;
    uchar time = 30;
    bool running = false;
    long int startTime = seconds;
    bool alert = false;
    uchar incrementPin = 255;
    uchar decrementPin = 255;
    uchar startstopPin = 255;

  public:
    Uhr() {}

    void setDisplay(TM1637Display* display) {
      this->display = display;
    }

    void setButtons(short inc, short dec, short start) {
      this->incrementPin = inc;
      this->decrementPin = dec;
      this->startstopPin = start;
    }

    void update() {
      this->checkControls();
      ushort time = this->time * 60;

      if (this->running) {
        time = time - (seconds - this->startTime)*10;
        if (time <= 0||this->alert) {
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
      if (!this->running && this->incrementPin != 255 && analogRead(this->incrementPin) < 5) {
        this->time = (this->time + 5 < 255) ? this->time + 5 : 255;
      }
      if (!this->running && this->decrementPin != 255 && analogRead(this->decrementPin) < 5) {
        this->time = (this->time - 5 > 5) ? this->time - 5 : 5;
      }
      if (this->startstopPin != 255
          && analogRead(this->startstopPin) < 5
          && seconds - this->startTime >= 1) {
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
  uhren[0].setDisplay(new TM1637Display(2, 3));
  pinMode(13, OUTPUT);
  uhren[0].setButtons(A0, A1, A2);
  uhren[1].setDisplay(new TM1637Display(4, 5));
  uhren[1].setButtons(-1, -1, -1);
  uhren[2].setDisplay(new TM1637Display(7, 6));
  uhren[2].setButtons(-1, -1, -1);
}

void loop()
{
  bool alerting = false;
  seconds = millis() / 1000;
  for (uchar i = 0; i < 3; i++) {
    uhren[i].update();
    alerting = alerting || uhren[i].isAlerting();
  }
  alert.write(alerting);
}
