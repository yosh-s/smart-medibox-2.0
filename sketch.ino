#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cmath>
#include <ESP32Servo.h>

// Define OLED Display Parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define hardware pins
#define BUZZER 5
#define LED_1 15
#define LED_2 4
#define PB_CANCEL 25
#define PB_OK 26
#define PB_UP 27
#define PB_DOWN 13
#define DHTPIN 12
#define LDR_PIN 33
#define SERVO_PIN 14

// Time settings
#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET 19800 // 5 hours 30 minutes in seconds (5 * 3600 + 30 * 60)
#define UTC_OFFSET_DST 0 // No daylight saving time in Sri Lanka

// Declare objects
DHTesp dhtSensor;                   // Create a DHT object
WiFiClient espClient;               // Create a WiFi client object
PubSubClient mqttClient(espClient); // Create a PubSubClient object
Servo servoMotor;                   // Create a Servo object

// Variables for timekeeping
int hours, minutes, seconds, day, month, year;
int alarm_hours[] = {-1, -1};
int alarm_minutes[] = {-1, -1};
bool snooze_active = false;
unsigned long snooze_time = 0;
int LED2_blink;
int MAX_ADC_VALUE = 4063; // Maximum ADC value for ESP32
int MIN_ADC_VALUE = 32;   // Minimum ADC value for ESP32

// Variables to store light intensity readings
unsigned long lastSamplingTime = 0;  // LDR sampling time
unsigned long lastSendTime = 0;      // LDR sampling time
unsigned long last_servo_update = 0; // Servo update time
unsigned long lastPublishTime = 0;   // DHT publish time
float lightReading_sum = 0;          // DHT read time
float normalized_LDR_Value = 0;      // Normalized LDR value

// dashboard variables
bool LED1_on = false;                                // LED state
int LDR_sampling_interval = 5 * 1000;                // Sampling interval in milliseconds
int LDR_sending_interval = 2 * 60 * 1000;            // Sending interval in milliseconds
bool scheduled = false;                              // Scheduled state
int DHT_sending_interval = 5000;                     // Default sending interval in milliseconds
int servo_update_interval = 2000;                    // Servo update interval in milliseconds
int sc_hours, sc_minutes, sc_day, sc_month, sc_year; // Scheduled time variables
float min_angle = 30;                                // Minimum angle for servo motor
float ctrl_factor = 0.75;                            // Control factor for servo motor
float ideal_temp = 30;                               // Ideal temperature for the system
bool sys_active = true;                              // System active state

// topics used for MQTT
const char *main_switch_topic = "220707/MEDI_ON_OFF";    
const char *LDR_data_send = "220707/LDR_DATA";           
const char *sampling_inter_receive = "220707/SAM_INTER"; 
const char *sending_inter_receive = "220707/SEND_INTER"; 
const char *schedule_time_receive = "220707/SCH_TIME";   
const char *schedule_state_receive = "220707/SCH_STATE";
const char *min_angle_receive = "220707/MIN_ANGLE";    
const char *ctrl_factor_receive = "220707/CTRL_FACT";   
const char *ideal_temp_receive = "220707/IDEAL_TEMP";

// Use a more unique client ID to resolve the issue of duplicate client IDs
String clientId = "esp1254785458" + String(random(0, 1000));
const char *esp_client = clientId.c_str();

char tempAr[6]; // Buffer for temperature string
char humAr[6];  // Buffer for humidity string

// Function to print text on OLED display at given position and size
void print_line(String text, int x, int y, int size){
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.println(text);
}

// display time, temp and humidity
void print_time_now(){
  display.clearDisplay(); // Clear display once at the start
  char text[9];
  sprintf(text, "%02d:%02d:%02d", hours, minutes, seconds);
  print_line(text, 20, 0, 2);
  // Print date
  char date[11];
  sprintf(date, "%04d - %02d - %02d", year, month, day); // Format date as YYYY-MM-DD
  print_line(date, 0, 20, 1);

  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  print_line("T = " + String(data.temperature) + "C" + " H = " + String(data.humidity) + "%", 0, 30, 1);
  // print_line("Humidity:    " + String(data.humidity) + " %", 0, 30, 1);

  boolean temp_warning = data.temperature < 24 || data.temperature > 32;
  boolean humidity_warning = data.humidity < 65 || data.humidity > 80;

  if (temp_warning)
    print_line("Temperature Warning!", 0, 40, 1);
  if (humidity_warning)
    print_line("Humidity Warning!", 0, 50, 1);

  display.display(); // Refresh once after all updates

  if (temp_warning || humidity_warning)
  {
    digitalWrite(LED_1, !digitalRead(LED_1));
    if (LED2_blink + 500 < millis())
    {
      digitalWrite(LED_2, !digitalRead(LED_2));
      digitalWrite(LED_1, !digitalRead(LED_1));
      LED2_blink = millis();

      tone(BUZZER, 330);
      delay(300);
      noTone(BUZZER);
      delay(50);
    }
  }
  else
  {
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_1, LOW);
  }

  if (LED1_on)
  {
    digitalWrite(LED_1, HIGH); // Turn on LED_1 if LED1_on is true
  }
  else
  {
    digitalWrite(LED_1, LOW); // Turn off LED_1 if LED1_on is false
  }
}

