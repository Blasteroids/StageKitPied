# StageKitPied
Rock Band Stage Kit Pied is a highly configurable interface that runs on the Raspberry Pi platform.  It sits between an xbox 360 and the Rock Band Stage Kit (released by PDP) to read the lighting cues from the Harmonix games "Rock Band 2" and "Rock Band 3", which it then uses to switch on & off LEDs in a large LED strip array.

UPDATE 25/04/2023 : No Stage Kit?  Well that's not a problem if you're using RB3Enhanced (https://github.com/RBEnhanced/RB3Enhanced)  To be able to use the RB3Enhanced data please read the RB3Enhanced section at the bottom of this readme.  Thanks go out to the RB3Enhanced devs for highlighting the relevent data they can send out over UDP.

## Examples of it in action
[![Example 1](https://img.youtube.com/vi/fq0_RiIjsV8/0.jpg)](https://www.youtube.com/watch?v=fq0_RiIjsV8)  [![Example 2](https://img.youtube.com/vi/q-61C9YkRUw/0.jpg)](https://www.youtube.com/watch?v=q-61C9YkRUw)

# WARNING
**You attempt any of this at your own risk.  Incorrectly wiring and powering electronics can result in fires or even worse.**

**You have been warned!**

## Build the LED Array
###### Hardware
Rock Band Stage Kit - Released by PDP.  Just the light POD on it's own will do.

Raspberry Pi - Version 1 should be enough, default Raspbian OS.

SK9822 LEDs - I'm using 60 per M but any configuration should be ok.

PSU - The SK9822 LEDs are 0.06amp per segment (each segment has 3 leds @ 0.02amp).  So 70 segments is 70 x 0.06 = 4.2amp.

###### Wiring it up

**Ensure you use the correct fuse ratings on the LED strips!**

Multiple strips can be joined together using the data & clock channels, then feed each strip with it's own power.

Example, I use 4 strips.  Each strip has it's own fuse and PSU connection.
 - 2 strips x 70 segments = 2 x 4.2amp = 5 amp fuses.
 - 2 strips x 40 segments = 2 x 2.4amp = 3 amp fuses.
 
**Ensure you use correct AWG rated wire for your power requirements.**

Connect the SK9822...
 - GND : Ensure it's to the Ground on the PSU, the Raspberry Pi should also use the same ground.
 - C(lock) : SPI SCLK (GPIO 11) on the Raspberry Pi.
 - D(ata) : SPI MOSI (GPIO 10) on the Raspberry Pi.
 - 5V : Positive output on PSU.
 
## Build the adapter
###### Hardware
Left: Serial Adapter = FTDI-FT232RL     Right: Pro Micro = ATMEGA32U4 5V 16MHz.

![FTDI-FT232RL](https://user-images.githubusercontent.com/127441225/224138326-7562e701-adcd-4776-a003-dd04618f61b9.PNG)  ![ProMicro-ATMEGA32U4](https://user-images.githubusercontent.com/127441225/224138343-69b9a5ba-e82e-4e15-a11f-e3460c5fc5dc.PNG)

If selectable voltage, then ensure the jumper is set to 5V (Green box in pic)

## Connection
 - Connect the Serial Adapter GROUND to Pro Micro GROUND (Grey box in pics)
 - Connect the Serial Adapter TX to Pro Micro RX1 (Purple box in pics)
 - Connect the Serial Adapter RX to Pro Micro TX0 (Orange box in pics)

## Installing the Firmware
###### Install GIMX
Thankfully GIMX 8.0 makes this bit super easy :)

Get it from here: https://github.com/matlo/GIMX/releases/tag/v8.0

In the install directory firmware folder
> Windows Default = "C:\Program Files\GIMX\firmware"

Either compile the firmware yourself, or use this compiled firmware file found in the gimx-adapter folder
> atmega32u4.hex


- Connect the Serial Adapter (FTDI-FT232RL) to the PC via USB.
- Open GIMX
- Click "Help" -> "Update Firmware"
- Select "atmega32u4.hex" then click "Load"
- Follow the instructions to load the firmware onto the Pro Micro.

## Setting up the StageKitPied software
###### Installing the software
Either compile from the source or copy the 'skp' & ini files from the 'StageKitPied' folder.

Place them into any folder you want to use on the Raspberry Pi, just ensure the ini files are in the same directory as the program.

###### Edit the lights.ini file
In the [LEDS] section
 - Enter in the amount of LEDS you have in the LED_AMOUNT=xxx
 - Enter in the INI_DEFAULT=x for the LED settings ini file you want.  Included is 5 examples.
 
In the [STAGEKIT] section, you can enable pass-through to the POD for the following items :-
 - Xbox LED Status
 - Stage Kit POD lights
 - Stage Kit Fog
 - Stage Kit Strobe
 
If you want the POD to go dark, set those to 0 and then there's no needs to have the FOG/Strobe unit out.

There's other settings but the other defaults should be ok for most.

###### Edit the leds(x).ini
[SK_COLOURS]
 - Stores the basic led colour values for RED, GREEN, BLUE, YELLOW & STROBE.

[RED_GROUP_X] [GREEN_GROUP_X] [BLUE_GROUP_X] [YELLOW_GROUP_X]
There are 8 sections for each of these, where X corresponds to the 8 colour leds on the actual Stage Kit POD.
 - BRIGHTNESS=xx : How bright do want these?  Values are 0 (off) to 15 (max)
 - AMOUNT=xx : The amount of leds that are in this grouping.
 - LEDS=xx,xx,xx : Comma seperated led numbers that are in this group.

[STROBE]
 - BRIGHTNESS=xx : How bright do want the strobe?  Values are 0 (off) to 15 (max)
 - LEDS_ALL=0 : Set this to 1 for the strobe to use every led.
 - LEDS_AUTO=0 : Set this to 1 and the program will work out which leds are not assigned to the colours and use them for strobe.
 - AMOUNT=xx : If manually setting the leds to use as strobe, enter how many there are.
 - LEDS=xx,xx : Comma seperated led numbers that are to be used for the strobe.

## Connecting everything
 - Connect the Pro Micro USB side to the X360.
 - Connect the Serial Adapter USB side to the Raspberry Pi.
 - Connect the Stage Kit light POD to the Raspberry Pi.
 - Run the Stage Kit Pied program with the command
   > sudo ./skp
 Note: root is required to access the USB PDP Stage Kit device.

## Notes
The StageKitPied program needs root access to be able to use the USB ports.

The Light Show does not work in practice mode.

The Stage Kit light POD will not show LEDS unless it has power via the PS/2 port by either :-
 - A PS/2 to USB cable connected to a minimum 1amp power source.
 - Using the Fog/Strobe unit.
 
The reason for all the different led INI files was due to an earlier version that allowed switching on the fly.
 - This probably won't be added back.

## Known Issues
  The StageKitPied program will generate warnings from the serial adapter, these can be surpressed in the lights.ini.
  
  The StageKitPied program will generate warnings from the USB connection, these are very infrequent & can be ignored.

## RB3Enhanced
  To run with RB3Enhanced, then a PDP Stage Kit is not required and the above adapter is also not required.
  Create the LED Array as mentioned above.
  Edit the lights.ini to enable RB3E mode.
   [RB3E]
    - ENABLED=1 : Set this to 1 to make the program listen for the RB3Enhanced data stream.
    - SOURCE_IP=0.0.0.0 : Leave this as 0.0.0.0 to listen out for any IP on your, or set it to the IP of the X360.
    - LISTENING_PORT=21070 : Default port that RB3Enhanced will send to. 
  Edit the RB3Enhanced rb3.ini section
   [Events]
    - EnableEvents = true : Set this to true for events to be sent over the network.
    - BroadcastTarget = 255.255.255.255 : This is broadcast to all IP on your network.  If you know your raspberry pi IP then you can set this here.
    
  Ensure that all the StageKitPied ini files are in the same folder as the skp program and then run it
    > ./skp
  Note: Root is not required.
  
