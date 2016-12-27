/*------------------------------------------------------------------------------
 * File: lightmgr.ino
 * Created: September 21, 2015
 * Last changed: September 26, 2015
 *
 * Author(s): Philip Arvidsson (contact@philiparvidsson.com)
 *
 * Description:
 *     Provides automatic management of lights in Philip's apartment! :-)
 *----------------------------------------------------------------------------*/

/*------------------------------------------------
 * INCLUDES
 *----------------------------------------------*/

#include "emw200r.h"
#include "telnetserver.h"

#include <Ethernet.h>

/*------------------------------------------------
 * CONSTANTS
 *----------------------------------------------*/

/*--------------------------------------
 * Constant: AuthString
 *
 * Description:
 *     Base-64 authentication string encoded from "<username>:<password>".
 *------------------------------------*/
#define AuthString ("YWRtaW46djRpOXc2WTVjeDdyMTJYZWFRVEg=")

/*--------------------------------------
 * Constant: DeviceConnected
 *
 * Description:
 *     Indicates that a device is connected.
 *------------------------------------*/
#define DeviceConnected (1)

/*--------------------------------------
 * Constant: DeviceDisconnected
 *
 * Description:
 *     Indicates that a device is disconnected.
 *------------------------------------*/
#define DeviceDisconnected (2)

/*--------------------------------------
 * Constant: PhilipPhoneMAC
 *
 * Description:
 *     MAC address of Philip's iPhone.
 *------------------------------------*/
#define PhilipPhoneMAC ("58:40:4e:c4:7a:5e")

/*--------------------------------------
 * Constant: RouterAttachedDevicesURL
 *
 * Description:
 *     URL to the webpage in the router's admin interface that lists the attached
 *     devices.
 *------------------------------------*/
#define RouterAttachedDevicesURL ("/DEV_show_device.htm")

/*--------------------------------------
 * Constant: RouterAttachedDevicesURL
 *
 * Description:
 *     URL to the webpage in the router's admin interface that lists the attached
 *     devices.
 *------------------------------------*/
#define RouterAdminPort (80)

/*--------------------------------------
 * Constant: RouterIP
 *
 * Description:
 *     Router's IP (used to connect to the web admin interface).
 *------------------------------------*/
#define RouterIP { 192, 168, 1, 1 }

/*--------------------------------------
 * Constant: TransmitPinNo
 *
 * Description:
 *     Index of the pin used for transmitting radio frequency signals to control
 *     the remote controlled EverFlourish EMW200R switches from Clas Ohlson.
 *------------------------------------*/
#define TransmitPinNo (9) // Pin used to send radio signals.

/*--------------------------------------
 * Constant: Unknown
 *
 * Description:
 *     Indicates that the device state is unknown.
 *------------------------------------*/
#define Unknown (0)

/*--------------------------------------
 * Constant: ValidResponseMinBytes
 *
 * Description:
 *     The minimum number of bytes in a response for it to be considered valid.
 *------------------------------------*/
#define ValidResponseMinBytes (1500)

/*------------------------------------------------
 * TYPES
 *----------------------------------------------*/

typedef struct timeT {
    int hour;
    int minute;
    int second;

    int weekend;

    int day;
    int month;
    int year;
} timeT;


/*------------------------------------------------
 * GLOBALS
 *----------------------------------------------*/

static timeT cur_time;
static timeT arrive_time;
static timeT leave_time;

static emw200r::SwitchMgr    switch_mgr = emw200r::SwitchMgr(TransmitPinNo);
static telnet ::TelnetServer telnet_srv = telnet ::TelnetServer(23);

static bool philip_is_home   = false;
static int  not_home_counter = 0;
static int  num_errors       = 0;
static int  num_warnings     = 0;

// bits = A1, A2, A3, B1, B2, B3, C1, C2, C3, D1, D2, D3 (high 4 bits of last 8 bits must be zero, ie B0000xxxx)
unsigned short arriveOn  = (B11111100 << 4) | B0000000;
unsigned short arriveOff = 0;
unsigned short leaveOn   = 0;
unsigned short leaveOff  = 0x00ffff; // everything off

/*------------------------------------------------
 * FUNCTIONS
 *----------------------------------------------*/

