// this is in hope that all will workout at the end

// lcd pinout
// vcc - 3v
// gnd - g
// sda - d2
// scl - d1

// rfid pinout
// vcc - 3v
// grn - g
// rst - d3
// ssn - d4
// mosi - d7
// miso - d6
// sck - d5
// irq - none

#include <LiquidCrystal_I2C.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// some logic global variables
const int DOOR_MAX_ESSAYS = 3;
int DOOR_WRONG_ESSAYS = 0;
bool DOOR_CAN_SCAN = true;
bool DOOR_STATUS = false;

// this logs data to the serial monitor
void log(const char* message){
  Serial.println(message);
}

// set up the LCD and hope that all will work at the end
const int LCD_STATE_IDLE = 0;
const int LCD_STATE_SCANNED_VALID = 1;
const int LCD_STATE_SCANNED_INVALID = 2;
const int LCD_STATE_TOO_MANY_ATTEMPTS = 3;
const int LCD_STATE_DOOR_OPEN = 4;
const int LCD_STATE_BOOTING = 5;
const int LCD_STATUS_ANALYZING = 6;
const int LCD_STATE_CLOSING = 7;
int LCD_INIT_STATUS = 0;
int LCD_TRIES_LEFT = 3;
// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 4;
// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

// this will init the lcd screen
void lcd_init(){
    // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
}

// this will write to the screen
void lcd_write(const char* text, int lin, int col){
   // set cursor to first column, first row
  lcd.setCursor(col, lin);
  // print message
  lcd.print(text);
}

// this will update the lcd bassed on the device status
void lcd_set_state(int LCD_STATUS){
  switch(LCD_STATUS){
    case 0:
      log("idle state selected.");
      lcd.clear();
      lcd_write("Smart Door Ready", 0, 0);
      lcd_write("Scan your card...", 1, 0);
      lcd_write("Tries left:", 2, 0);
      lcd_write(String(DOOR_MAX_ESSAYS - DOOR_WRONG_ESSAYS).c_str(), 2, 12);
      LCD_INIT_STATUS = 0;

      //lcd_write((char*) LCD_TRIES_LEFT, 2, 13);
      break;
    case 1:
      // scanned valid state
      log("scanned valid state selected.");
      lcd.clear();
      lcd_write("Access Granted", 0, 0);
      lcd_write("Welcome!", 1, 0);
      lcd_write("Door is unlocking...", 2, 0);
      DOOR_WRONG_ESSAYS = 0;
      DOOR_STATUS = false;
      break;
    case 2:
      // scanned invalid state
      log("scanned invalid state selected");
      lcd.clear();
      lcd_write("Access Denied", 0, 0);
      lcd_write("Invalid card", 1, 0);
      lcd_write("Tries left:", 2, 0);
      lcd_write(String(DOOR_MAX_ESSAYS - DOOR_WRONG_ESSAYS).c_str(), 2, 12);
      //lcd_write((char*) LCD_TRIES_LEFT, 2, 13);
      break;
    case 3:
      // too many attempts
      log("too many attempts state selected");
      lcd.clear();
      lcd_write("Access Blocked!", 0, 0);
      lcd_write("Too many tries.", 1, 0);
      for (int i = 30; i > 9; i--) {
        lcd.setCursor(0, 2);
        lcd_write("Wait ", 2, 0);
        lcd_write(String(i).c_str(), 2, 5);
        lcd_write(" sec...   ", 2 , 7);  // Spaces to clear old text
        delay(1000); // wait 1 second
      }
      lcd.clear();
      lcd_write("Access Blocked!", 0, 0);
      lcd_write("Too many tries.", 1, 0);
      for (int i = 9; i > 0; i--) {
        lcd.setCursor(0, 2);
        lcd_write("Wait ", 2, 0);
        lcd_write(String(i).c_str(), 2, 5);
        lcd_write(" sec...   ", 2 , 6);  // Spaces to clear old text
        delay(1000); // wait 1 second
      }
      DOOR_WRONG_ESSAYS = 0;
      break;
    case 4:
      // door open
      log("door open state selected");
      lcd.clear();
      lcd_write("Door Open", 0, 0);
      lcd_write("use it while you can!", 1, 0);
      
      break;
    case 5:
      // device booting state
      lcd.clear();
      lcd_write("Initializing....", 0, 0);
      lcd_write("Please wait...", 1, 0);
      break;
    case 6:
      // device booting state
      lcd.clear();
      lcd_write("Analyzing data....", 0, 0);
      lcd_write("Please wait...", 1, 0);
      break;
    case 7:
      lcd_write("Door Closing", 0, 0);
      lcd_write("empty the way!", 1, 0);
      for (int i = 5; i > 0; i--) {
        lcd.setCursor(0, 2);
        lcd_write("Door closing in ", 2, 0);
        lcd_write(String(i).c_str(), 2, 15);
        lcd_write(" sec...   ", 2 , 6);
      }
      DOOR_STATUS = true;
      break;
    default:
      log("nothing selected as a state.");
  }
}


