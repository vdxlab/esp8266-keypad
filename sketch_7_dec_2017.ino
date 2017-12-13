#include <dummy.h>

#include <EEPROM.h>
#include <Keypad.h>

#define pin_door D0 // Connected to active low relay board to door opener
bool panelActivo = true;

const byte ROWS = 4; // Four rows - this is the 1,4,7,* and 2,5,8,0, etc
const byte COLS = 3; // Three columns - this is the 123 and 456 and 789 and *0#
char claveCSO[4] = {'1', '2', '3', '4'}; // el codigo para abrir la puerta clave de entrada sujeto a horario del cso
char claveVIV[4] = {'5', '6', '7', '8'}; // el codigo para abrir la puerta clave de entrada 7 dias 24 horas.
//int posActual = 0; // this is the integer that rises as each of the keys is pressed correctly. a 4-digit code like 1,3,2,5 rises to 4
int CSO = 0; //aquest marcador aumenta cada cop que una tecla del codi del CSO es prem correctament.
int VIV = 0; //aquest marcador aumenta cada cop que una tecla del codi de lA VIVenda es prem correctament.


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

// ToDo: Store user settable password in non volatile memory (Maybe use arduino EEPROM emulation)
// Sorry this is some spaggethi code, didn't have the time to clean it up properly ;)
// DONE:  Resistors( around 3k) from each row pin to 3.3V to compensate for the signal noise in long cable

char password_array[5] = {'1', '2', '3', '4', '5'}; // User Settable Password
int password_length = 5;

char change_password_array[6] = {'5', '6', '7', '8', '9', '0'}; // Enter this combination, wait for buzzer to stop and enter new 4 digit password
int change_password_length = 6;

char master_password_array[5] = {'*', '#', '8', '2', '*'}; // Master Password
int master_password_length = 5;

char diagnostics_password_array[5] = {'2', '2', '2', '2', '*'}; // spits out diagnostics
int diagnostics_password_length = 5;

const int door_open_time = 2000;

char input_array[8]; // Circular Buffer for the last pressed buttons
char input_array_length = 8;
int input_pos = 0; // Points to next location in circular buffer to be pressed

void setup() {
  // put your setup code here, to run once:
  pinMode(pin_door, OUTPUT);
  digitalWrite(pin_door, LOW);
  delay(10);
  digitalWrite(pin_door, HIGH);
  delay(10);
  Serial.begin(9600); //Serial communication to display data
  ESP.wdtDisable();  // turns off watchdog?
  EEPROM.begin(512);  //Initialize EEPROM
  // read appropriate byte of the EEPROM.  
  Serial.println(""); //Goto next line, as ESP sends some garbage when you reset it  
  Serial.println("Password array:");
  Serial.println(password_array);
  if ( EEPROM.read ( 0 ) != 0xff ) {     //Read from address 0x00, 0x01, 0x02, etc
    for (int i = 0; i < password_length; ++i ) {
      password_array[i] = EEPROM.read(i);
    }
  Serial.println("Password array:");
  Serial.print(password_array);
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
    
    Serial.print("Lask keys pressed: ");
    input_array[input_pos] = key;
    input_pos = (input_pos + 1) % input_array_length;
    for (int i = 0; i < 8; i++) {
      Serial.print(input_array[i]);
    }
    
    Serial.println(' ');

    // Check for temporary password
    int password_correct = 1;
    for (int i = 0; i < password_length; i++) { // Check each Password Digit againt the last pressed buttons
      if (input_array[(input_pos + i + input_array_length - password_length) %  input_array_length] != password_array[i]) { // "input_array_length" is added again to make sure we don't take the mod of a negative number
        password_correct = 0;
      }
    }
    if (password_correct == 1) {
      Serial.println("Correct Temporary Password");
      digitalWrite(pin_door, LOW);
      delay(door_open_time);
      digitalWrite(pin_door, HIGH);
      }

    // Check for master password
    int master_password_correct = 1;
    for (int i = 0; i < master_password_length; i++) {
      if (input_array[(input_pos + i + input_array_length - master_password_length) %  input_array_length] != master_password_array[i]) {
        master_password_correct = 0;
      }
    }
    if (master_password_correct == 1) {
      Serial.println("Correct Master Password");
      digitalWrite(pin_door, LOW);
      delay(door_open_time);
      digitalWrite(pin_door, HIGH);
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
      Serial.println("Master Password Array");
      Serial.println(master_password_array);
      Serial.println(); 
      Serial.println("Password Array");
      Serial.println(diagnostics_password_array);           
      Serial.println(); 
      Serial.println("Change Password Array");
      Serial.println(change_password_array);
      Serial.println();
      Serial.println("Contents of EEPROM");
      for (int i = 0; i < password_length; ++i ) {
        Serial.print(EEPROM.read(i));
      }      
    }


    // Check for password change code
    int change_password_correct = 1;
    for (int i = 0; i < change_password_length; i++) {
      if (input_array[(input_pos + i + input_array_length - change_password_length) %  input_array_length] != change_password_array[i]) {
        change_password_correct = 0;
      }
    }

    // if password change is enabled, open door three times quickly so user knows they are changing the code, then once code is changed, seven times.
    if (change_password_correct == 1) {
      Serial.println("Wait 3 buzzes, then enter new 5 digit password");
      for (int i=0; i < 4; i++) {
        delay(door_open_time / 50);
        digitalWrite(pin_door, LOW);
        delay(door_open_time / 50);
        digitalWrite(pin_door, HIGH);
      }
      

      Serial.print("New code: ");
      
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
      Serial.println("The Seven buzzes confirm that new code is saved");
      Serial.print("New code: ");
      Serial.println(password_array);
      Serial.println("WTF??");
      Serial.println(password_array);
      for ( int i = 0; i < password_length; ++i ) {
        Serial.print("Writing digit: ");
        Serial.print(i);
        Serial.print(", ");
        Serial.print(password_array[i]);
        EEPROM.write(i, password_array[i]);
        EEPROM.commit();
      }
      for (int i=0; i < 8; i++) {
        delay(door_open_time / 50);
        digitalWrite(pin_door, LOW);
        delay(door_open_time / 50);
        digitalWrite(pin_door, HIGH);
      }
      
    }
  }
}



