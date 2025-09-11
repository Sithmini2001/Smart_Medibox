// Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

// Define OLED Parameters
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64,

// Time parameters
#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET  3600*5 + 30*60
#define UTC_OFFSET_DST 0

// Pin Configurations for Components
#define BUZZER 5    // Buzzer pin
#define LED_1 15     // LED used to alert the user for medicine time
#define LED_2 2      // LED used to warn the user about temperature & humidity
#define CANCEL 34    // Push button for cancel the menu selection and stop alarm when ringing
#define OK 32        // Push button to open menu selection & confirm the selections
#define UP 33        // Push button for navigating up in the menu
#define DOWN 35      // Push button for navigating down in the menu
#define DHTPIN 12    // Pin for DHT temperature and humidity sensor

// Delay time after printing a message on the display
#define MESSAGE_DELAY 1000
// Delay time for push buttons debounce & other common wait
#define DELAY 200

// Object parameters for display & sensor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED display object
DHTesp dhtSensor; // DHT temperature and humidity sensor object


//.........................................................................................................

//variables

// Buzzer tone parameters
int n_notes = 8;  // Number of musical notes
int C = 262;      // Frequency of note C
int D = 294;      // Frequency of note D
int E = 330;      // Frequency of note E
int F = 349;      // Frequency of note F
int G = 392;      // Frequency of note G
int A = 440;      // Frequency of note A
int B = 494;      // Frequency of note B
int C_H = 523;    // Frequency of high C
int notes[] = {C, D, E, F, G, A, B, C_H};  // Array to store note frequencies

// Time parameters to keep hours & minutes to check alarms
int hours = 0;    // Variable to store hours
int minutes = 0;  // Variable to store minutes

// Variable to store UTC offset in seconds
long utc_offset_sec = 0;  

// Alarm parameters
bool alarm_enabled = false;  // Flag to indicate whether alarms are enabled or not
int n_alarms = 2;  // Number of alarms

// Struct to represent an alarm
struct alarm {
  int hours;        // Hours for the alarm
  int minutes;      // Minutes for the alarm
  bool triggered;   // Flag to indicate if the alarm has been triggered
};

// Array of alarm structs to store multiple alarms
struct alarm alarms[] = {
  {0, 0, true},  // Alarm 1: Default values (0 hours, 0 minutes), initially triggered
  {0, 0, true},  // Alarm 2: Default values (0 hours, 0 minutes), initially triggered
  
};


// Menu parameters
int current_mode = 0;  // Current selected menu mode (default: 0)
int max_modes = 5;     // Maximum number of menu modes
String modes[] = {
  " 1 - Set Timezone",
  " 2 - Set Alarm 1",
  " 3 - Set Alarm 2",
  " 4 - View Active Alarms",
  "5-Delete a Specific Alarm"
};  // Array of menu mode strings

void setup() {
  Serial.begin(9600); // Baud rate

  // Pin configurations
  pinMode(BUZZER, OUTPUT);  // Configure BUZZER pin as an output
  pinMode(LED_1, OUTPUT);    // Configure LED_1 pin as an output
  pinMode(LED_2, OUTPUT);    // Configure LED_2 pin as an output
  pinMode(CANCEL, INPUT);    // Configure CANCEL button pin as an input
  pinMode(UP, INPUT);        // Configure UP button pin as an input
  pinMode(OK, INPUT);        // Configure OK button pin as an input
  pinMode(DOWN, INPUT);      // Configure DOWN button pin as an input

  // Initialize Temp & Humidity sensor
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);  // Setup DHT22 sensor on specified pin

  // Initialize the OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for (;;); // If OLED initialization fails, loop forever
  }

  display.display(); // Display initialization
  delay(MESSAGE_DELAY); // Pause for 1 second 

  print_line("Welcome to Medibox!", 5, 20, 2, true); // Display welcome message
  delay(MESSAGE_DELAY); // Pause for 1 second 

  // Initialize the WIFI
  WiFi.begin("Wokwi-GUEST", "", 6); // Connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(DELAY);
    print_line("Connecting to WIFI", 5, 20, 2, true); // Display connecting message
  }

  print_line("Connected to WIFI", 5, 20, 2, true); // Display connected message

  // Configure Initial Time using WIFI network
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER); 
}

void loop() {
  // Check for menu button pressed
  if (digitalRead(OK) == LOW) {
    delay(MESSAGE_DELAY); // Add a delay to debounce the button
    go_to_menu(); // If OK button is pressed, go to the menu function
  }

  update_time(); // Update the current time
  check_alarm(); // Check if any alarms should be triggered
  check_temperature_humidity(); // Check temperature and humidity levels
  
}

