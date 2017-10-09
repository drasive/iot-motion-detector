/*
  IoT Motion Detector v0.1
  A $6 internet of things and easy to build motion detector.

  See iot-motion-detector_implementation.ino for implementation.

  License:      MIT
  Repository:   https://github.com/drasive/iot-motion-detector
  Author:       Dimitri Vranken <me@dimitrivranken.com>
*/


// ===== Includes =====
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


// ===== Configuration =====
const uint32_t c_light_on_duration = 30 * 1000;              // Time to keep the lights on in milliseconds

const char* c_hue_ip = "TO_BE_SET";                          // Local IP Address of the Hue Bridge
const uint16_t c_hue_port = 80;                              // Port of the Hue Bridge API
const uint16_t c_hue_timeout = 10 * 1000;                    // Timeout for requests to the Hue Bridge in milliseconds
const char* c_hue_user_id = "TO_BE_SET";                     // ID of the Hue Bridge user for authentication
const char* c_hue_light_id = "1";                            // ID of the Hue light to control
const char* c_hue_command_on = "{\"on\":true, \"bri\":150}"; // Command to turn the Hue light on
const char* c_hue_command_off = "{\"on\":false}";            // Command to turn the Hue light off

const char* c_wifi_ssid = "TO_BE_SET";                       // SSID of the WiFi network
const char* c_wifi_password = "TO_BE_SET";                   // Password for the Wifi network
const uint16_t c_wifi_timeout = 15 * 1000;                   // Timeout for connecting to the WiFi network in milliseconds

const char* c_ntp_host = "pool.ntp.org";                     // Hostname of the NTP server
const int32_t c_ntp_offset = 0 * 60 * 60 * 1000;             // Offset from the UCT time in milliseconds
const uint32_t c_ntp_update_interval = 10 * 1000;            // Interval of NTP server synchronisation

const uint32_t c_baud_rate = 115200;                         // Baud rate for serial communication
const uint8_t c_pin_status_led = LED_BUILTIN;                // Pin number of the status LED
const uint8_t c_pin_motion_sensor = 5;                       // Pin number of the motion sensor (data pin)
const uint32_t c_motion_sensor_init_duration = 60 * 1000;    // Time until the motion sensor is ready for the first reading in milliseconds
const bool c_debug = false;                                  // Output debug information