// Function to play a simple melody
void play_melody(){
  int melody[] = {262, 294, 330, 262, 330, 392, 440};        // musical notes (C4, D4, E4, C4, E4, G4, A4)
  int noteDurations[] = {300, 300, 300, 300, 400, 400, 500}; // playing durations in milliseconds

  for (int i = 0; i < 7; i++)
  {
    tone(BUZZER, melody[i]); // Play note
    delay(noteDurations[i]); // Hold note duration
    noTone(BUZZER);          // Stop note before next one
    delay(50);               // Small pause between notes
  }
}

// Function to ring the alarm
void ring_alarm(){
  display.clearDisplay();
  print_line("MEDICINE", 25, 5, 2);
  print_line("TIME!", 40, 20, 2);
  print_line("OK: Snooze (5 min)", 10, 40, 1);
  print_line("Cancel: Stop", 10, 50, 1);
  display.display();

  digitalWrite(LED_1, HIGH); // LED alert

  while (true)
  {
    play_melody(); // Play melody on buzzer
    digitalWrite(LED_1, !digitalRead(LED_1));

    // Snooze Alarm if PB_OK is pressed
    if (digitalRead(PB_OK) == LOW)
    {
      snooze_active = true;
      snooze_time = millis() + 300000; // 5 minutes = 300000 ms

      display.clearDisplay();
      print_line("Snoozed for 5 min", 10, 20, 1);
      display.display();
      delay(1000);
      break;
    }

    // Stop Alarm if PB_CANCEL is pressed
    if (digitalRead(PB_CANCEL) == LOW)
    {
      display.clearDisplay();
      print_line("Alarm", 10, 20, 2);
      print_line("Stopped !", 10, 40, 2);
      display.display();
      delay(1000);
      break;
    }
  }

  // Stop Buzzer & LED after alarm is stopped or snoozed
  noTone(BUZZER);
  digitalWrite(LED_1, LOW);
  display.clearDisplay();
  display.display();
}

// Function to update time and check for alarm triggers
void update_time_with_check_alarm(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return;
  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  day = timeinfo.tm_mday;
  month = timeinfo.tm_mon + 1;    // tm_mon is 0-11, so add 1
  year = timeinfo.tm_year + 1900; // tm_year is years since 1900
  print_time_now();

  // Check if the current time matches any alarm time
  for (int i = 0; i < 2; i++)
  {
    if (hours == alarm_hours[i] && minutes == alarm_minutes[i] && seconds == 0)
    {
      ring_alarm(); // Trigger alarm
    }
  }

  // check scheduled date and time
  if (scheduled)
  {
    if (hours == sc_hours && minutes == sc_minutes && seconds == 0 &&
        day == sc_day && month == sc_month && year == sc_year)
    {
      ring_alarm(); // Trigger scheduled alarm
    }
  }

  if (snooze_active && millis() >= snooze_time)
  {
    snooze_active = false;
    ring_alarm(); // Trigger snoozed alarm
  }
}

