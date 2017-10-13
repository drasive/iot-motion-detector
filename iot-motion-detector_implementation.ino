/*
  IoT Motion Detector v0.2
  A $6 internet of things and easy to build motion detector.

  See iot-motion-detector.ino for configuration.

  License:      MIT
  Repository:   https://github.com/drasive/iot-motion-detector
  Author:       Dimitri Vranken <me@dimitrivranken.com>
*/


// ===== Program =====
volatile uint32_t g_motion_last_detected_time = 0;
uint32_t g_light_last_turned_on_time = 0;

WiFiUDP g_ntp_udp;
NTPClient g_ntp_client(g_ntp_udp, c_ntp_host, c_ntp_offset / 1000, 0);
uint32_t g_time_last_updated_time = 0;


void setup() {
  // Initialize
  pinMode(c_pin_status_led, OUTPUT);
  digitalWrite(c_pin_status_led, LOW);
  uint32_t boot_time = millis();

  Serial.begin(c_baud_rate);
  Serial.println();
  Serial.println(F("===== IoT Motion Detector v0.2 ====="));
  Serial.println(F("https://github.com/drasive/iot-motion-detector"));
  Serial.println(F("Dimitri Vranken <me@dimitrivranken.com>"));

  // Connect to WiFi network
  Serial.println();
  if (!ensure_wifi_connected(c_wifi_ssid, c_wifi_password)) {
    shut_down("Failed to connect to WiFi during setup");
  }

  // Update time
  Serial.println();
  g_ntp_client.begin();
  update_ntp_time(c_ntp_host, c_ntp_offset);
  g_time_last_updated_time = millis();

  // Wait for motion sensor initialization
  Serial.println();
  Serial.println("Waiting for motion sensor initialization");
  while (millis() - boot_time <= c_motion_sensor_init_duration) {
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
  // Ensure WiFi connection
  ensure_wifi_connected(c_wifi_ssid, c_wifi_password);

  // Handle detected motion
  bool sending_commands_allowed =
    g_motion_last_detected_time == 0 // Do not act until motion has been detected at least once
    || millis() - g_light_last_turned_on_time >= c_light_on_duration; // Do not act while light should still be turned on

  if (g_motion_last_detected_time != 0 && sending_commands_allowed) {
    if (millis() - g_motion_last_detected_time <= c_light_on_duration) {
      // Motion detector was activated recently
      Serial.println();
      Serial.println(F("== Turning light on =="));

      int8_t command_status = is_daytime(g_ntp_client.getHours(), g_ntp_client.getMinutes(), c_nighttime_start, c_nighttime_duration)
        ? hue_send_command(c_hue_light_identifier, c_hue_command_on_daytime)
        : hue_send_command(c_hue_light_identifier, c_hue_command_on_nighttime);

      if (command_status == 1) {
        g_light_last_turned_on_time = millis();

        Serial.print(F("Response time: "));
        Serial.print(millis() - g_motion_last_detected_time);
        Serial.println(F(" ms"));
      }
    }
    else {
      // Motion detector wasn't activated recently
      Serial.println();
      Serial.println(F("== Turning light off =="));
      hue_send_command(c_hue_light_identifier, c_hue_command_off);
    }
  }
  else {
    // Sending commands is suspended
  }

  // Update time
  if (millis() - g_time_last_updated_time >= c_ntp_update_interval) {
    Serial.println();
    Serial.println(F("== Updating time =="));

    update_ntp_time(c_ntp_host, c_ntp_offset);
    g_time_last_updated_time = millis();
  }

  delay(10);
}

void interrupt_handler_motion_sensor() {
  bool motion_detected = digitalRead(c_pin_motion_sensor) == HIGH;

  digitalWrite(c_pin_status_led, motion_detected ? LOW : HIGH);
  if (motion_detected) {
    g_motion_last_detected_time = millis();
  }

  Serial.print("# Motion sensor interrupt: ");
  Serial.println(motion_detected ? F("Motion detected") : F("No motion detected"));
}


/*
 * Tries to connect to the specified WiFi network, if not already connected.
 * The return value indicates if the connection was successful.
 */
bool ensure_wifi_connected(const char* ssid, const char* password) {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

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
int8_t hue_send_command(const char* hue_light_identifier, const char* command) {
  Serial.println(F("Sending command to Hue light"));
  print_name_value("Light identifier", hue_light_identifier);
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
  client.println("PUT /api/" + String(c_hue_user_id) + "/" + String(hue_light_identifier) + "/state");
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

/*
 * Synchronizes the system clock with an NTP server.
 */
void update_ntp_time(const char* ntp_host, const int32_t ntp_offset) {
  Serial.println(F("Requesting time from NTP server"));
  print_name_value("NTP Host", ntp_host);
  print_name_value("NTP Offset", String(ntp_offset));

  uint32_t request_sent_time = millis();
  g_ntp_client.forceUpdate();
  print_with_duration("Response from NTP server received", request_sent_time);

  Serial.print(F("System time: "));
  Serial.println(g_ntp_client.getFormattedTime());
}


// ===== Helpers =====
/*
 * Returns true if the specified time lies outside of the specified nighttime; false otherwise.
 */
bool is_daytime(
  const uint8_t current_hour, const uint8_t current_minute,
  const uint16_t nighttime_start, const uint16_t nighttime_duration) {
    uint16_t current_minutes = (current_hour * 60) + current_minute;
    uint16_t nighttime_end = (nighttime_start + nighttime_duration) % (24 * 60);
    
    if (nighttime_end < nighttime_start) {
      // Nighttime crosses over midnight
      return current_minutes < nighttime_start && current_minutes >= nighttime_end;
    }
    
    // Nighttime doesn't cross over midnight
    return current_minutes < nighttime_start || current_minutes >= nighttime_end;
}


/*
 * Prints a text with the time elapsed since the specified start time.
 */
void print_with_duration(const char* text, const uint32_t start_time) {
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
 * Prints a name-value pair.
 */
void print_name_value(const char* name, String value) {
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
