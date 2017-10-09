# iot-motion-detector

A $6 internet of things and easy to build motion detector.  

This implementation controls a Philips Hue light, but any IoT application can be implemented using the builtin WiFi adapter.

## Hardware
Name                                 | Type            | Price (USD) | Link
-------------------------------------|-----------------|-------------|-----
WeMos D1 mini V2.2.0 (ESP8266 board) | Microcontroller | 4.09        | [banggood.com](https://www.banggood.com/WeMos-D1-mini-V2_2_0-WIFI-Internet-Development-Board-Based-ESP8266-4MB-FLASH-ESP-12S-Chip-p-1143874.html)
HC-SR501                             | Motion detector | 1.95        | [banggood.com](https://www.banggood.com/HC-SR501-Human-Infrared-Sensor-Module-Including-Lens-p-972697.html)

Total cost: USD 6.03 (excluding shipping, recorded 2017-09-16)

### Required equipment
* Jumper wires
* Breadboard (prototyping)
* Soldering iron
* Micro USB cable (system power/ESP8266 programming)
* USB power adapter (system power)
* Computer (ESP8266 programming)

### Wiring
[TODO]

### Power Consumption
The values in this section are calculated, I do not have the required hardware to make measurements.

#### Standby
Sensing for motion, not sending/receiving over WiFi.

Component | State          | Power Draw (mA)
----------|----------------|----------------
ESP8266   | Modem-Sleep(³) | 15
HC-SR501  | On             |  0.065

Total: 15.065mA

#### Active
Sensing for motion, sending/receiving over WiFi.

Component | State         | Power Draw (mA)
----------|---------------|----------------
ESP8266   | Tx 802.11b(³) | 170
HC-SR501  | On            |   0.065

Total: 170.065mA

### Using Different Hardware
You can quite easily build this project with other ESP8266 boards and motion sensors other than the HC-SR501.
When doing so, watch out for the following possible differences (list not complete):

Different ESP8266 board:
- No USB connector (power delivery, programming)
- No 5V output (power delivery to HC-SR501)

Motion sensor other than HC-SR501:
- Required voltage (power delivery)

## Software
### Configuration
Name                        | Type     | Default Value              | Description
----------------------------|----------|----------------------------|------------
light_on_duration           | uint32_t | 60 * 1000 (60s)            | Time to keep the lights on in milliseconds
nighttime_start             | uint16_t | 23 * 60 (23:00)            | Start of nighttime in minutes since midnight
nighttime_duration          | uint16_t | 7 * 60 (07:00)             | Duration of nighttime in minutes
hue_ip                      | char*    | -                          | Local IP Address of the Hue Bridge
hue_port                    | uint16_t | 80                         | Port of the Hue Bridge API
hue_timeout                 | uint16_t | 10 * 1000 (10s)            | Timeout for requests to the Hue Bridge in milliseconds
hue_user_id                 | char*    | -                          | ID of the Hue Bridge user for authentication*
hue_light_id                | char*    | -                          | ID of the Hue light to control
hue_command_on_daytime      | char*    | {\"on\":true, \"bri\":254} | Command to turn the Hue light on during daytime
hue_command_on_nighttime    | char*    | {\"on\":true, \"bri\":1}   | Command to turn the Hue light on during nighttime
hue_command_off             | char*    | {\"on\":false}             | Command to turn the Hue light off
wifi_ssid                   | char*    | -                          | SSID of the WiFi network*
wifi_password               | char*    | -                          | Password for the Wifi network*
wifi_timeout                | uint16_t | 15 * 1000 (15s)            | Timeout for connecting to the WiFi network in milliseconds
ntp_host                    | char*    | pool.ntp.org               | Hostname of the NTP server
ntp_offset                  | int32_t  | 0                          | Offset from the UTC time in milliseconds
ntp_update_interval         | uint32_t | 24 * 60 * 60 * 1000 (24h)  | Interval of system clock synchronisation with NTP server
baud_rate                   | uint32_t | 115200                     | Baud rate for serial communication
pin_status_led              | uint8_t  | [LED_BUILTIN]              | Pin number of the status LED
pin_motion_sensor           | uint8_t  | 5                          | Pin number of the motion sensor (data pin)
motion_sensor_init_duration | uint32_t | 60 * 1000 (60s)            | Time until the motion sensor is ready for the first reading in milliseconds
debug                       | bool     | false                      | Output debug information

*Confidential information, keep private

### Libraries
- [ESP8266WiFi](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi)
- [NTPClient](https://github.com/arduino-libraries/NTPClient)

### Functionality
#### Status LED
The following states are communicated using the onboard LED:
- Initializing: LED is on, blinks once when it is done
- Shutdown: LED blinks 3 times long, then turns off
- Motion detected: LED is on
- No motion detected: LED is off

#### Initialization
After powering on the device gets everything ready for operation. 
This includes connecting to the WiFi network and waiting for the motion sensor initialization.
If any of these operations fail, shutdown is initiated. The shutdown reason is logged to the serial bus.
The amount of time required for initialization is usually bound by the configuration value of "motion_sensor_init_duration".

#### Shutdown
The device reduces power usage to a minimum. A power cycle is required to restart.
A shutdown can only occur during initialization (see section "Initialization").

#### Turning Light on
When motion is detected the light is turned on.

#### Turning Light off
When motion is not detected for the specified amount of time (configuration value "light_on_duration") the light is turned off.
When motion is detected again while the light is still on the timer is reset.

### Daytime/Nighttime
Different on-commands can be defined for daytime/nighttime (configuration values "hue_command_on_daytime" and "hue_command_on_nighttime").
When and how long nighttime is can be configured as well (configuration values "nighttime_start" and "nighttime_duration").

### NTP Time Synchronization
The device periodically (configuration value "ntp_update_interval") updates its internal clock with an NTP server (configuration value "ntp_host").
This is a blocking operation and can slightly delay turning the light on/off.

#### Serial Bus
Information about interrupts, actions and requests are logged to the serial interface (USB port).
Additional debug information can be turned on (configuration value "debug").

### Arduino IDE Board Settings
Board: WeMos D1 R2 & mini  
CPU Frequency: 80 MHz  
Flash Size: 4M (3M SPIFFS)  
Upload Speed: 57600

### Improvement Ideas
- Support for OTA updating of configuration values


## Development Process
Project goal: Building a motion detector that can turn on Philips Hue lightbulbs when entering a room.

### Hardware selection
Shipping prices and durations were calculated for Switzerland.

#### Motion detector
Selection criteria:
* <= 5$ cost (including shipping and microcontroller integration)
* &gt;= 2m range
* &gt;= 60 degrees horizontal sensing range
* <= 1s detection delay
* <= 4 weeks shipping time
* Easy to use with microcontroller (documentation/tutorials)
* Low power consumption

Evaluated options:
* HC-SR501

The HC-SR501 passive infrared motion sensor satisfies all criteria and seems to be a hobbyist favorite.

#### Microcontroller
Selection criteria:
* <= 15$ cost (including shipping and adding WiFi functionality)
* <= 4 weeks shipping time
* Easy to use with motion sensor (documentation/tutorials)
* Easy to use with WiFi (documentation/tutorials)
* Easy to power (documentation/tutorials)
* Low power consumption

Evaluated options:
* Genuino MKR1000
* Arduino Uno
* Raspberry Pi
* ESP8266

The ESP8266 is the only evaluated option that satisfies the cost criteria,
has integrated WiFi and seems to be a hobbyist favorite.

### Powering the System
The system should be able to be powered by a single power cable or batteries.  

The ESP8266 is powered by a micro USB cable. This enables the use of a USB power adaptor and battery packs.  
The HC-SR501 is powered directly with 5V by the ESP8266.

### HC-SR501 False Positives
The HC-SR501 initially fired a lot of false positives. Putting a 10k Ohm resistor in the data line seems to have solved this problem (see section "Wiring").


## References
1. Similar project: https://www.reddit.com/r/esp8266/comments/5l94gl/motion_control_hue_lights_with_esp8266_and_pir/

1. ESP8266 programming: https://hackaday.com/2015/03/18/how-to-directly-program-an-inexpensive-esp8266-wifi-module/
1. ESP8266 power consumption: http://bbs.espressif.com/viewtopic.php?t=133
1. ESP8266 power saving: https://github.com/esp8266/Arduino/issues/1381
1. ESP8266 programming with Arduino IDE: https://arduino-esp8266.readthedocs.io/en/latest/installing.html#instructions

1. HC-SR501 configuration: http://henrysbench.capnfatz.com/henrys-bench/arduino-sensors-and-input/arduino-hc-sr501-motion-sensor-tutorial/

1. Philips Hue API: https://developers.meethue.com/documentation/getting-started

All references were viewed on 2017-09-16.