// Function to set the system time
void set_time(){
  int new_hours = UTC_OFFSET / 3600;        // Convert seconds to hours
  int new_minutes = UTC_OFFSET % 3600 / 60; // Convert seconds to minutes
  bool setting_minutes = false;

  // Loop until time is set
  while (true)
  {
    display.clearDisplay();
    print_line("Time Zone:", 0, 0, 2);
    print_line(String(new_hours) + ":" + (new_minutes < 10 ? "0" : "") + String(new_minutes), 20, 20, 2);
    print_line(setting_minutes ? "Adjusting Minutes" : "Adjusting Hours", 0, 40, 1);
    print_line("Press Cancel to Exit", 0, 50, 1);
    display.display();

    // handle "PB_UP" button presses
    if (digitalRead(PB_UP) == LOW)
    {
      if (!setting_minutes)
      {
        new_hours = (new_hours == 14) ? -12 : new_hours + 1;
      }
      else
      {
        new_minutes = (new_minutes + 1) % 60;
      }
      delay(200);
    }

    // handle "PB_DOWN" button presses
    if (digitalRead(PB_DOWN) == LOW)
    {
      if (!setting_minutes)
      {
        new_hours = (new_hours == -12) ? 14 : new_hours - 1;
      }
      else
      {
        new_minutes = (new_minutes == 0) ? 59 : new_minutes - 1;
      }
      delay(200);
    }

    // handle "PB_OK" button presses
    if (digitalRead(PB_OK) == LOW)
    {
      delay(200);
      if (!setting_minutes)
      {
        setting_minutes = true;
      }
      else
      {
        int new_utc_offset = new_hours * 3600 + new_minutes * 60; // Convert to seconds
        configTime(new_utc_offset, UTC_OFFSET_DST, NTP_SERVER);   // update new time
        display.clearDisplay();
        print_line("Time Zone", 0, 0, 2);
        print_line("Updated..!", 0, 20, 2);
        display.display();
        delay(1000);

        break;
      }
    }
    if (digitalRead(PB_CANCEL) == LOW)
    {
      delay(200);
      break;
    }
  }
}

// Function to set the alarm
void set_alarm(){
  int alarm_index = 0; // Select alarm slot (0 or 1)
  int new_hours = hours;
  int new_minutes = minutes;
  bool setting_minutes = false;
  bool selecting_alarm = true;

  while (true)
  {
    display.clearDisplay();
    if (selecting_alarm)
    {
      print_line("___Select Alarm___", 5, 0, 1);
      print_line((alarm_index == 0 ? "--> " : "   ") + String("Alarm 1"), 0, 20, 1);
      print_line((alarm_index == 1 ? "--> " : "   ") + String("Alarm 2"), 0, 30, 1);
    }
    else
    {
      print_line("_Alarm " + String(alarm_index + 1) + "_", 10, 0, 2);
      print_line(String(new_hours) + ":" + (new_minutes < 10 ? "0" : "") + String(new_minutes), 30, 20, 2);
      print_line(setting_minutes ? "Adjusting Minutes" : "Adjusting Hours", 0, 40, 1);
    }
    display.display();

    // handle "PB_UP" button presses
    if (digitalRead(PB_UP) == LOW)
    {
      if (selecting_alarm)
      {
        alarm_index = (alarm_index == 0) ? 1 : 0;
      }
      else
      {
        if (!setting_minutes)
          new_hours = (new_hours + 1) % 24;
        else
          new_minutes = (new_minutes + 1) % 60;
      }
      delay(200);
    }

    // handle "PB_DOWN" button presses
    if (digitalRead(PB_DOWN) == LOW)
    {
      if (selecting_alarm)
      {
        alarm_index = (alarm_index == 0) ? 1 : 0;
      }
      else
      {
        if (!setting_minutes)
          new_hours = (new_hours == 0) ? 23 : new_hours - 1;
        else
          new_minutes = (new_minutes == 0) ? 59 : new_minutes - 1;
      }
      delay(200);
    }

    // handle "PB_OK" button presses
    if (digitalRead(PB_OK) == LOW)
    {
      delay(200);
      if (selecting_alarm)
      {
        selecting_alarm = false;
      }
      else if (!setting_minutes)
      {
        setting_minutes = true;
      }
      else
      {
        alarm_hours[alarm_index] = new_hours;
        alarm_minutes[alarm_index] = new_minutes;
        break;
      }
    }

    // handle "PB_CANCEL" button presses
    if (digitalRead(PB_CANCEL) == LOW)
    {
      delay(200);
      break;
    }
  }
}

// Function to view the alarms
void view_alarms(){
  while (true)
  {
    display.clearDisplay();
    print_line("___Current Alarms___", 0, 0, 1);

    for (int i = 0; i < 2; i++)
    {
      // Display the alarm time if set, otherwise display "None"
      if (alarm_hours[i] != -1)
      {
        String alarmText = "Alarm " + String(i + 1) + ": " +
                           String(alarm_hours[i]) + ":" +
                           (alarm_minutes[i] < 10 ? "0" : "") + String(alarm_minutes[i]);
        print_line(alarmText, 0, 15 + (i * 10), 1);
      }
      else
      {
        print_line("Alarm " + String(i + 1) + ": None", 0, 15 + (i * 10), 1);
      }
    }

    print_line("Press Cancel key to exit", 0, 45, 1);
    display.display();

    // Exit when any button is pressed
    if (digitalRead(PB_CANCEL) == LOW)
    {
      delay(200);
      break;
    }
  }
}

