#include "Cosa/Types.h"
#include "Cosa/Trace.hh"
#include "Cosa/Watchdog.hh"
#include "Cosa/Periodic.hh"
#include "Cosa/EEPROM.hh"

#include "Cosa/LCD/Driver/PCD8544.hh"
#include "Cosa/Canvas/Font/FixedNums8x16.hh"

#include "Cosa/AnalogPin.hh"
#include "Cosa/OutputPin.hh"

LCD::Serial3W port;
PCD8544 lcd(&port);
EEPROM eeprom;

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
    uint16_t* eepromlocation;
  public:
    Strut(Board::AnalogPin _sensor, Board::DigitalPin _vent, Compressor *_compr, uint8_t _eepromlocation, uint16_t ms=2000) : Periodic(ms), 
                sensor(_sensor), vent(_vent), eepromlocation((uint16_t*)(_eepromlocation*sizeof(uint16_t))), compr(_compr) {};
    virtual void run();
    uint16_t GetDesired();
    uint16_t SetDesired(uint16_t newdesired);
    uint16_t GetActual();
};

uint16_t Strut::GetDesired() {
  eeprom.read<uint16_t>(&desired, eepromlocation);
  return desired;
}

uint16_t Strut::SetDesired(uint16_t newdesired) {
  desired=newdesired;
  eeprom.write<uint16_t>(eepromlocation, &desired);
  return desired;
}

uint16_t Strut::GetActual() {
  return actual;
}

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

Compressor compressor(Board::D2);
Strut strut1(Board::A0, Board::D3, &compressor, 0);
Strut strut2(Board::A1, Board::D4, &compressor, 1);
Strut strut3(Board::A2, Board::D5, &compressor, 2);
Strut strut4(Board::A3, Board::D6, &compressor, 3);

void setup() {
  Watchdog::begin(16, Watchdog::push_timeout_events);
  
  lcd.begin();
  lcd.putchar('\f');
  
  trace.begin(&lcd);
  trace << PSTR("JEPPE RULES");
  
    
  strut1.begin();
  strut2.begin();
  strut3.begin();
  strut4.begin();
}

void loop() {
  Event event;
  Event::queue.await(&event);
  event.dispatch();
}