timeT parseTime(char* s) {
    timeT t = { 0 };

    // Date: Thu, 14 Apr 2016 21:01:07 GMT\r\n

    char* p = strstr(s, "Date: ");
    if (!p) return t;

    p += strlen("Date: ");
    if (strstr(p, "Sat") == p || strstr(p, "Sun") == p)
        t.weekend = 1;

    // Skip to day of month
    p = strstr(p, " ");
    if (!p) return t;
    p++;

    if (p[1] == ' ') {
        t.day = ((int)(p[1]-'0'));
    }
    else {
        t.day = ((int)(p[0]-'0'))*10+((int)(p[1]-'0'));
    }

    // Skip to month
    p = strstr(p, " ");
    if (!p) return t;
    p++;

    if (strstr(p, "Jan") == p) t.month = 1;
    if (strstr(p, "Feb") == p) t.month = 2;
    if (strstr(p, "Mar") == p) t.month = 3;
    if (strstr(p, "Apr") == p) t.month = 4;
    if (strstr(p, "May") == p) t.month = 5;
    if (strstr(p, "Jun") == p) t.month = 6;
    if (strstr(p, "Jul") == p) t.month = 7;
    if (strstr(p, "Aug") == p) t.month = 8;
    if (strstr(p, "Sep") == p) t.month = 9;
    if (strstr(p, "Oct") == p) t.month = 10;
    if (strstr(p, "Nov") == p) t.month = 11;
    if (strstr(p, "Dev") == p) t.month = 12;

    // Skip to year
    p = strstr(p, " ");
    if (!p) return t;
    p++;

    t.year = ((int)(p[0]-'0'))*1000 + ((int)(p[1]-'0'))*100 + ((int)(p[2]-'0'))*10 + ((int)(p[3]-'0'));

    // Skip to time
    p = strstr(p, " ");
    if (!p) return t;
    p++;

    t.hour   = ((int)(p[0]-'0'))*10+((int)(p[1]-'0'));
    t.minute = ((int)(p[3]-'0'))*10+((int)(p[4]-'0'));
    t.second = ((int)(p[6]-'0'))*10+((int)(p[7]-'0'));

    return t;
}

void authWNDR3700(EthernetClient& client) {
    client.print("GET ");
    client.print(RouterAttachedDevicesURL);
    client.println(" HTTP/1.0");
    client.print("Authorization: Basic ");
    client.println(AuthString);

    client.println("Connection: close");

    client.println("");
    client.println("");

    //client.flush();
 }

/*--------------------------------------
 * Function: isDeviceOnLAN(device_mac_addr, max_attempts)
 *
 * Parameters:
 *     device_mac_addr    MAC address to look for.
 *     max_attempts         The maximum number of attempts.
 *
 * Description:
 *     Connects to the router's web admin interface and checks if a device with
 *     the specified MAC address is connected.
 *
 * Usage:
 *     bool on_lan = isDeviceOnLAN("f0:d1:a9:af:46:eb", 10);
 *------------------------------------*/
int isDeviceOnLAN(const char* device_mac_addr, int max_attempts) {
    EthernetClient client;

    uint8_t router_ip[] = RouterIP;
    if (!client.connect(router_ip, RouterAdminPort)) {
        if (max_attempts > 1) {
            // Keep bugging that motherfucker. I hate my router.
            num_warnings++;
            delay(1000);
            return isDeviceOnLAN(device_mac_addr, max_attempts-1);
        }

        num_errors++;
        return (Unknown);
    }

    authWNDR3700(client);

    // Give the router a chance to respond to the request.

    int delay_millis = 0;
    while (!client.available()) {
        delay(1);
        delay_millis++;

        // No data received from that bitch.
        if (delay_millis > 5000) {
            if (max_attempts > 1) {
                // Keep bugging that motherfucker. I hate my router.
                num_warnings++;
                delay(1000);
                return isDeviceOnLAN(device_mac_addr, max_attempts-1);
            }

            num_errors++;
            return (Unknown);
        }
    }

    int num_bytes = 0;
    const char* mac_addr_ptr = device_mac_addr;
    static char buf[300] = { 0 };

    const char mac_addr_ptr_orig = mac_addr_ptr;
    while (client.available()) {
        char c = client.read();
        if (num_bytes < sizeof(buf)) {
            buf[num_bytes] = c;
        }

        num_bytes++;

        if ((c == *mac_addr_ptr)) {
            mac_addr_ptr++;
            if (!(*mac_addr_ptr)) {
                //setTime(t.hour, t.minute, t.second, t.day, t.month, t.year);
                cur_time = parseTime(buf);
                return (DeviceConnected);
            }
        }
        else {
            mac_addr_ptr = mac_addr_ptr_orig;
        }
    }

    if (num_bytes < ValidResponseMinBytes) {
        if (max_attempts > 1) {
            // Keep bugging that motherfucker. I hate my router.
            num_warnings++;
            delay(1000);
            return isDeviceOnLAN(device_mac_addr, max_attempts-1);
        }

        num_errors++;
        return (Unknown);
    }

    cur_time = parseTime(buf);
    return (DeviceDisconnected);
}