// Function to view the scheduled time
void view_schedule(){
  Serial.println("Schelude state");
  Serial.println(scheduled);

  while (true)
  {
    display.clearDisplay();
    print_line("___Scheduled___", 0, 0, 1);
    if (scheduled)
    {
      // print schedule date
      String scheduleText = "Scheduled: " + String(sc_hours) + ":" +
                            (sc_minutes < 10 ? "0" : "") + String(sc_minutes) + " on " +
                            String(sc_day) + "/" + String(sc_month) + "/" + String(sc_year);
      print_line(scheduleText, 0, 20, 1);
    }
    else
    {
      print_line("No Schedule Set", 0, 20, 1);
    }
    print_line("Press Cancel key to exit", 0, 50, 1);
    display.display();

    // Exit when any button is pressed
    if (digitalRead(PB_CANCEL) == LOW)
    {
      delay(200);
      break;
    }
  }
}

// Function to delete an alarm
void delete_alarm(){
  int alarm_index = 0; // Start with the first alarm

  // Loop until an alarm is deleted or the user exits
  while (true)
  {
    display.clearDisplay();
    print_line("___Delete Alarm___", 0, 0, 1);

    for (int i = 0; i < 2; i++)
    {
      if (i == alarm_index)
      {
        print_line("--> Alarm " + String(i + 1) + ": " +
                       String(alarm_hours[i]) + ":" +
                       (alarm_minutes[i] < 10 ? "0" : "") + String(alarm_minutes[i]),
                   0, 15 + (i * 10), 1);
      }
      else
      {
        print_line("Alarm " + String(i + 1) + ": " +
                       String(alarm_hours[i]) + ":" +
                       (alarm_minutes[i] < 10 ? "0" : "") + String(alarm_minutes[i]),
                   0, 15 + (i * 10), 1);
      }
    }

    print_line("PB_OK to Delete", 0, 40, 1);
    print_line("PB_CANCEL to Exit", 0, 50, 1);
    display.display();

    // Navigate between alarms
    if (digitalRead(PB_UP) == LOW)
    {
      alarm_index = (alarm_index == 0) ? 1 : 0; // Toggle between 0 and 1
      delay(200);
    }
    if (digitalRead(PB_DOWN) == LOW)
    {
      alarm_index = (alarm_index == 1) ? 0 : 1; // Toggle between 1 and 0
      delay(200);
    }

    // Delete the selected alarm
    if (digitalRead(PB_OK) == LOW)
    {
      alarm_hours[alarm_index] = -1;   // Mark as deleted
      alarm_minutes[alarm_index] = -1; // Mark as deleted

      display.clearDisplay();
      print_line("Alarm", 0, 20, 2);
      print_line("Deleted!", 0, 40, 2);
      display.display();
      delay(1000);
      break;
    }

    // Exit delete menu
    if (digitalRead(PB_CANCEL) == LOW)
    {
      delay(200);
      break;
    }
  }
}

