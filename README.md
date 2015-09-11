## Is Philip's phone at home?

This project is an attempt to make my home more intelligent. The idea is to use an Arduino UNO to detect whether my iPhone is at home, and use that information to manage lamps and such.

The first step is to assemble a chip that can detect whether my iPhone is in my apartment or not. My initial idea was to use a wifi-chip and a servo to move switches from on to off and back. I quickly abandoned that idea due to multiple issues: wifi requires complex authentication, switches wear out and mechanical solutions are pretty unreliable. Instead, I decided to use a wired ethernet connection to my router, and replace the servo with a 433MHz radio able to control my remote controlled AC adapters.

#### Hardware:

| Piece          | Roll                      |
| -------------- | ------------------------- |
| Arduino UNO    | Main board                |
| Funduino W5100 | Ethernet LAN connectivity |

#### Encountered Issues:

##### iOS WLAN "sleep mode" (WOWLAN)

The iPhone WLAN chip is shut down shortly after the phone is locked. Actually, it's not entirely shut down, but rather enters WOWLAN-mode (wake on wireless lan), so the router still keeps it in its list of connected devices. It won't enumerate on the network, but if I log in to the router's management console, my iPhone can be seen in the list of attached devices, though without an IP address (only a MAC address).

The obvious solution is to have the Arduino-chip log in to the management console and look through the contents of it, searching for the phone's MAC. Turned out to work decently.

#### Current state of the project:

The Arduino now logs in to the router and detects whether my phone is at home or not. The radio transmitter has been ordered and should arrive within a couple of weeks. My current plan is to record a signal from the RF remote that controls the remote controlled AC adapters, save it, and then play it back when needed to turn them on or off.
