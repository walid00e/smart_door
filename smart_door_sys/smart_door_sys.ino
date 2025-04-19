#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// set up the wifi stuff
const char* ssid = "6716";
const char* password = "qwerty12345";
// state variables
bool DOOR_FIRST_TIME = true;
bool DOOR_STATUS = false;
Servo myServo;

// connection to wifi
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

// hit the endpoint and get the door status
bool get_door_status(){
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://13.61.16.167/door/doorState/get";
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("GET Response:");
      Serial.println(response);
      if(response == "true")
        return true;
      else
        return false;
    } else {
      Serial.print("GET Error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected for GET");
  }
}

// open the door
void servo_open_door(){
  myServo.write(0); // or any angle to "open" the door
  Serial.println("Door opened");
}

// close the door
void servo_close_door(){
  myServo.write(90); // or any angle to "close" the door
  Serial.println("Door closed");
}

// update the door status
void update_door_irl(){
  bool DOOR_STATUS_ONLINE = get_door_status();
  if(DOOR_STATUS_ONLINE != DOOR_STATUS){
    if(DOOR_STATUS_ONLINE){
      DOOR_STATUS = true;
      servo_open_door();
    } else {
      DOOR_STATUS = false;
      servo_close_door();
    }
  }  
}

// init the servo
void init_servo(){
  myServo.attach(18); // Use your actual servo control pin
  myServo.write(90);   // Start closed
}

void setup() {
  Serial.begin(115200);
  connect_wifi();
  init_servo();
}

void loop() {
  update_door_irl();
}
