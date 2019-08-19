
#include <dummy.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define _len(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#define _openDoor() digitalWrite(pin_door, LOW)
#define _closeDoor() digitalWrite(pin_door, HIGH)

// Assign Wifi connection to AP
char ssid[] = "CanMasdeu";     // your network SSID (name)
char password[] = ""; // your network key

// NTP time client
WiFiUDP ntpUDP;
// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "10.13.0.1", 3600, 60000);

#define pin_door D0 // Connected to active low relay board to door opener
bool panelActivo = true;

const byte ROWS = 4; // Four rows - this is the 1,4,7,* and 2,5,8,0, etc
const byte COLS = 3; // Three columns - this is the 123 and 456 and 789 and *0#

// Define the Keymap
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// Connect keypad ROW0, ROW1, ROW2 and ROW3 to these Arduino pins.
byte rowPins[ROWS] = {5, 4, 0, 2,}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {14, 12, 13};

// Create the Keypad
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

char password_array[5] = {'1', '2', '3', '4', '5'}; // User Settable Password
int password_length = _len(password_array);

char change_password_array[6] = {'5', '6', '8', '9', '#', '*'}; // Enter this combination, wait for buzzer to stop and enter new 4 digit password
int change_password_length = 6;

char master_password_array[5] = {'6', '7', '8', '9', '0'}; // Master Password
int master_password_length = 5;

int both_passwords_length = password_length + master_password_length;

char diagnostics_password_array[5] = {'2', '2', '2', '2', '2'}; // spits out diagnostics to serial
int diagnostics_password_length = 5;

const int door_open_time = 2000;

char input_array[9]; // Circular Buffer for the last pressed buttons
char input_array_length = 9;
int input_pos = 0; // Points to next location in circular buffer to be pressed

void setup() {
    // put your setup code here, to run once:
  pinMode(pin_door, OUTPUT);
  digitalWrite(pin_door, HIGH); //Make sure the door is closed
  Serial.begin(9600); //Serial communication to display 
  ESP.wdtDisable();  // turns off watchdog?
  EEPROM.begin(512);  //Initialize EEPROM
  Serial.println(""); //Goto next line, as ESP sends some garbage when you reset it
  //             Print out the passwords and changes after restart - if you have serial monitor open
  Serial.print("Visitor password before reading EEPROM:"); 
  Serial.println(password_array);
  Serial.print("Master password array before reading EEPROM:");
  Serial.println(master_password_array);    
  // read appropriate byte of the EEPROM.  
  if ( EEPROM.read ( 0 ) != 0xff ) {     //Read from address 0x00, 0x01, 0x02, etc
    // reads visitor password from EEPROM
    for (int i = 0; i < password_length; ++i ) {
      password_array[i] = EEPROM.read(i);
    }
    // reads master password from EEPROM starting after visitor password
    for (int i = 0; i < master_password_length ; ++i ) {
      master_password_array[i] = EEPROM.read(i + password_length);
    }
    //             Print out the changes after writing from EEPROM after restart - if you have serial monitor open
    Serial.print("Visitor password after over-writing from EEPROM:");
    Serial.println(password_array);
    Serial.print("Master password after over-writing from EEPROM:");
    Serial.println(master_password_array);
  }
  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  delay(3000);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");  
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    timeClient.begin();
    timeClient.update();
    Serial.println(timeClient.getFormattedTime());
  }
}


void loop() // this is the loop that runs all the time
{
  doKeypad();
  ESP.wdtFeed();
}