// set up the rfid stuff

// Define the pin for SS (SDA) and RST
#define SS_PIN   D4  // SDA / SS pin (GPIO2)
#define RST_PIN  D3  // Reset pin (GPIO0)

// define the uid string
String RFID_UID = "";
bool RFID_IS_SCANNED = false;
String RFID_UID_VALID = "6abe0cbf";

// Define SPI driver with pin configuration
MFRC522DriverPinSimple ss_pin(SS_PIN);
MFRC522DriverSPI driver{ss_pin}; // SPI driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance


void rfid_init(){
  mfrc522.PCD_Init();    // Init MFRC522 board
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.
}

void rfid_read(){
  // Reset the loop if no new card present on the sensor/reader
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // set the lcd screen to analyzing data
  lcd_set_state(LCD_STATUS_ANALYZING);
  // Save the UID on a String variable
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      RFID_UID += "0"; 
    }
    RFID_UID += String(mfrc522.uid.uidByte[i], HEX);
  }

  log(RFID_UID.c_str());

  RFID_IS_SCANNED = true;

  delay(1000);  // Wait for 2 seconds before next card read
}

bool rfid_check(){
  if(!RFID_UID_VALID.compareTo(RFID_UID)){
    DOOR_WRONG_ESSAYS++;
    return false;
  }
  return true;
}

void rfid_uid_clear(){
  RFID_UID.clear();
}


// this is the servo stuff




const char* ssid = "6716";
const char* password = "qwerty12345";

void connect_wifi(){
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());
}

void put_door_status(){
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url;
    // You have same value for both states in your code - maybe that's a bug?
    if(DOOR_STATUS){
      url = "http://13.61.16.167/door/doorState/set?val=true";
    }else{
      url = "http://13.61.16.167/door/doorState/set?val=false";
    }
      
    WiFiClient client;
    http.begin(client, url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("PUT Response:");
      Serial.println(response);
    } else {
      Serial.print("PUT Error: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected for PUT");
  }
}

void setup(){
  // init the serial for debuging
  Serial.begin(115200);
  // init the lcd and set the idle state
  lcd_init();
  //lcd_set_state(LCD_STATUS);
  // init the RFID
  rfid_init();
  // init the servo
  //servo_init();
  // set the init screen text
  lcd_set_state(LCD_STATE_BOOTING);
  lcd_set_state(LCD_STATE_IDLE);

  connect_wifi();

}



void loop(){
  // Check if the card is scanned every time loop() runs
  if (LCD_INIT_STATUS)
    lcd_set_state(LCD_STATE_IDLE);

  if (!RFID_IS_SCANNED) {
    rfid_read();  // Scan the card without blocking
  }
  // If card is scanned, process the UID
  if (RFID_IS_SCANNED) {
    if(rfid_check()){  // Check if the scanned card is valid or invalid
      lcd_set_state(LCD_STATE_SCANNED_VALID);
      DOOR_STATUS = true;
      put_door_status();
      lcd_set_state(LCD_STATE_DOOR_OPEN);
      RFID_IS_SCANNED = true;
      while(true){
        Serial.print("scanning");
        if (!RFID_IS_SCANNED) {
          rfid_read();  // Scan the card without blocking
        }
        if (RFID_IS_SCANNED) {
          if(rfid_check()){  // Check if the scanned card is valid or invalid
            lcd_set_state(LCD_STATE_CLOSING);
            put_door_status();
            break;
          }
        }
      } 
    }else {
      if(DOOR_WRONG_ESSAYS == DOOR_MAX_ESSAYS){
        lcd_set_state(LCD_STATE_TOO_MANY_ATTEMPTS);
      } else {
        lcd_set_state(LCD_STATE_SCANNED_INVALID);
        delay(3000);
      }

    }
    RFID_IS_SCANNED = false;  // Reset the scanned flag for next scan
    rfid_uid_clear();
    LCD_INIT_STATUS = 1; 
  }
}
