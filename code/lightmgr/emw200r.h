/*------------------------------------------------------------------------------
 * File: emw200r.h
 * Created: September 24, 2015
 * Last changed: September 26, 2015
 *
 * Author(s): Philip Arvidsson (contact@philiparvidsson.com)
 *
 * Description:
 *   Provides functionality for transmatting radio frequency signals to control
 *   the EverFlourish EMW200R switches from Clas Ohlson.
 *----------------------------------------------------------------------------*/

#ifndef emw200r_h_
#define emw200r_h_

/*------------------------------------------------
 * INCLUDES
 *----------------------------------------------*/

#include <stdint.h>

/*------------------------------------------------
 * TYPES
 *----------------------------------------------*/

namespace emw200r {

class SwitchMgr {
  
public:

  SwitchMgr(int pin_no);

  void init();

  void turnOff(char group, int socket_no);
  void turnOn(char group, int socket_no);

private:

  void transmit(int num_high, int num_low);
  void transmit0();
  void transmit1();
  void transmitByte(uint8_t b);

  int m_pin_no;

};

}

#endif // emw200r_h_

