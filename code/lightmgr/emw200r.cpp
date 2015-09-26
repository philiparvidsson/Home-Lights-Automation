/*------------------------------------------------------------------------------
 * File: emw200r.cpp
 * Created: September 24, 2015
 * Last changed: September 26, 2015
 *
 * Author(s): Philip Arvidsson (contact@philiparvidsson.com)
 *
 * Description:
 *   Provides functionality for transmatting radio frequency signals to control
 *   the EverFlourish EMW200R switches from Clas Ohlson.
 *----------------------------------------------------------------------------*/

/*------------------------------------------------
 * INCLUDES
 *----------------------------------------------*/

#include "emw200r.h"

#include <Arduino.h>

#include <stdint.h>

/*------------------------------------------------
 * CONSTANTS
 *----------------------------------------------*/

#define NumTransmits (10)
#define PulseWidthMicrosecs (350)

/*------------------------------------------------
 * FUNCTIONS
 *----------------------------------------------*/

namespace emw200r {

SwitchMgr::SwitchMgr(int pin_no) : m_pin_no(pin_no) {
  // ...
}

void SwitchMgr::init() {
  pinMode(m_pin_no, OUTPUT);
}

void SwitchMgr::turnOff(char group, int socket_no) {
  // Three bytes make up the 24 bits: 010101010101010101010100
  uint8_t b0 = 0x55; 
  uint8_t b1 = 0x55;
  uint8_t b2 = 0x54;

  switch (group) {

  case 'a':
  case 'A':
    b0 &= 0x15;
    break;

  case 'b':
  case 'B':
    b0 &= 0x45;
    break;

  case 'c':
  case 'C':
    b0 &= 0x51;
    break;

  case 'd':
  case 'D':
    b0 &= 0x54;
    break;
    
  }

  switch (socket_no) {

  case 1:
    b1 &= 0x15;
    break;

  case 2:
    b1 &= 0x45;
    break;

  case 3:
    b1 &= 0x51;
    break;
    
  }
  
  int i = 0;
  for (i = 0; i < NumTransmits; i++) {    
    transmit(1, 31);
    
    transmitByte(b0);
    transmitByte(b1);
    transmitByte(b2);
  }
}

void SwitchMgr::turnOn(char group, int socket_no) {
  // Three bytes make up the 24 bits: 010101010101010101010111
  uint8_t b0 = 0x55; 
  uint8_t b1 = 0x55;
  uint8_t b2 = 0x57;

  switch (group) {

  case 'a':
  case 'A':
    b0 &= 0x15;
    break;

  case 'b':
  case 'B':
    b0 &= 0x45;
    break;

  case 'c':
  case 'C':
    b0 &= 0x51;
    break;

  case 'd':
  case 'D':
    b0 &= 0x54;
    break;
    
  }

  switch (socket_no) {

  case 1:
    b1 &= 0x15;
    break;

  case 2:
    b1 &= 0x45;
    break;

  case 3:
    b1 &= 0x51;
    break;
    
  }
  
  int i = 0;
  for (i = 0; i < NumTransmits; i++) {    
    transmit(1, 31);
    
    transmitByte(b0);
    transmitByte(b1);
    transmitByte(b2);
  }
}

void SwitchMgr::transmit(int num_high, int num_low) {
  if (num_high > 0) {
    digitalWrite(m_pin_no, HIGH);
    delayMicroseconds(num_high * PulseWidthMicrosecs);
  }
  
  
  if (num_low > 0) {
    digitalWrite(m_pin_no, LOW);
    delayMicroseconds(num_low * PulseWidthMicrosecs);
  }
}

void SwitchMgr::transmit0(void) {
  // One high followed by three lows for a zero-bit.
  transmit(1, 3);
}

void SwitchMgr::transmit1(void) {
  // Three high followed by one low for a one-bit.
  transmit(3, 1);
}

void SwitchMgr::transmitByte(uint8_t b) {
  int i;
  for (i = 7; i >= 0; i--) {
    int r = (b>>i) & 1;
    
    if (r) transmit1();
    else   transmit0();
  }
}

}