void doKeypad() {
  char key = kpd.getKey();
  if (key) {
    Serial.print("Pressed Key: ");
    Serial.println(key);
        input_array[input_pos] = key;
    input_pos = (input_pos + 1) % input_array_length;
    
    // Check for guest password
    int password_correct = 1;
    for (int i = 0; i < password_length; i++) { // Check each Password Digit againt the last pressed buttons
      if (input_array[(input_pos + i + input_array_length - password_length) %  input_array_length] != password_array[i]) { // "input_array_length" is added again to make sure we don't take the mod of a negative number
        password_correct = 0;
      }
    }

    // Check Guest Password, if correct, Open Door, and check wifi connected
    if (password_correct == 1) {
      Serial.println("Correct Visitor Password");
      // Open Door, connect wifi if not connected
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, password);
      }
      _openDoor();
      // Close door
      delay(door_open_time);
      _closeDoor();

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        timeClient.begin();
        timeClient.update();
        Serial.println(timeClient.getFormattedTime());
        Serial.println();
      }
    }

    // Check master password, if correct, open door, and check wifi is connected
    int master_password_correct = 1;
    for (int i = 0; i < master_password_length; i++) {
      if (input_array[(input_pos + i + input_array_length - master_password_length) %  input_array_length] != master_password_array[i]) {
        master_password_correct = 0;
      }
    }
    if (master_password_correct == 1) {
      Serial.println("Correct Master Password");
      // Open Door, connect wifi if not connected
      digitalWrite(pin_door, LOW);
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, password);
      }
      // Close door      
      delay(door_open_time);
      digitalWrite(pin_door, HIGH);
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        timeClient.begin();
        timeClient.update();
        Serial.println(timeClient.getFormattedTime());
        Serial.println();
      }
    }

    // Check for diagnostics password
    int diagnostics_password_correct = 1;
    for (int i = 0; i < diagnostics_password_length; i++) {
      if (input_array[(input_pos + i + input_array_length - diagnostics_password_length) %  input_array_length] != diagnostics_password_array[i]) {
        diagnostics_password_correct = 0;
      }
    }
    if (diagnostics_password_correct == 1) {
      Serial.println("Correct Diagnostics Password");
      Serial.println(diagnostics_password_array);
      Serial.println(); 
      Serial.println("Visitor Password Array");
      Serial.println(password_array);           
      Serial.println(); 
      Serial.println("Master Password Array");
      Serial.println(master_password_array);
      Serial.println(); 
      Serial.println("Change Password Array");
      Serial.println(change_password_array);
      Serial.println();
      timeClient.update();
      Serial.println(timeClient.getFormattedTime());
    }

    // Check for password change code
    int change_password_correct = 1;
    for (int i = 0; i < change_password_length; i++) {
      if (input_array[(input_pos + i + input_array_length - change_password_length) %  input_array_length] != change_password_array[i]) {
        change_password_correct = 0;
      }
    }

    // if password change is enabled, open door three times quickly so user knows they are changing the code, then once visitor code is changed, five times.
    // after the 5 opens/buzzes, user must enter master code, then the door will open/buzz 7 times
    if (change_password_correct == 1) {
      Serial.println("Wait 3 buzzes, then enter new visitor password");
      for (int i=0; i < 4; i++) {
        delay(door_open_time / 50);
        digitalWrite(pin_door, LOW);
        delay(door_open_time / 50);
        digitalWrite(pin_door, HIGH);
      }
      
      Serial.print("Old visitors code: ");
      Serial.println(password_array);
      
      for (int i = 0; i < password_length; i++) {
        key = 0;
        while (key == 0) {
          key = kpd.getKey();
          ESP.wdtFeed();
        }
        password_array[i] = key;
        Serial.print(key);
      }
      Serial.println();
      Serial.println("The Five buzzes confirm that visitor password is saved");
      Serial.print("New visitor password: ");
      Serial.println(password_array);
      // writes each digit to EEPROM
      for ( int i = 0; i < password_length; ++i ) {
        Serial.print("count is: ");
        Serial.println(i);
        EEPROM.write(i, password_array[i]);
        EEPROM.commit();
      }
      
      // Five buzzes
      for (int i=0; i < 6; i++) {
        delay(door_open_time / 50);
        digitalWrite(pin_door, LOW);
        delay(door_open_time / 50);
        digitalWrite(pin_door, HIGH);
      }

      // Enter Master Password
      Serial.print("Old Master Password: ");
      Serial.println(master_password_array);
      Serial.println("Now enter new master password");   
      for (int i = 0; i < master_password_length; i++) {
        key = 0;
        while (key == 0) {
          key = kpd.getKey();
          ESP.wdtFeed();
        }
        master_password_array[i] = key;
        
        Serial.print(key);
      }
      Serial.println();
      Serial.println("The Nine buzzes confirm that new master password is saved");
      Serial.print("New Master Password: ");
      Serial.println(master_password_array);
      // writes each digit to EEPROM
      for ( int i = 0; i < master_password_length; ++i ) {
        EEPROM.write(i + password_length, master_password_array[i]);
        EEPROM.commit();
      }
      for (int i=0; i < 10; i++) {
        delay(door_open_time / 50);
        digitalWrite(pin_door, LOW);
        delay(door_open_time / 50);
        digitalWrite(pin_door, HIGH);
      }
    }
  }
}