// Function to navigate the main menu
void go_to_menu(){
  int menu_option = 1;

  // Loop until the user exits the menu
  while (true)
  {
    display.clearDisplay();

    // display menu according to the selected option
    if (menu_option == 1)
    {
      print_line("--> Set Time Zone", 0, 0, 1);
    }
    else
    {
      print_line("1. Set Time Zone", 0, 0, 1);
    }

    if (menu_option == 2)
    {
      print_line("--> Set Alarms", 0, 10, 1);
    }
    else
    {
      print_line("2. Set Alarms", 0, 10, 1);
    }

    if (menu_option == 3)
    {
      print_line("--> View Alarms", 0, 20, 1);
    }
    else
    {
      print_line("3. View Alarms", 0, 20, 1);
    }

    if (menu_option == 4)
    {
      print_line("--> View Scheduled", 0, 30, 1);
    }
    else
    {
      print_line("4. View Scheduled", 0, 30, 1);
    }

    if (menu_option == 5)
    {
      print_line("--> Delete Alarm", 0, 40, 1);
    }
    else
    {
      print_line("5. Delete Alarm", 0, 40, 1);
    }

    if (menu_option == 6)
    {
      print_line("--> Exit", 0, 50, 1);
    }
    else
    {
      print_line("6. Exit", 0, 50, 1);
    }

    display.display();

    // Navigate menu using PB_UP and PB_DOWN
    if (digitalRead(PB_UP) == LOW)
    {
      menu_option = (menu_option == 1) ? 6 : menu_option - 1;
      delay(200);
    }
    if (digitalRead(PB_DOWN) == LOW)
    {
      menu_option = (menu_option == 6) ? 1 : menu_option + 1;
      delay(200);
    }

    // Select option
    if (digitalRead(PB_OK) == LOW)
    {
      delay(200);
      if (menu_option == 1)
        set_time();
      if (menu_option == 2)
        set_alarm();
      if (menu_option == 3)
        view_alarms();
      if (menu_option == 4)
        view_schedule();
      if (menu_option == 5)
        delete_alarm();
      if (menu_option == 6)
        break; // Exit menu
    }

    // Exit menu when PB_CANCEL is pressed
    if (digitalRead(PB_CANCEL) == LOW)
    {
      delay(200);
      break;
    }
  }
}

// Function to set up WiFi connection
void setupWifi(){
  Serial.println("Connecting to WiFi...");
  WiFi.begin("Wokwi-GUEST", "");

  while (WiFi.status() != WL_CONNECTED)
  { // Add timeout
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
  Serial.println("IP address: " + WiFi.localIP().toString());

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER); // Set time zone
}

void connectToBroker(){

  const char *mqtt_server = "test.mosquitto.org";
  mqttClient.setServer(mqtt_server, 1883);

  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5)
  {
    Serial.println("Connecting to MQTT broker...");
    if (mqttClient.connect(esp_client))
    {
      Serial.println("Connected to MQTT broker");
      // Subscribe to topics 
      mqttClient.subscribe("220707/MEDI_ON_OFF");
      mqttClient.subscribe("220707/SAM_INTER");
      mqttClient.subscribe("220707/SEND_INTER");
      mqttClient.subscribe("220707/SCH_TIME");
      mqttClient.subscribe("220707/SCH_STATE");
      mqttClient.subscribe("220707/MIN_ANGLE");
      mqttClient.subscribe("220707/CTRL_FACT");
      mqttClient.subscribe("220707/IDEAL_TEMP");
    }
    else
    {
      Serial.print("Failed to connect, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" - trying again in 2 seconds");
      delay(2000);
      attempts++;
    }
  }

  if (!mqttClient.connected())
  {
    Serial.println("Failed to connect to MQTT broker after several attempts");
    delay(10000); // Wait longer before trying again
  }
}

void power_on_off(bool power_on){
  if (power_on)
  {
    display.clearDisplay();
    print_line("Medibox \nTurning \nON...", 0, 0, 2);
    display.display();
    delay(2000);
    configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER); // Set time zone
  }
  else
  {
    display.clearDisplay();
    print_line("Medibox \nTurning \nOFF...", 0, 0, 2);
    display.display();
    delay(3000);
    display.clearDisplay();
    display.display();
  }
}