void print_line(String text, int column, int row, int text_size, bool clear) {
  // Clear the display if the 'clear' parameter is true
  if (clear) {
    display.clearDisplay();
  }

  // Set text size, color, and cursor position
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);

  // Print the specified text on the display
  display.println(text);

  // Update the display
  display.display();
}

void update_time() {
  // Function to update time and print in OLED display (HH:MM:SS)

  struct tm timeinfo;
  getLocalTime(&timeinfo); // Get the current time information from the system

  char time_hour[3]; // Array to store the extracted hour
  char time_minute[3]; // Array to store the extracted minute
  char full_time[9]; // Array to store the full time in HH:MM:SS format

  strftime(time_hour, 3, "%H", &timeinfo); // Extract and format the hour from timeinfo
  strftime(time_minute, 3, "%M", &timeinfo); // Extract and format the minute from timeinfo
  strftime(full_time, 9, "%T", &timeinfo); // Format the full time in HH:MM:SS and store in full_time array

  print_line(String(full_time), 15, 20, 2, true); // Display the time in (HH:MM:SS) on the OLED display

  // Update global time variables
  hours = atoi(time_hour); // Convert the extracted hour to an integer and update the global hours variable
  minutes = atoi(time_minute); // Convert the extracted minute to an integer and update the global minutes variable
}

void check_alarm() {
  // Function to check alarms and ring if any.

  // Check if alarms are enabled
  if (!alarm_enabled) {
    return; // If alarms are not enabled, exit the function
  }

  // Iterate through each alarm
  for (int i = 0; i < n_alarms; i++) {
    // Check if the alarm is not triggered and its time matches the current time
    if (!alarms[i].triggered && alarms[i].hours == hours && alarms[i].minutes == minutes) {
      ring_alarm(); // If conditions are met, ring the alarm
      alarms[i].triggered = true; // Set the alarm as triggered
    }
  }
}

void ring_alarm() {
  // Function to ring alarm to notify user
  print_line("Medicine Time!", 10, 20, 2, true); // Display "Medicine Time!" on the OLED display

  digitalWrite(LED_1, HIGH); // Light up LED 1 to indicate alarm activation

  bool alarm_active = true;  // Flag to track if the alarm is still active
  bool snoozed = false;      // Flag to track if snooze is activated

  while (alarm_active) {
    // Iterate through the notes for the buzzer
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(CANCEL) == LOW) {  // If CANCEL button is pressed
        delay(200);  // Debounce delay
        alarm_active = false; // Stop the alarm
        print_line("Alarm Stopped", 10, 20, 2, true);
        delay(1000);
        break;
      }
      if (digitalRead(OK) == LOW) {  // If OK button is pressed (Set snooze alarm for OK button)
        delay(200);  // Debounce delay
        snoozed = true;
        alarm_active = false;
        print_line("Snoozed for 5 min", 10, 20, 2, true);
        delay(1000);
        break;
      }

      // Activate the buzzer with the current note
      tone(BUZZER, notes[i]);
      delay(DELAY);  // Pause for a short duration while the buzzer sounds
      noTone(BUZZER); // Turn off the buzzer
      delay(2); // A brief delay between notes
    }
  }

  digitalWrite(LED_1, LOW);  // Turn off LED 1
  display.clearDisplay();  // Clear the OLED display

  // If snooze was activated, set a delay for 5 minutes (300000 ms) before ringing again
  if (snoozed) {
    delay(300000);  // 5 minutes
    ring_alarm();   // Call the function again to ring the alarm
  }
}

int wait_for_button_press() {

  // Continue waiting for a button press until one is detected
  while (true) {
    // Check if the UP button is pressed
    if (digitalRead(UP) == LOW) {
      delay(DELAY); // Add a delay for button debounce
      return UP; // Return the pin number for the UP button
    }

    // Check if the DOWN button is pressed
    else if (digitalRead(DOWN) == LOW) {
      delay(DELAY); // Add a delay for button debounce
      return DOWN; // Return the pin number for the DOWN button
    }

    // Check if the OK button is pressed
    else if (digitalRead(OK) == LOW) {
      delay(DELAY); // Add a delay for button debounce
      return OK; // Return the pin number for the OK button
    }

    // Check if the CANCEL button is pressed
    else if (digitalRead(CANCEL) == LOW) {
      delay(DELAY); // Add a delay for button debounce
      return CANCEL; // Return the pin number for the CANCEL button
    }
  }
}