void cmdArriveSeq(telnet::TelnetServer& srv, char* args[], int num_args) {
    if (num_args == 0) {
        srv.respond("arrive sequence");
        srv.respond("\r\n");

        for (int i = 1; i <= 3; i++) {
            int m = 1<<(i-1);
            int a = (arriveOn >> 9) & m,
                b = (arriveOn >> 6) & m,
                c = (arriveOn >> 3) & m,
                d = (arriveOn >> 0) & m;

            if (a) { srv.respond("    "); srv.respond("a"); srv.respond(String(i).c_str()); srv.respond(" -> on"); srv.respond("\r\n"); }
            if (b) { srv.respond("    "); srv.respond("b"); srv.respond(String(i).c_str()); srv.respond(" -> on"); srv.respond("\r\n"); }
            if (c) { srv.respond("    "); srv.respond("c"); srv.respond(String(i).c_str()); srv.respond(" -> on"); srv.respond("\r\n"); }
            if (d) { srv.respond("    "); srv.respond("d"); srv.respond(String(i).c_str()); srv.respond(" -> on"); srv.respond("\r\n"); }
        }

        for (int i = 1; i <= 3; i++) {
            int m = 1<<(i-1);
            int a = (arriveOff >> 9) & m,
                b = (arriveOff >> 6) & m,
                c = (arriveOff >> 3) & m,
                d = (arriveOff >> 0) & m;

            if (a) { srv.respond("    "); srv.respond("a"); srv.respond(String(i).c_str()); srv.respond(" -> off"); srv.respond("\r\n"); }
            if (b) { srv.respond("    "); srv.respond("b"); srv.respond(String(i).c_str()); srv.respond(" -> off"); srv.respond("\r\n"); }
            if (c) { srv.respond("    "); srv.respond("c"); srv.respond(String(i).c_str()); srv.respond(" -> off"); srv.respond("\r\n"); }
            if (d) { srv.respond("    "); srv.respond("d"); srv.respond(String(i).c_str()); srv.respond(" -> off"); srv.respond("\r\n"); }
        }

        srv.respond("\r\n");
        srv.respond("\r\n");
        return;
    }

    if (num_args != 2) {
        srv.respond("usage: aseq [<on-bits> <off-bits>]");
        srv.respond("\r\n");
        srv.respond("\r\n");
        return;
    }

    if (strlen(args[0]) != 12 || strlen(args[1]) != 12) {
        srv.respond("both bit strings must be 12 chars long\r\n");
        return;
    }

    int aOn  = 0;
    int aOff = 0;

    for (int i = 0; i < 12; i++) {
        int onBit  = args[0][i] - '0';
        int offBit = args[1][i] - '0';

        aOn |= onBit << (11 - i);
        aOff |= offBit << (11 - i);
    }

    arriveOn = aOn;
    arriveOff = aOff;

    srv.respond("arrive sequence updated\r\n");
}

void cmdLeaveSeq(telnet::TelnetServer& srv, char* args[], int num_args) {
    if (num_args == 0) {
        srv.respond("leave sequence");
        srv.respond("\r\n");

        for (int i = 1; i <= 3; i++) {
            int m = 1<<(i-1);
            int a = (leaveOn >> 9) & m,
                b = (leaveOn >> 6) & m,
                c = (leaveOn >> 3) & m,
                d = (leaveOn >> 0) & m;

            if (a) { srv.respond("    "); srv.respond("a"); srv.respond(String(i).c_str()); srv.respond(" -> on"); srv.respond("\r\n"); }
            if (b) { srv.respond("    "); srv.respond("b"); srv.respond(String(i).c_str()); srv.respond(" -> on"); srv.respond("\r\n"); }
            if (c) { srv.respond("    "); srv.respond("c"); srv.respond(String(i).c_str()); srv.respond(" -> on"); srv.respond("\r\n"); }
            if (d) { srv.respond("    "); srv.respond("d"); srv.respond(String(i).c_str()); srv.respond(" -> on"); srv.respond("\r\n"); }
        }

        for (int i = 1; i <= 3; i++) {
            int m = 1<<(i-1);
            int a = (leaveOff >> 9) & m,
                b = (leaveOff >> 6) & m,
                c = (leaveOff >> 3) & m,
                d = (leaveOff >> 0) & m;

            if (a) { srv.respond("    "); srv.respond("a"); srv.respond(String(i).c_str()); srv.respond(" -> off"); srv.respond("\r\n"); }
            if (b) { srv.respond("    "); srv.respond("b"); srv.respond(String(i).c_str()); srv.respond(" -> off"); srv.respond("\r\n"); }
            if (c) { srv.respond("    "); srv.respond("c"); srv.respond(String(i).c_str()); srv.respond(" -> off"); srv.respond("\r\n"); }
            if (d) { srv.respond("    "); srv.respond("d"); srv.respond(String(i).c_str()); srv.respond(" -> off"); srv.respond("\r\n"); }
        }

        srv.respond("\r\n");
        srv.respond("\r\n");
        return;
    }

    if (num_args != 2) {
        srv.respond("usage: lseq [<on-bits> <off-bits>]");
        srv.respond("\r\n");
        srv.respond("\r\n");
        return;
    }

    if (strlen(args[0]) != 12 || strlen(args[1]) != 12) {
        srv.respond("both bit strings must be 12 chars long\r\n");
        return;
    }

    int lOn  = 0;
    int lOff = 0;

    for (int i = 0; i < 12; i++) {
        int onBit  = args[0][i] - '0';
        int offBit = args[1][i] - '0';

        lOn |= onBit << (11 - i);
        lOff |= offBit << (11 - i);
    }

    leaveOn = lOn;
    leaveOff = lOff;

    srv.respond("leave sequence updated\r\n");
}

