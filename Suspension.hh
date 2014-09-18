/**
 * @file Suspension.h
 * @version 0.9
 *
 * @section License
 * Copyright (C) 2014, Jimmy Hedman
 *
 * @section Description
 * This is the declaration of Compressor and Strut
 *
 *
 *
 */

#ifndef SUSPENSION_H
#define SUSPENSION_H

#include "Cosa/Bits.h"
#include "Cosa/EEPROM.hh"
#include "Cosa/Menu.hh"
#include "Cosa/Periodic.hh"

EEPROM eeprom;

#define POWER 256
#define ALPHA 178
#define AALPHA 0.3
#define HYSTERESIS 100


/**
 * Support macro to create the strut and create an menu entry
 * @param[in] var variable name
 * @param[in] name what is showed in the menu
 * @param[in] sensor analog pin in
 * @param[in] vent digital pin to control
 */
#define STRUT(var,name,sensor,vent)                     \
  Strut var(sensor, vent);                              \
  const char var ## _name[] PROGMEM = name;             \
  const Menu::int_range_t var ## _menu PROGMEM = {      \
  {                                                     \
    Menu::INT_RANGE,                                    \
    (str_P) var ## _name                                \
  },                                                    \
  0,                                                    \
  1023,                                                 \
  &var.desired                                          \
};

/**
 * Compressor singleton. Takes care of controlling the compressor
 * and remembers which struts that turned it on.
 */
class Compressor {
  private:
    Compressor() {};
    static OutputPin compr;
    static uint8_t bitmap;  //!< Bitmap that remembers which struts that turned it on.
  public:
    static void on(uint8_t bm) { bit_set(bitmap, bm); compr.on(); };
    static void off(uint8_t bm) { bit_clear(bitmap, bm); if (!bitmap) compr.off(); };
};

OutputPin Compressor::compr = Board::D2;
uint8_t Compressor::bitmap = 0;

/**
 * Strut class. Manages desired value in eEPROM, reads actual value from
 * sensor and controls vent and compressor. Is based on Periodic and runs
 * every 2000 ms.
 *
 * @code
 * States:
 * Strut to low -> Start compressor and open vent.
 * Strut in desired level -> Turn off compressor and close vent.
 * Strut to high -> Open vent.
 * @endcode
 */
class Strut : public Periodic {
  private:
    /** Actual value */
    int16_t actual;
    /** Saved desired, from EEPROM */
    int16_t saveddesired;
    /** The smoothed value */
    int16_t smoothed;

    /** Level sensor pin */
    AnalogPin sensor;
    /** Vent output pin */
    OutputPin vent;

    /** Internal value, used for calculating EEPROM location and bitmap */
    static uint8_t location;

    /** Pointer to the EEPROM location */
    int16_t* eepromlocation;
    /** Bitmask position */
    uint8_t bm;
  public:
    /**
     * Constructor for strut.
     * @param[in] sensor AnalogPin for the sensor
     * @param[in] vent OutputPin for the vent
     * @param[in] ms time for the periodic run
     */
    Strut(AnalogPin _sensor, OutputPin _vent,
	  uint16_t ms = 2000) : Periodic(ms), sensor(_sensor), vent(_vent)
    {
      eepromlocation = (int16_t*)(location*sizeof(int16_t));
      bm = location;

      location++;

      is_leveled = false;
      GetDesired();
    };

    /** Variable for the menu system to change desired level */
    int16_t desired;
    /** Boolean state if it's leveled */
    bool is_leveled;

    /** @return the desired value stored in EEPROM */
    int16_t GetDesired()
    {
      eeprom.read<int16_t>(&desired, eepromlocation);
      saveddesired = desired;
      return desired;
    }


    /**
     *  Writes desired value to EEPROM
     *  @return the desired value stored in EEPROM, -1 if it fails
     */
    int16_t SetDesired()
    {
      if (saveddesired != desired) {
        eeprom.write<int16_t>(eepromlocation, &desired);
      }
      return desired;
    }

    /**
     * Writes desired value to EEPROM
     * @param[in] newdesired is the desired value to write
     * @return the desired value, -1 it write failes.
     */
    int16_t SetDesired(int16_t newdesired)
    {
      desired = newdesired;
      eeprom.write<int16_t>(eepromlocation, &desired);
      return desired;
    }

    /**
     * @return last read sensor reading
     */
    int16_t GetActual() { return actual; };

    /**
     * @override Periodic
     * The Strut periodic function; runs every 2000 ms, reads actuall value,
     * calculates a new smoothed value, decides if it's leveled and controlls
     * the compressor and wen't accordingly.
     *
     * Updates the EEPROM value if it has been changed from the menu.
     */
    virtual void run()
    {
      // Read
      actual = sensor.sample();

      // Calculate
      //smoothed = (ALPHA * actual + (POWER - ALPHA) * smoothed )/ POWER; // No float (didn't work)
      smoothed = AALPHA * actual + (1 - AALPHA) * smoothed;

      // Write

      SetDesired();
    }

};

uint8_t Strut::location = 0;

#endif
