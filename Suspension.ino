#include "Cosa/Types.h"
#include "Cosa/Trace.hh"
#include "Cosa/Watchdog.hh"
#include "Cosa/Canvas/OffScreen.hh"
#include "Cosa/LCD/Driver/PCD8544.hh"
#include "Cosa/Canvas/Font/FixedNums8x16.hh"
#include "Cosa/Canvas/Icon/arduino_icon_64x32.h"
#include "Cosa/Canvas/Icon/arduino_icon_96x32.h"
#include "Cosa/AnalogPin.hh"
#include "Cosa/OutputPin.hh"
#include "Cosa/Periodic.hh"

LCD::Serial3W port;
PCD8544 lcd(&port);

#define POWER 256
#define ALPHA 178
#define AALPHA 0.3

class Compressor {
  private:
    OutputPin compr;
    uint8_t count;
    uint8_t maxon;
  public:
    Compressor(Board::DigitalPin _comp, uint8_t _max=4) : compr(_comp), count(0), maxon(_max) {};
    void on() { if (count==0) { compr.on(); };
                if (count<=maxon) { count++; };
    };
    void off() { if (count>0) { count--; };
                 if (count==0) { compr.off(); };
    };
};


    
class Strut : public Periodic {
  private:
    uint16_t desired;
    uint16_t actual;
    uint16_t smoothed;
    AnalogPin sensor;
    OutputPin vent;
    Compressor* compr;
    bool comprstate = false;
    int eepromlocation;
  public:
    Strut(Board::AnalogPin _sensor, Board::DigitalPin _vent, Compressor *_compr, int _eepromlocation, uint16_t ms=2000) : Periodic(ms), 
                sensor(_sensor), vent(_vent), eepromlocation(_eepromlocation), compr(_compr) {};
    virtual void run();
    int GetDesired();
    int SetDesired(int newdesired);
    int GetActual();
};

void Strut::run() {
  // Read
  actual=sensor.sample();
  //smoothed = (ALPHA * actual + (POWER - ALPHA) * smoothed )/ POWER;
  smoothed = AALPHA * actual + (1 - AALPHA) * smoothed;

  // Calculate
  // Write
  //vent.toggle();
  lcd.putchar('\f');
  trace << actual << endl;
  trace << smoothed << endl;
  lcd.draw_bar((smoothed * 100L)/1023, lcd.WIDTH - 20);
  if (smoothed < 300) {
    if (!comprstate) { comprstate=true; compr->off(); }
    vent.off();
  } else if ((smoothed > 300) and (smoothed < 600)) {
    if (comprstate) { comprstate=false; compr->on(); }
    vent.on();
  } else if (smoothed  > 600) {
    vent.off();
  }
}

Compressor compressor(Board::D3);
Strut strut1(Board::A0, Board::D2, &compressor, 0, 512);

OutputPin die(Board::D4);

void setup() {
  Watchdog::begin(16, Watchdog::push_timeout_events);
  
  lcd.begin();
  lcd.putchar('\f');
  
  trace.begin(&lcd);
  trace << PSTR("JEPPE RULES");
  
    
  strut1.begin();
}

void loop() {
  Event event;
  Event::queue.await(&event);
  event.dispatch();
}