int read_value(String text, int init_value, int max_value, int min_value) {

  // Initialize a temporary variable with the initial value
  int temp_value = init_value;

  // Continue reading user input until a decision is made
  while (true) {
    // Display the current value and user instructions on the OLED display
    print_line(text + String(temp_value), 0, 0, 2, true);

    // Wait for a button press and store the pressed button's value
    int pressed = wait_for_button_press();

    // Check if the UP button is pressed
    if (pressed == UP) {
      delay(DELAY); // Add a delay for button debounce
      temp_value += 1; // Increment the temporary value
      // Wrap around to the minimum value if it exceeds the maximum
      if (temp_value > max_value) {
        temp_value = min_value;
      }
    }

    // Check if the DOWN button is pressed
    else if (pressed == DOWN) {
      delay(DELAY); // Add a delay for button debounce
      temp_value -= 1; // Decrement the temporary value
      // Wrap around to the maximum value if it goes below the minimum
      if (temp_value < min_value) {
        temp_value = max_value;
      }
    }

    // Check if the OK button is pressed
    else if (pressed == OK) {
      delay(DELAY); // Add a delay for button debounce
      return temp_value; // Return the final adjusted value
    }

    // Check if the CANCEL button is pressed
    else if (pressed == CANCEL) {
      delay(DELAY); // Add a delay for button debounce
      break; // Exit the loop if cancel button is pressed
    }
  }

  // Return the initial value if the loop is exited without confirmation
  return init_value;
}

void set_timezone_and_config() {
  // Function to set timezone and configuration based on user input

  // Calculate current offset hours and minutes
  int current_offset_hours = utc_offset_sec / 3600;
  int current_offset_minutes = (utc_offset_sec % 3600) / 60;

  // Read user input for offset hours with a default value, a maximum value, and a minimum value
  int offset_hours = read_value("Enter offset hours: ", current_offset_hours, 14, -12);

  // Read user input for offset minutes with a default value, a maximum value, and a minimum value
  int offset_minutes = read_value("Enter offset minutes: ", current_offset_minutes, 59, 0);

  // Calculate the new UTC offset in seconds
  utc_offset_sec = offset_hours * 3600 + offset_minutes * 60;

  // Configure time settings using the new UTC offset and NTP server
  configTime(utc_offset_sec, UTC_OFFSET_DST, NTP_SERVER);

  // Display a message indicating the updated timezone
  print_line("Timezone set to " + String(offset_hours) + ":" + String(offset_minutes), 0, 10, 2, true);

  // Pause for a predefined delay after displaying the message
  delay(MESSAGE_DELAY);
}

void set_alarm(int alarm) {
  /**
   Function to set alarm based on user input
  */

  // Read user input for alarm hour with a default value, a maximum value, and a minimum value
  alarms[alarm].hours = read_value("Enter hour: ", alarms[alarm].hours, 23, 0);

  // Read user input for alarm minute with a default value, a maximum value, and a minimum value
  alarms[alarm].minutes = read_value("Enter minute: ", alarms[alarm].minutes, 59, 0);

  // Reset the triggered flag for the alarm
  alarms[alarm].triggered = false;

  // Enable alarms if not already enabled
  if (!alarm_enabled) {
    alarm_enabled = true;
  }

  // Display a message indicating the newly set alarm time
  print_line(" Alarm " + String(alarm + 1 ) + " is set to " + String(alarms[alarm].hours) + ":" + String(alarms[alarm].minutes), 10, 10, 2, true);

  // Pause for a predefined delay after displaying the message
  delay(MESSAGE_DELAY);
}

void view_active_alarms() {
  display.clearDisplay();
  int y_offset = 0;
  print_line("Active Alarms:", 0, 0, 1, false);
  
  bool found = false;
  for (int i = 0; i < n_alarms; i++) {
    if (!alarms[i].triggered) {
      String alarmText = "Alarm " + String(i + 1) + ": " + String(alarms[i].hours) + ":" + (alarms[i].minutes < 10 ? "0" : "") + String(alarms[i].minutes);
      print_line(alarmText, 0, 20 + i*20, 1, false);
      found = true;
    }
  }
  
  if (!found) {
    print_line("No active alarms", 0, 10, 2, false);
  }
  
  display.display();
  delay(MESSAGE_DELAY * 2);
}

