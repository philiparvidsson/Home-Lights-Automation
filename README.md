## Automation of Household Electronics

<img align="right" alt="" src="https://raw.githubusercontent.com/philiparvidsson/phil-phone-home/master/photos/DSC_9461.jpg" width="280px" height="280px" />

This project was an attempt to make my home more intelligent. The idea was to use an Arduino UNO to detect whether my iPhone is at home, and use that information to manage lamps and such.

The first step was to assemble a chip that can detect whether my iPhone is in my apartment or not. My initial idea was to use a wifi-chip and a servo to move switches from on to off and back. I quickly abandoned that idea due to multiple issues: wifi requires complex authentication, switches wear out and mechanical solutions are pretty unreliable. Instead, I decided to use a wired ethernet connection to my router, and replace the servo with a 433MHz radio able to control my remote controlled AC adapters.

Code for this project can be found in the code-folder, with a simple class for managing the EverFlourish EMW200R switches from Clas Ohlson.

## Video

[![Controlling lights manually](https://img.youtube.com/vi/anmDG1T86Gg/0.jpg)](https://youtu.be/anmDG1T86Gg)

The video shows me controlling a couple of lamps at home via the Arduino telnet server that I implemented as a network interface for the system. The more interesting part of the project—automatic management of household electronics; the fact that all electronics in my apartment are turned off when I leave my home—is hard to illustrate with a video.

#### Hardware:

| Piece          | Roll                      |
| -------------- | ------------------------- |
| Arduino UNO    | Main board                |
| Funduino W5100 | Ethernet LAN connectivity |
| XD-FST         | Radio freq. transmission  |

#### Encountered Issues:

##### iOS WLAN "sleep mode" (WOWLAN)

The iPhone WLAN chip is shut down shortly after the phone is locked. Actually, it's not entirely shut down, but rather enters WOWLAN-mode (wake on wireless lan), so the router still keeps it in its list of connected devices. It won't enumerate on the network, but if I log in to the router's management console, my iPhone can be seen in the list of attached devices, though without an IP address (only a MAC address).

The obvious solution is to have the Arduino-chip log in to the management console and look through the contents of it, searching for the phone's MAC. Turned out to work decently.

##### Old and tired WNDR3700 router

I've had issues with my WNDR3700 throughout the years. A couple of freezes, random reboots, web interface hangs, inability to update device lists etc. The issues have become more frequent over time, so I'm guessing it's got a screw loose somewhere. Either way, an issue that got in the way of this project was that the router would randomly not respond (or send an empty response) to the Arduino when it requested the device list. I came up with an easy fix: Determine my status (home or not) multiple times and require they all come up negative, to be sure. Ie, finding that I'm not home once doesn't result in it turning the lights of. Finding that I'm not home three times in a row, does. Easy fix, so I'm content.

##### iOS WOWLAN does not reconnect to Wi-Fi

Coming back to my apartment with the phone in the pocket, in WOWLAN-mode, it does not reconnect automatically to the wireless network. Easy fix: Just press the home button once and it immediately wakes up and reconnects. Can easily be done with phone in pocket.

##### Weak RF signals

The radio signals were a bit too weak and some units did not receive them clearly enough to be able to act on them. I took 20cm of speaker wire (copper) and mounted in the antenna connection on the XD-FST, which boosted the signal strength enough to easily reach everywhere in my apartment.

## Current state of the project:

I received the radio transmitter and researched the signals required to manipulate the state of my switches. The project is, in essence, complete.

The lights are now turned off ~21 minutes after I leave my apartment, and turned on again within 60 seconds of me coming back.

### Plans:

* ~~Disassembling the EMW200R switches and mounting them in ceiling lights. Shouldn't be very hard, haven't looked into it.~~ 
*Completed May 2016*
* ~~Improving the telnet server with some statistics command for uptime, error rate, etc.~~ *Completed May 2016*
* 3D-print a case for the device.