void cmdGetReq(telnet::TelnetServer& srv, char* args[], int num_args) {
    EthernetClient client;

    uint8_t router_ip[] = RouterIP;
    if (!client.connect(router_ip, RouterAdminPort)) {
        srv.respond("\r\n");
        srv.respond("<eof>");
        srv.respond("\r\n");
        srv.respond("\r\n");
        return;
    }

    authWNDR3700(client);

    int num_bytes = 0;

    char s[2] = { 0 };
    while (client.connected()) {
        while (client.available()) {
            s[0] = client.read();

            if (s[0] <= 0) {
                client.stop();
                break;
            }

            num_bytes++;

            if (s[0] == '\n')
                srv.respond("\r");

            srv.respond(s);
        }
    }

    client.stop();

    srv.respond("\r\n");
    srv.respond(String(num_bytes).c_str());
    srv.respond(" bytes received.");
    srv.respond("\r\n");
}

void cmdOff(telnet::TelnetServer &srv, char* args[], int num_args) {
    if (num_args == 0) {
        for (int i = 1; i <= 3; i++) {
            switch_mgr.turnOff('A', i);
            switch_mgr.turnOff('B', i);
            switch_mgr.turnOff('C', i);
            switch_mgr.turnOff('D', i);
        }
    }
    else if (num_args == 2) {
        char group  = args[0][0];
        int  socket = (int)(args[1][0] - '0');
        switch_mgr.turnOff(group, socket);
    }
    else {
        srv.respond("usage: off [<group> <socket>]");
        srv.respond("\r\n");
        return;
    }

    srv.respond("lights off");
    srv.respond("\r\n");
}

void cmdOn(telnet::TelnetServer& srv, char* args[], int num_args) {
    if (num_args == 0) {
        for (int i = 1; i <= 3; i++) {
            switch_mgr.turnOn('A', i);
            switch_mgr.turnOn('B', i);
            switch_mgr.turnOn('C', i);
            switch_mgr.turnOn('D', i);
        }
    }
    else if (num_args == 2) {
        char group  = args[0][0];
        int  socket = (int)(args[1][0] - '0');
        switch_mgr.turnOn(group, socket);
    }
    else {
        srv.respond("usage: on [<group> <socket>]");
        srv.respond("\r\n");
        return;
    }

    srv.respond("lights on");
    srv.respond("\r\n");
}

void cmdStats(telnet::TelnetServer& srv, char* args[], int num_args) {
    srv.respond("statistics:");
    srv.respond("\r\n");

    srv.respond("  errors: ");
    srv.respond(String(num_errors).c_str());
    srv.respond("\r\n");

    srv.respond("  warnings: ");
    srv.respond(String(num_warnings).c_str());

    srv.respond("\r\n");
    srv.respond("  phil home? ");

    if (philip_is_home) {
        srv.respond("true");
        srv.respond("phil arrived at:");
        char tbuf[100] = {0};
        sprintf(tbuf, "%d-%d-%d %d:%d:%d\r\n", arrive_time.year, arrive_time.month, arrive_time.day, arrive_time.hour, arrive_time.minute, arrive_time.second);
        srv.respond(tbuf);
    }
    else {
        srv.respond("false\r\n");
        srv.respond("phil left at:");
        char tbuf[100] = {0};
        sprintf(tbuf, "%d-%d-%d %d:%d:%d\r\n", leave_time.year, leave_time.month, leave_time.day, leave_time.hour, leave_time.minute, leave_time.second);
        srv.respond(tbuf);
    }

    srv.respond("\r\n");

    srv.respond("time: ");

    char buf[100] = {0};
    sprintf(buf, "%d-%d-%d %d:%d:%d\r\n", cur_time.year, cur_time.month, cur_time.day, cur_time.hour, cur_time.minute, cur_time.second);
    srv.respond(buf);
    if (cur_time.weekend)
        srv.respond("      it is weekend today");

    srv.respond("\r\n");
}