void receiveCallback(char *topic, byte *payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  char message[length + 1];
  for (int i = 0; i < length; i++)
  {
    message[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  message[length] = '\0';
  Serial.println();

  // check the topic and message to perform actions
  if (strcmp(topic, main_switch_topic) == 0)
  {
    if (strcmp(message, "ON") == 0)
    {
      Serial.println("Turning device ON");
      power_on_off(true);
      sys_active = true;
    }
    else if (strcmp(message, "OFF") == 0)
    {
      Serial.println("Turning device OFF");
      power_on_off(false);
      sys_active = false;
    }
  }
  else if (strcmp(topic, sampling_inter_receive) == 0)
  {
    LDR_sampling_interval = atoi(message) * 1000; // Convert seconds to milliseconds
  }
  else if (strcmp(topic, sending_inter_receive) == 0)
  {
    LDR_sending_interval = atof(message) * 1000 * 60; // Convert minutes to milliseconds
  }
  else if (strcmp(topic, schedule_time_receive) == 0)
  {
    int sc_seconds;
     // Parse the date and time and store them in the variables
    sscanf(message, "%d-%d-%dT%d:%d:%d", &sc_year, &sc_month, &sc_day, &sc_hours, &sc_minutes, &sc_seconds);
  }
  else if (strcmp(topic, schedule_state_receive) == 0)
  {
    scheduled = strcasecmp(message, "SCHEDULED") == 0;
  }
  else if (strcmp(topic, min_angle_receive) == 0)
  {
    min_angle = strtod(message, nullptr);
  }
  else if (strcmp(topic, ctrl_factor_receive) == 0)
  {
    ctrl_factor = strtod(message, nullptr);
  }
  else if (strcmp(topic, ideal_temp_receive) == 0)
  {
    ideal_temp = strtod(message, nullptr);
  }
}

// Function to publish DHT data
void publish_DHT_data(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  dtostrf(data.temperature, 1, 2, tempAr); // Convert float to string with 2 decimal places
  dtostrf(data.humidity, 1, 2, humAr); // Convert float to string with 2 decimal places

  mqttClient.publish("220707/MEDI_TEMP", tempAr, true); // Retained message
  mqttClient.publish("220707/MEDI_HUM", humAr, true);   // Publish humidity as well
}

// Function to set up pin modes
void setupPinModes(){
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(PB_CANCEL, INPUT_PULLUP);
  pinMode(PB_OK, INPUT_PULLUP);
  pinMode(PB_UP, INPUT_PULLUP);
  pinMode(PB_DOWN, INPUT_PULLUP);
}

// Function to sample light intensity
void sampleLightIntensity(){
  // Read analog value from light sensor
  int rawValue = analogRead(LDR_PIN);

  // Normalize to 0-1 range
  normalized_LDR_Value = (-(float)rawValue + MAX_ADC_VALUE) / (MAX_ADC_VALUE - MIN_ADC_VALUE);
  lightReading_sum += normalized_LDR_Value; // Add to sum for average calculation
}

// Function to send average light intensity
void sendAverageIntensity(){
  // Calculate average
  float count = LDR_sending_interval / LDR_sampling_interval;
  float average = lightReading_sum / count;

  // Convert float to string with 4 decimal places
  char valueStr[10];
  dtostrf(average, 1, 4, valueStr);

  // Publish to MQTT
  mqttClient.publish(LDR_data_send, valueStr);
  lightReading_sum = 0;
}

// Function to calculate servo angle based on light intensity and temperature
double servo_angle(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  double T = data.temperature; // Current temperature

  double theta = min_angle +
                 (180.0 - min_angle) *
                     normalized_LDR_Value *
                     ctrl_factor *
                     std::log((double)LDR_sending_interval / LDR_sampling_interval) *
                     (T / ideal_temp);

  return theta;
}

void setup(){
  Serial.begin(115200);
  setupPinModes();                        // Set up pin modes
  dhtSensor.setup(DHTPIN, DHTesp::DHT22); // Initialize DHT sensor

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println("SSD1306 allocation failed");
    while (true)
      ;
  }

  display.display(); // Initialize display
  delay(2000);
  setupWifi();

  // Set the callback function for incoming messages
  mqttClient.setCallback(receiveCallback);

  connectToBroker();
  // Servo motor setup
  servoMotor.setPeriodHertz(50);
  servoMotor.attach(SERVO_PIN, 500, 2400);
}

void loop(){

  if (!mqttClient.connected())
    connectToBroker();
  mqttClient.setCallback(receiveCallback); 
  mqttClient.loop();

  if (!sys_active)
  {
    delay(3000); // if system is off, main loop stop here
    return;
  }

  update_time_with_check_alarm();

  unsigned long currentTime = millis();

  if (digitalRead(PB_OK) == LOW)
  {
    delay(200);
    go_to_menu();
  }

   // Publish temperature and humidity data
  if (currentTime - lastPublishTime >= DHT_sending_interval)
  {
    publish_DHT_data();
    lastPublishTime = currentTime;
  }

   // Sample light intensity in given interval
  if (currentTime - lastSamplingTime >= LDR_sampling_interval)
  {
    lastSamplingTime = currentTime;
    sampleLightIntensity();
  }

  // Send average light intensity at the specified interval
  if (currentTime - lastSendTime >= LDR_sending_interval)
  {
    lastSendTime = currentTime;
    sendAverageIntensity();
  }

  // Update servo motor position in given interval
  if (currentTime - last_servo_update >= servo_update_interval)
  {
    last_servo_update = currentTime;
    servoMotor.write(constrain(servo_angle(), 0, 180));
  }
}
