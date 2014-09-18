/**
 * @file Suspension.ino
 * @version 0.9
 *
 * @section License
 * Copyright (C) 2014, Jimmy Hedman
 *
 *
 *
 */

#include "Cosa/Types.h"
#include "Cosa/Trace.hh"
#include "Cosa/Watchdog.hh"
#include "Cosa/Menu.hh"
#include "Cosa/Periodic.hh"
#include "Cosa/EEPROM.hh"

#include "Cosa/LCD/Driver/PCD8544.hh"
#include "Cosa/Canvas/Font/FixedNums8x16.hh"

#include "Cosa/AnalogPin.hh"
#include "Cosa/OutputPin.hh"

#include "Suspension.hh"

LCD::SPI3W port;
PCD8544 lcd(&port);
EEPROM eeprom;

#define POWER 256
#define ALPHA 178
#define AALPHA 0.3
#define HYSTERESIS 100
Strut::Strut(Board::AnalogPin _sensor,
             Board::DigitalPin _vent,
             Compressor *_compr,
             int8_t _eepromlocation,
             uint16_t ms) :             Periodic(ms),
                                        sensor(_sensor),
                                        vent(_vent),
                                        compr(_compr) {
  eepromlocation = (int16_t*)(_eepromlocation*sizeof(int16_t));
  comprstate = false;
  is_leveled = false;
  GetDesired();
};

int16_t Strut::GetDesired() {
  eeprom.read<int16_t>(&desired, eepromlocation);
  saveddesired = desired;
  return desired;
}

int16_t Strut::SetDesired() {
  if (saveddesired != desired) {
    eeprom.write<int16_t>(eepromlocation, &desired);
  }
  return desired;
}

int16_t Strut::SetDesired(int16_t newdesired) {
  desired = newdesired;
  eeprom.write<int16_t>(eepromlocation, &desired);
  return desired;
}

int16_t Strut::GetActual() {
  return actual;
}

void Strut::run() {
  // Read
  actual=sensor.sample();
  //smoothed = (ALPHA * actual + (POWER - ALPHA) * smoothed )/ POWER;
  smoothed = AALPHA * actual + (1 - AALPHA) * smoothed;

  // Calculate
  // Write
  SetDesired();
  if (smoothed < desired - HYSTERESIS) {
    is_leveled = false;
    if (!comprstate) { comprstate=true; compr->off(); }
    vent.off();
  } else if ((smoothed > desired + HYSTERESIS) and (smoothed < desired - HYSTERESIS)) {
    is_leveled = true;
    if (comprstate) { comprstate=false; compr->on(); }
    vent.on();
  } else if (smoothed  > desired + HYSTERESIS) {
    is_leveled = false;
    vent.off();
  }
}

Compressor compressor(Board::D2);
STRUT(strut1, "Front Left",  Board::A0, Board::D3, 0);
STRUT(strut2, "Front Right", Board::A1, Board::D4, 1);
STRUT(strut3, "Rear Left",   Board::A2, Board::D5, 2);
STRUT(strut4, "Rear Right",  Board::A3, Board::D6, 3);


MENU_BEGIN(root_menu,"Demo")
  MENU_ITEM(strut1_menu)
  MENU_ITEM(strut2_menu)
  MENU_ITEM(strut3_menu)
  MENU_ITEM(strut4_menu)
MENU_END(root_menu)

Menu::Walker walker(&lcd, &root_menu);
Menu::RotaryController rotary(&walker, Board::PCI10, Board::PCI12, Board::D7);

void setup() {
  Watchdog::begin(16, Watchdog::push_timeout_events);
  
  lcd.begin();
  lcd.display_contrast(30);
  lcd.putchar('\f');
  
  trace.begin(&lcd);
  trace << PSTR("JEPPE RULES");
  
  walker.begin();
  rotary.begin();
    
  strut1.begin();
  strut2.begin();

  while (!strut1.is_leveled && !strut2.is_leveled) {
    Event event;
    Event::queue.await(&event);
    event.dispatch();
  }

  strut3.begin();
  strut4.begin();
}

void loop() {
  Event event;
  Event::queue.await(&event);
  event.dispatch();
}