void delete_an_alarm() {
  if (n_alarms == 0) {
    print_line("No Alarms Set", 10, 10, 2, true);
    delay(MESSAGE_DELAY);
    return;
  }

  int selected_alarm = 0;

  while (true) {
    // Display the currently selected alarm for deletion
    print_line("Delete Alarm:", 10, 10, 1, true);
    print_line("Alarm " + String(selected_alarm + 1) + ": " + 
               String(alarms[selected_alarm].hours) + ":" + 
               (alarms[selected_alarm].minutes < 10 ? "0" : "") + 
               String(alarms[selected_alarm].minutes), 10, 20, 1, false);
    print_line("UP to increase", 10, 30, 1, false);
    print_line("DOWN to decrease", 10, 40, 1, false);
    print_line("OK to delete", 10, 50, 1, false);

    int pressed = wait_for_button_press();

    if (pressed == UP) {
      // Increase selected alarm index
      selected_alarm = (selected_alarm + 1) % n_alarms;
    } 
    else if (pressed == DOWN) {
      // Decrease selected alarm index
      selected_alarm = (selected_alarm - 1 + n_alarms) % n_alarms;
    } 
    else if (pressed == OK) {
      // Shift alarms to remove the selected one
      for (int i = selected_alarm; i < n_alarms - 1; i++) {
        alarms[i] = alarms[i + 1];
      }
      n_alarms--; // Reduce alarm count

      // Display the updated alarm list
      print_line("Alarm Deleted!", 10, 30, 2, true);
      delay(MESSAGE_DELAY);
      if (n_alarms > 0) {
        // Display next alarm after deletion
        selected_alarm = (selected_alarm >= n_alarms) ? n_alarms - 1 : selected_alarm; // Adjust index if necessary
        print_line("Next Alarm:", 10, 10, 1, true);
        print_line("Alarm " + String(selected_alarm + 1) + ": " + 
                   String(alarms[selected_alarm].hours) + ":" + 
                   (alarms[selected_alarm].minutes < 10 ? "0" : "") + 
                   String(alarms[selected_alarm].minutes), 10, 20, 1, false);
      } else {
        print_line("No Alarms Left", 10, 30, 2, true);
      }
      delay(MESSAGE_DELAY);
      return;
    } 
    else if (pressed == CANCEL) {
      print_line("Cancelled!", 10, 30, 2, true);
      delay(MESSAGE_DELAY);
      return;
    }
  }
}

void check_temperature_humidity() {
  // Function to check temperature and humidity and warn if exceeded recommended limit

  // Retrieve temperature and humidity data from the DHT sensor
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  // Initialize a flag for warnings
  bool warning = false;

  // Check if temperature is higher than 32 degrees Celsius
  if (data.temperature > 32) {
    warning = true;
    print_line("Temperature HIGH", 15, 40, 1, false);
  }
  // Check if temperature is lower than 24 degrees Celsius
  else if (data.temperature < 24) {
    warning = true;
    print_line("Temperature LOW", 15, 40, 1, false);
  }

  // Check if humidity is higher than 80%
  if (data.humidity > 80) {
    warning = true;
    print_line("Humidity HIGH", 15, 50, 1, false);
  }
  // Check if humidity is lower than 65%
  else if (data.humidity < 65) {
    warning = true;
    print_line("Humidity LOW", 15, 50, 1, false);
  }

  // If there is a warning, briefly activate LED_2 to draw attention
  if (warning) {
    digitalWrite(LED_2, HIGH);
    delay(DELAY);
    digitalWrite(LED_2, LOW);
  }
}

void run_mode(int mode) {
  // Function to run the selected menu

  // Check the value of the mode parameter and execute corresponding actions
  if (mode == 0) {
    set_timezone_and_config(); // Call function to set timezone and configuration
  }

  else if (mode == 1 || mode == 2 ) {
    set_alarm(mode - 1); // Call function to set alarm based on the mode
  }

  else if (mode == 3) {
    view_active_alarms();  // call function to view active alarms
  }

  else if (mode == 4) {
    delete_an_alarm(); // Call function to deleta a specific alarm
  }
}

void go_to_menu() {
  // Function to check menu and run based on user selection

  // Continue displaying the menu until the cancel button is pressed
  while (digitalRead(CANCEL) == HIGH) {
    print_line(modes[current_mode], 10, 10, 2, true); // Display the current menu item on the OLED display

    // Wait for a button press and store the pressed button's value
    int pressed = wait_for_button_press();

    // Check if the UP button is pressed
    if (pressed == UP) {
      delay(DELAY); // Add a delay for button debounce
      current_mode += 1; // Increment the current mode
      current_mode = current_mode % max_modes; // Wrap around to the first mode if it exceeds the maximum
    }

    // Check if the DOWN button is pressed
    else if (pressed == DOWN) {
      delay(DELAY); // Add a delay for button debounce
      current_mode -= 1; // Decrement the current mode
      // Wrap around to the last mode if it goes below 0
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    }

    // Check if the OK button is pressed
    else if (pressed == OK) {
      delay(DELAY); // Add a delay for button debounce
      run_mode(current_mode); // Execute the selected mode
    }

    // Check if the CANCEL button is pressed
    else if (pressed == CANCEL) {
      delay(DELAY); // Add a delay for button debounce
      break; // Exit the menu loop if cancel button is pressed
    }
  }
}



