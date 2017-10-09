/*
  IoT Motion Detector v0.1
  A $6 internet of things and easy to build motion detector.

  License:      MIT
  Repository:   https://github.com/drasive/iot-motion-detector
  Author:       Dimitri Vranken <me@dimitrivranken.com>
*/

#include <ESP8266WiFi.h>


// ===== Configuration =====
const uint32_t c_light_on_duration = 30 * 1000;              // Time to keep the lights on in milliseconds

const char* c_hue_ip = "";                                   // Local IP Address of the Hue Bridge
const uint16_t c_hue_port = 80;                              // Port of the Hue Bridge API
const uint16_t c_hue_timeout = 10 * 1000;                    // Timeout for requests to the Hue Bridge in milliseconds
const char* c_hue_user_id = "";                              // ID of the Hue Bridge user for authentication
const char* c_hue_light_id = "1";                            // ID of the Hue light to control
const char* c_hue_command_on = "{\"on\":true, \"bri\":150}"; // Command to turn the Hue light on
const char* c_hue_command_off = "{\"on\":false}";            // Command to turn the Hue light off

const char* c_wifi_ssid = "";                                // SSID of the WiFi network
const char* c_wifi_password = "";                            // Password for the Wifi network
const uint16_t c_wifi_timeout = 15 * 1000;                   // Timeout for connecting to the WiFi network in milliseconds

// TODO: Implement NTP
const char* c_ntp_host = "pool.ntp.org";                     // Hostname of the NTP server
const uint16_t c_ntp_port = 123;                             // Port of the NTP server
const uint16_t c_ntp_timeout = 15 * 1000;                    // Timeout for requests to the NTP server
const uint32_t c_ntp_update_interval = 24 * 60 * 60 * 1000;  // Update interval of time synchronisation (not yet implemented)

const uint32_t c_baud_rate = 115200;                         // Baud rate for serial communication
const uint8_t c_pin_status_led = LED_BUILTIN;                // Pin number of the status LED
const uint8_t c_pin_motion_sensor = 5;                       // Pin number of the motion sensor (data pin)
const uint32_t motion_sensor_init_duration = 60 * 1000;      // Time until the motion sensor is ready for the first reading in milliseconds
const bool c_debug = false;                                  // Output debug information


// ===== Program =====
volatile uint32_t g_last_motion_detected_time = 0;
uint32_t g_light_last_turned_on_time = 0;
uint32_t g_time_last_update_time = 0;


void setup() {
  // Initialize
  pinMode(c_pin_status_led, OUTPUT);
  digitalWrite(c_pin_status_led, LOW);
  uint32_t boot_time = millis();

  Serial.begin(c_baud_rate);
  Serial.println();
  Serial.println(F("===== IoT Motion Detector v0.1 ====="));
  Serial.println(F("https://github.com/drasive/iot-motion-detector"));
  Serial.println(F("Dimitri Vranken <me@dimitrivranken.com>"));

  // Connect to WiFi network
  // TODO: Reconnect if needed
  Serial.println();
  if (!wifi_connect(c_wifi_ssid, c_wifi_password)) {
    shut_down("Failed to connect to WiFi during setup");
  }

  // TOOD: Update time
  //shut_down("Failed to get NTP time during setup");

  // Wait for motion sensor initialization
  Serial.println();
  Serial.println("Waiting for motion sensor initialization");
  while (millis() - boot_time <= c_motion_sensor_initialization_duration) {
    delay(10);
  }

  // Finish
  Serial.println();
  Serial.println(F("Setup completed"));
  Serial.println();

  digitalWrite(c_pin_status_led, HIGH);
  delay(1 * 1000);

  // Register interrupts
  pinMode(c_pin_motion_sensor, INPUT);
  attachInterrupt(
    digitalPinToInterrupt(c_pin_motion_sensor),
    interrupt_handler_motion_sensor,
    CHANGE
  );
}

void loop() {
  bool sending_commands_allowed =
    g_last_motion_detected_time == 0 // Do not act until motion has been detected at least once
    || millis() - g_light_last_turned_on_time >= c_light_on_duration; // Do not act while light should still be turned on

  // Handle detected motion
  // TODO: Improve program flow clarity
  if (g_last_motion_detected_time != 0 && sending_commands_allowed) {
    if (millis() - g_last_motion_detected_time <= c_light_on_duration) {
      // Motion detector was activated recently
      Serial.println();
      Serial.println(F("== Turning light on =="));
      int8_t command_status = hue_send_command(c_hue_light_id, c_hue_command_on);
      // TODO: Time from interrupt until command completion
      g_light_last_turned_on_time = millis();
    }
    else {
      // Motion detector wasn't activated recently and light is not off
      Serial.println();
      Serial.println(F("== Turning light off =="));
      int8_t command_status = hue_send_command(c_hue_light_id, c_hue_command_off);
    }
  }
  else {
    // Sending commands is suspended
  }

  // Update time
  if (millis() - g_time_last_update_time >= c_ntp_update_interval) {
    Serial.println(F("== Updating time =="));

    g_time_last_update_time = millis();
  }
}

