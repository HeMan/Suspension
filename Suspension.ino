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

#include "Suspension.hh"

#include "Cosa/Menu.hh"
#include "Cosa/Trace.hh"

#include "Cosa/LCD/Driver/PCD8544.hh"

// Select PCD8544 IO Adapter, SPI
LCD::SPI3W port;
PCD8544 lcd(&port);

// Create the strut objects
STRUT(strut1, "Front Left",  Board::A0, Board::D3);
STRUT(strut2, "Front Right", Board::A1, Board::D4);
STRUT(strut3, "Rear Left",   Board::A2, Board::D5);
STRUT(strut4, "Rear Right",  Board::A3, Board::D6);


MENU_BEGIN(root_menu, "Demo")
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