/*--------------------------------------
 * Function: setup()
 *
 * Description:
 *     Initializes the Arduino Uno.
 *
 * Usage:
 *     ---
 *------------------------------------*/
void setup() {
    switch_mgr.init();

    // Hang if we can't get a DHCP lease.
    uint8_t mac_addr[] = { 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed };
    uint8_t ip_addr[]  = { 192, 168, 1, 7 };
    Ethernet.begin(mac_addr, ip_addr);

    delay(1000);

    telnet_srv.init();
    telnet_srv.onCommand("aseq"  , cmdArriveSeq);
    telnet_srv.onCommand("lseq"  , cmdLeaveSeq);
    telnet_srv.onCommand("getreq", cmdGetReq);
    telnet_srv.onCommand("off"   , cmdOff);
    telnet_srv.onCommand("on"    , cmdOn);
    telnet_srv.onCommand("stats" , cmdStats);

    delay(1000);
}

int timer = 0;

/*--------------------------------------
 * Function: loop()
 *
 * Description:
 *     Performs main processing - checking for devices on LAN etc.
 *
 * Usage:
 *     ---
 *------------------------------------*/
void loop() {
    delay(1000);

    //Ethernet.maintain();
    telnet_srv.update();

    timer += 1;

    if ((timer < 30) || (philip_is_home && (timer < 60*5)))
        return;

    timer = 0;

    int r = isDeviceOnLAN(PhilipPhoneMAC, 10);

    if (r == DeviceConnected) {
        not_home_counter = 0;

        if (!philip_is_home) {
            philip_is_home = true;
            arrive_time = cur_time;

            for (int i = 1; i <= 3; i++) {
                int m = 1<<(i-1);
                int a = (arriveOn >> 9) & m,
                    b = (arriveOn >> 6) & m,
                    c = (arriveOn >> 3) & m,
                    d = (arriveOn >> 0) & m;

                if (a) switch_mgr.turnOn('A', i);
                if (b) switch_mgr.turnOn('B', i);
                if (c) switch_mgr.turnOn('C', i);
                if (d) switch_mgr.turnOn('D', i);
            }

            for (int i = 1; i <= 3; i++) {
                int m = 1<<(i-1);
                int a = (arriveOff >> 9) & m,
                    b = (arriveOff >> 6) & m,
                    c = (arriveOff >> 3) & m,
                    d = (arriveOff >> 0) & m;

                if (a) switch_mgr.turnOff('A', i);
                if (b) switch_mgr.turnOff('B', i);
                if (c) switch_mgr.turnOff('C', i);
                if (d) switch_mgr.turnOff('D', i);
            }
        }
    }
    else if (r == DeviceDisconnected) {
        if (philip_is_home) {
            not_home_counter++;

            if (not_home_counter >= 3) {
                philip_is_home = false;
                leave_time = cur_time;

                for (int i = 1; i <= 3; i++) {
                    int m = 1<<(i-1);
                    int a = (leaveOn >> 9) & m,
                        b = (leaveOn >> 6) & m,
                        c = (leaveOn >> 3) & m,
                        d = (leaveOn >> 0) & m;

                    if (a) switch_mgr.turnOn('A', i);
                    if (b) switch_mgr.turnOn('B', i);
                    if (c) switch_mgr.turnOn('C', i);
                    if (d) switch_mgr.turnOn('D', i);
                }

                for (int i = 1; i <= 3; i++) {
                    int m = 1<<(i-1);
                    int a = (leaveOff >> 9) & m,
                        b = (leaveOff >> 6) & m,
                        c = (leaveOff >> 3) & m,
                        d = (leaveOff >> 0) & m;

                    if (a) switch_mgr.turnOff('A', i);
                    if (b) switch_mgr.turnOff('B', i);
                    if (c) switch_mgr.turnOff('C', i);
                    if (d) switch_mgr.turnOff('D', i);
                }
            }
        }
    }
    else {
        // We don't know where Philip is, so do nothing.
    }
}