void interrupt_handler_motion_sensor() {
  bool motion_detected = digitalRead(c_pin_motion_sensor) == HIGH;

  digitalWrite(c_pin_status_led, motion_detected ? LOW : HIGH);
  if (motion_detected) {
    g_last_motion_detected_time = millis();
  }

  Serial.print("# Motion sensor interrupt: ");
  Serial.println(motion_detected ? F("Motion detected") : F("No motion detected"));
}


/*
 * Tries to connect to the specified wifi network.
 * The return value indicates if the connection was successful.
 */
bool wifi_connect(const char* ssid, const char* password) {
  Serial.println(F("Connecting to WiFi network"));
  print_name_value("SSID", ssid);
  print_name_value("Password", password);

  uint32_t connection_start_time = millis();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - connection_start_time > c_wifi_timeout) {
      print_with_duration("Connecting to Wifi network failed", connection_start_time);
      return false;
    }

    delay(10); // Checking the WiFi status too quickly results in an error
  }
  print_with_duration("Connecting to Wifi network successful", connection_start_time);
  char ip_address[16];
  WiFi.localIP().toString().toCharArray(ip_address, sizeof(ip_address));
  print_name_value("IP address", ip_address);

  return true;
}

/* 
 * Sends a command to a Hue light.
 * 
 * Return values:
 *  1: Operation successful
 *  0: Operation not successful
 * -1: Failed to execute operation
 */
int8_t hue_send_command(const char* hue_light_id, const char* command) {
  Serial.println(F("Sending command to Hue light"));
  print_name_value("Light ID", hue_light_id);
  print_name_value("Command", command);

  // Connect to Hue Bridge
  WiFiClient client;
  uint32_t connection_start_time = millis();
  if (!client.connect(c_hue_ip, c_hue_port)) {
    print_with_duration("Connecting to Hue Bridge failed", connection_start_time);
    return -1;
  }
  print_with_duration("Connecting to Hue Bridge successful", connection_start_time);

  // Send request
  client.println("PUT /api/" + String(c_hue_user_id) + "/lights/" + String(hue_light_id) + "/state");
  client.println("Host: " + String(c_hue_ip) + ":" + String(c_hue_port));
  client.println("User-Agent: ESP8266/1.0");
  client.println("Connection: keep-alive");
  client.println("Content-Type: text/json; charset=\"utf-8\"");
  client.print("Content-Length: ");
  client.println(strlen(command));
  client.println();
  client.println(command);

  // Wait for response
  uint32_t request_sent_time = millis();
  while (client.available() == 0) {
    if (millis() - request_sent_time > c_hue_timeout) {
      print_with_duration("Response from Hue Bridge not received", request_sent_time);
      client.stop();
      return -1;
    }

    delay(10);
  }
  print_with_duration("Response from Hue Bridge received", request_sent_time);

  // Parse response
  int8_t operation_successful = 0;
  while(client.available()) {
    String line = client.readStringUntil('\r');
    if (c_debug) {
      Serial.print(line);
    }

    if (line.indexOf("\"success\":") != -1) {
      operation_successful = 1;
      break;
    }
  }
  if (c_debug) {
    Serial.println();
  }

  // Close connection
  client.stop();

  Serial.print(F("Command status: "));
  Serial.println(operation_successful ? F("Successful") : F("Failed"));

  return operation_successful;
}


// ===== Helpers =====
/*
 * Prints a text with the time elapsed since the specified start time.
 */
void print_with_duration(const char* text, uint32_t start_time) {
  Serial.print(text);
  Serial.print(F(" ("));
  Serial.print(millis() - start_time);
  Serial.println(F(" ms)"));
}

/*
 * Prints a name-value pair.
 */
void print_name_value(const char* name, const char* value) {
  Serial.print(name);
  Serial.print(F(": "));
  Serial.println(value);
}

/*
 * Reduces power usage to a minimum.
 * A power cycle is required to restart.
 */
void shut_down(const char* reason) {
  Serial.println();
  Serial.print(F("Shutting down: "));
  Serial.println(reason);

  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(c_pin_status_led, HIGH);
    delay(1 * 1000);
    digitalWrite(c_pin_status_led, LOW);
    delay(1 * 2000);
  }

  digitalWrite(c_pin_status_led, HIGH);
  ESP.deepSleep(0);
}
