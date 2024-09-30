#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <ESP32Servo.h>

// Your Wi-Fi and Firebase configuration
#define WIFI_SSID "Your_WIFI_Name"
#define WIFI_PASSWORD "Your_Password"
#define API_KEY "Your_api"
#define DATABASE_URL "Your_URL"
#define USER_EMAIL "Tester@gmail.com"
#define USER_PASSWORD "qwerty1234"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
Servo Door;

#define CW 14
#define CCW 27
void setup() {
  Serial.begin(115200);

  // Start Wi-Fi connection
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Firebase configuration
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectNetwork(true);
  
  // Set buffer and response sizes
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);

  // Begin Firebase connection
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
  config.timeout.serverResponse = 15 * 1000;

  Door.attach(18);
  Door.write(0);

  pinMode(34, INPUT);
  pinMode(CW, OUTPUT);
  pinMode(CCW, OUTPUT);

  digitalWrite(CW,HIGH);
  digitalWrite(CCW,HIGH);
}

void open_curtain(){
  digitalWrite(CW,LOW);
  digitalWrite(CCW,HIGH);
  delay(1000);
  digitalWrite(CW,HIGH);
  digitalWrite(CCW,HIGH);
}

void close_curtain(){
  digitalWrite(CW,HIGH);
  digitalWrite(CCW,LOW);
  delay(1000);
  digitalWrite(CW,HIGH);
  digitalWrite(CCW,HIGH);
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    Serial.println("Firebase is ready. Reading and writing data...");

    // Read LDR value and set it in Firebase
    bool ldr = digitalRead(34);
    char temp[50];
    if (Firebase.RTDB.setBool(&fbdo, F("/light"), !ldr)) {
      Serial.println("Light status updated successfully.");
    } else {
      Serial.print("Failed to update light status. Reason: ");
      Serial.println(fbdo.errorReason());
      strcpy(temp, fbdo.errorReason().c_str());
    }
    bool intru;
    // Read intruder status from Firebase
    if (Firebase.RTDB.getBool(&fbdo, F("/intruder"))) {
      intru = fbdo.boolData();
      Serial.print("Intruder status: ");
      Serial.println(intru);
    } else {
      Serial.print("Failed to get intruder status. Reason: ");
      Serial.println(fbdo.errorReason());
    }
    bool mode = false;
    if (Firebase.RTDB.getBool(&fbdo, F("/mode"))) {
      mode = fbdo.boolData();
    }
    String status;
    if (Firebase.RTDB.getString(&fbdo, F("/status"))) {
     status = fbdo.stringData();
    }

    if(mode){
      bool light = !ldr;
      if(light && status == "Open"){
          close_curtain();
          status = "Close";
          Firebase.RTDB.setString(&fbdo, F("/status"), status);
      }
      else if(!light && status == "Close"){
          open_curtain();
          status = "Open";
          Firebase.RTDB.setString(&fbdo, F("/status"), status);
      }
    }
    else{
      bool toggle = false;
      if (Firebase.RTDB.getBool(&fbdo, F("/toggle"))) toggle = fbdo.boolData();
      if(toggle){
        if(status == "Open"){
          close_curtain();
          status = "Close";
          Firebase.RTDB.setString(&fbdo, F("/status"), status);
          Firebase.RTDB.setBool(&fbdo, F("/status"), false);
        }
        else{
          open_curtain();
          status = "Open";
          Firebase.RTDB.setString(&fbdo, F("/status"), status);
          Firebase.RTDB.setBool(&fbdo, F("/status"), false);
        }
      }
    }

    if (intru){
      Door.write(180);
    }
    else{
      Door.write(0);
    }

    delay(1000);
  }
}
