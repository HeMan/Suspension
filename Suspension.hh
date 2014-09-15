/**
 * @file Suspension.h
 * @version 0.9
 *
 * @section License
 * Copyright (C) 2014, Jimmy Hedman
 *
 *
 *
 */

#ifndef SUSPENSION_H
#define SUSPENSION_H

#define STRUT(var,name,sensor,vent,eepromlocation)      \
  Strut var(sensor, vent, &compressor, eepromlocation); \
  const char var ## _name[] PROGMEM = name;             \
  const Menu::int_range_t var ## _menu PROGMEM = {      \
  {                                                     \
    Menu::INT_RANGE,                                    \
    (str_P) var ## _name                                        \
  },                                                    \
  0,                                                    \
  1023,                                                 \
  &var.desired                                          \
};

class Compressor {
  private:
    OutputPin compr;
    int8_t count;
    int8_t maxon;
  public:
    Compressor(Board::DigitalPin _comp, int8_t _max=4) : compr(_comp), count(0), maxon(_max) {};
    void on() { if (count==0) { compr.on(); };
                if (count<=maxon) { count++; };
    };
    void off() { if (count>0) { count--; };
                 if (count==0) { compr.off(); };
    };
};

    
class Strut : public Periodic {
  private:
    int16_t actual;
    int16_t saveddesired;
    int16_t smoothed;
    AnalogPin sensor;
    OutputPin vent;
    Compressor* compr;
    bool comprstate;
    int16_t* eepromlocation;
  public:
    Strut(Board::AnalogPin _sensor, Board::DigitalPin _vent, Compressor *_compr, int8_t _eepromlocation, uint16_t ms=2000);
    virtual void run();
    int16_t GetDesired();
    int16_t SetDesired();
    int16_t SetDesired(int16_t newdesired);
    int16_t GetActual();
    int16_t desired;
    bool is_leveled;
};

#endif
