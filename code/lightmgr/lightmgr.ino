/*------------------------------------------------------------------------------
 * File: lightmgr.ino
 * Created: September 21, 2015
 * Last changed: September 26, 2015
 *
 * Author(s): Philip Arvidsson (contact@philiparvidsson.com)
 *
 * Description:
 *   Provides automatic management of lights in Philip's apartment! :-)
 *----------------------------------------------------------------------------*/

/*------------------------------------------------
 * INCLUDES
 *----------------------------------------------*/

#include "emw200r.h"
#include "memorylog.h"
#include "telnetserver.h"

#include <Ethernet.h>
#include <SPI.h>

/*------------------------------------------------
 * CONSTANTS
 *----------------------------------------------*/

/*--------------------------------------
 * Constant: AuthString
 *
 * Description:
 *   Base-64 authentication string encoded from "<username>:<password>".
 *------------------------------------*/
#define AuthString ("<removed>")

#define DeviceConnected (1)
#define DeviceDisconnected (2)

#define PhilipPhoneMAC ("f0:d1:a9:af:46:eb")

/*--------------------------------------
 * Constant: RouterAttachedDevicesURL
 *
 * Description:
 *   URL to the webpage in the router's admin interface that lists the attached
 *   devices.
 *------------------------------------*/
#define RouterAttachedDevicesURL ("/DEV_show_device.htm")

/*--------------------------------------
 * Constant: RouterAttachedDevicesURL
 *
 * Description:
 *   URL to the webpage in the router's admin interface that lists the attached
 *   devices.
 *------------------------------------*/
#define RouterAdminPort (80)

#define RouterIP { 192, 168, 1, 1 }

/*--------------------------------------
 * Constant: TransmitPinNo
 *
 * Description:
 *   Index of the pin used for transmitting radio frequency signals to control
 *   the remote controlled EverFlourish EMW200R switches from Clas Ohlson.
 *------------------------------------*/
#define TransmitPinNo (9) // Pin used to send radio signals.
#define Unknown (0)

/*------------------------------------------------
 * GLOBALS
 *----------------------------------------------*/

static const uint8_t mac_addr [] = { 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed };

static emw200r::SwitchMgr    switch_mgr = emw200r::SwitchMgr(TransmitPinNo);
static telnet ::TelnetServer telnet_srv = telnet ::TelnetServer(23);
static logging::MemoryLog    mem_log    = logging::MemoryLog();

static bool philip_is_home   = false;
static int  not_home_counter = 0;

/*------------------------------------------------
 * FUNCTIONS
 *----------------------------------------------*/

 void authWNDR3700(EthernetClient& client) {
  client.print("GET ");
  client.print(RouterAttachedDevicesURL);
  client.println(" HTTP/1.1");
  client.print("Authorization: Basic ");
  client.println(AuthString);
  client.println("Connection: close");
  client.println("");
  client.println("");
  client.flush();
 }

/*--------------------------------------
 * Function: isDeviceOnLAN(device_mac_addr)
 *
 * Parameters:
 *   device_mac_addr  MAC address to look for.
 *
 * Description:
 *   Connects to the router's web admin interface and checks if a device with
 *   the specified MAC address is connected.
 *
 * Usage:
 *   bool on_lan = isDeviceOnLAN("f0:d1:a9:af:46:eb");
 *------------------------------------*/
int isDeviceOnLAN(const char* device_mac_addr) {
  EthernetClient client;

  uint8_t router_ip[] = RouterIP;
  if (!client.connect(router_ip, RouterAdminPort)) {
    mem_log.error("could not connect to router");
    return (Unknown);
  }

  authWNDR3700(client);

  // Give the router a chance to respond to the request.
  delay(500);

  if (!client.available()) {
    mem_log.error("no data received from router");
    return (Unknown);
  }

  while (client.available()) {
    char c = client.read();

    if ((c == *device_mac_addr) && (!(*(++device_mac_addr))))
      return (DeviceConnected);

    delay(1);
  }

  return (DeviceDisconnected);
}

void cmdGetReq(telnet::TelnetServer& srv, const char* args[]) {
  EthernetClient client;

  uint8_t router_ip[] = RouterIP;
  if (!client.connect(router_ip, RouterAdminPort)) {
    mem_log.error("could not connect to router");
    srv.respond("\r\n<eof>\r\n\n");
    return;
  }

  authWNDR3700(client);

  // Give the router a chance to respond to the request.
  delay(500);

  if (!client.available()) {
    mem_log.error("no data received from router");
    srv.respond("\r\n<eof>\r\n\n");
    return;
  }

  int num_bytes = 0;

  char s[2] = { 0 };
  while (client.available()) {
    s[0] = client.read();
    num_bytes++;

    if (s[0] == '\n')
      srv.respond("\r");

    srv.respond(s);

    delay(1);
  }

  srv.respond("\r\n<eof>\r\n\n");
  srv.respond(String(num_bytes).c_str());
  srv.respond(" bytes received.\r\n");
}

void cmdShowLog(telnet::TelnetServer& srv, const char* args[]) {
  srv.respond(mem_log.getText());
  srv.respond("\r\n<eof>\r\n\n");
}

/*--------------------------------------
 * Function: setup()
 *
 * Description:
 *   Initializes the Arduino Uno.
 *
 * Usage:
 *   ---
 *------------------------------------*/
void setup() {
  switch_mgr.init();

  telnet_srv.init();
  telnet_srv.onCommand("getreq" , cmdGetReq);
  telnet_srv.onCommand("showlog", cmdShowLog);

  // Hang if we can't get a DHCP lease.
  if (!Ethernet.begin((uint8_t*)mac_addr))
    while (true);

  mem_log.info("up and running");

  delay(1000);
}

int timer = 0;

/*--------------------------------------
 * Function: loop()
 *
 * Description:
 *   Performs main processing - checking for devices on LAN etc.
 *
 * Usage:
 *   ---
 *------------------------------------*/
void loop() {
  delay(1000);

  Ethernet.maintain();
  telnet_srv.update();

  timer += 1;

  if ((timer < 60) || (philip_is_home && (timer < 60*7)))
    return;

  timer = 0;

  int r = isDeviceOnLAN(PhilipPhoneMAC);

  if (r == DeviceConnected) {
    not_home_counter = 0;

    if (!philip_is_home) {
      philip_is_home   = true;

      switch_mgr.turnOn('A', 2);
      switch_mgr.turnOn('A', 3);

      mem_log.info("turned lights on");
    }
  }
  else if (r == DeviceDisconnected) {
    if (philip_is_home) {
      not_home_counter++;

      if (not_home_counter >= 3) {
        philip_is_home = false;

        switch_mgr.turnOff('A', 1);
        switch_mgr.turnOff('A', 2);
        switch_mgr.turnOff('A', 3);

        mem_log.info("turned lights off");
      }
    }
  }
  else {
    // We don't know where Philip is, so do nothing.
    mem_log.warn("could not determine whether philip is home");
  }
}

