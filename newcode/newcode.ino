#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

// Fingerprint sensor on pins 2 (RX) and 3 (TX)
SoftwareSerial fingerSerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// Bluetooth HC-05 on pins 10 (TX) and 11 (RX)
SoftwareSerial BTSerial(10, 11);

// Define users
struct User {
  uint8_t id;
  const char* uname;
};

User users[] = {
  {1, "Janice"},
  {2, "Bob"},
 {3,"Bronil"} 
};

const int numUsers = sizeof(users) / sizeof(users[0]);

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Sensor OK");
  } else {
    Serial.println("Sensor FAIL");
    while (1); // Halt
  }

  Serial.println("Type 'ready' to scan...");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("ready")) {
      Serial.println("Place your finger...");
      int attempts = 3;
      while (attempts--) {
        int id = getFingerprintID();
        if (id > 0) {
          const User* user = checkAuthorization(id);
          if (user) {
            // Send username to server
            String sendLine = "SEND:" + String(user->uname);
            Serial.println(sendLine); // To server
            // Wait for URL from server
            waitForURLResponse();
          } else {
            Serial.println("Unrecognized fingerprint.");
          }
          break;
        } else {
          Serial.println("Try again...");
          delay(2000);
        }
      }
      Serial.println("Type 'ready' to scan...");
    }
  }
}

// === Wait for URL from server and forward to Bluetooth ===
void waitForURLResponse() {
  unsigned long start = millis();
  while (millis() - start < 5000) {
    if (Serial.available()) {
      String response = Serial.readStringUntil('\n');
      response.trim();

      if (response.startsWith("URL:")) {
        String url = response.substring(4);
        String fullUrl = "http://192.168.7.38:3000" + url;
        Serial.print("Received from server: ");
        Serial.println(fullUrl);

        BTSerial.println("Opening Document...");
        BTSerial.println(fullUrl);
      } else {
        Serial.println("Unknown server response: " + response);
      }
      break;
    }
  }
}

// === Fingerprint logic ===
int getFingerprintID() {
  if (finger.getImage() != FINGERPRINT_OK) return -1;
  if (finger.image2Tz() != FINGERPRINT_OK) return -1;
  if (finger.fingerSearch() != FINGERPRINT_OK) return -1;

  return finger.fingerID;
}

const User* checkAuthorization(uint8_t id) {
  for (int i = 0; i < numUsers; i++) {
    if (users[i].id == id) return &users[i];
  }
  return NULL;
}
