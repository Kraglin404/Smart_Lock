#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>

// --- Configuration ---
const char* ssid = "Hehe";           // Your WiFi SSID
const char* password = "12345679";   // Your WiFi Password

String correctPassword = "5678";     // Default system password
String enteredPassword = "";
int wrongAttempts = 0;
bool locked = false;
unsigned long lockStartTime = 0;
const int lockDuration = 60;         // Lockout in seconds

// --- Hardware Pins ---
int servoPin = 18;
int buzzerPin = 19;

// --- Objects ---
WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;

// Keypad Setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*', '0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26}; 
byte colPins[COLS] = {27, 14, 12, 13}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- Functions ---

void beep() {
  digitalWrite(buzzerPin, HIGH);
  delay(300);
  digitalWrite(buzzerPin, LOW);
}

void checkPassword(String input) {
  if (locked) return;

  lcd.clear();
  if (input == correctPassword) {
    lcd.print("Access Granted");
    wrongAttempts = 0;
    myServo.write(90); // Unlocked position
    delay(5000);
    myServo.write(0);  // Locked position
    lcd.clear();
    lcd.print("Enter Password");
  } else {
    lcd.print("Wrong Password");
    beep();
    wrongAttempts++;
    
    if (wrongAttempts >= 3) {
      locked = true;
      lockStartTime = millis();
      // Triple beep alarm
      for(int i=0; i<3; i++) {
        beep();
        delay(200);
      }
    } else {
      delay(2000);
      lcd.clear();
      lcd.print("Enter Password");
    }
  }
}

// --- Web Server Handlers ---

void handleRoot() {
  String page = R"rawliteral(
    <!DOCTYPE html><html><head><title>Smart Door Lock</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial; background: linear-gradient(135deg, #4facfe, #00f2fe); text-align: center; color: white; }
      .container { margin-top: 60px; }
      .card { background: rgba(255,255,255,0.2); padding: 30px; border-radius: 20px; width: 300px; margin: auto; box-shadow: 0px 10px 25px rgba(0,0,0,0.3); }
      h2 { font-size: 30px; }
      input[type=password] { padding: 12px; width: 200px; font-size: 18px; border: none; border-radius: 10px; text-align: center; }
      button { padding: 12px 25px; margin-top: 15px; font-size: 18px; border: none; border-radius: 10px; background: #28a745; color: white; cursor: pointer; }
      button:hover { background: #218838; }
    </style></head><body>
    <div class="container"><div class="card">
      <h2>Smart Door Lock</h2>
      <form action="/unlock"><p>Enter Password</p>
      <input type="password" name="pass"><br>
      <button type="submit">Unlock Door</button>
      </form></div></div></body></html>
  )rawliteral";
  server.send(200, "text/html", page);
}

void handleUnlock() {
  String pass = server.arg("pass");
  checkPassword(pass);
  server.sendHeader("Location", "/");
  server.send(303);
}

// --- Standard Arduino Setup & Loop ---

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(buzzerPin, OUTPUT);

  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.print("WiFi Connected");
  delay(2000);
  lcd.clear();
  lcd.print("Enter Password");

  server.on("/", handleRoot);
  server.on("/unlock", handleUnlock);
  server.begin();

  myServo.attach(servoPin);
  myServo.write(0);
}

void loop() {
  server.handleClient();

  if (locked) {
    int elapsed = (millis() - lockStartTime) / 1000;
    int remaining = lockDuration - elapsed;
    
    if (remaining <= 0) {
      locked = false;
      wrongAttempts = 0;
      lcd.clear();
      lcd.print("Enter Password");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("System Locked ");
      lcd.setCursor(0, 1);
      lcd.print("Try after: ");
      lcd.print(remaining);
      lcd.print("s ");
      delay(1000);
    }
    return;
  }

  char key = keypad.getKey();
  if (key) {
    if (key == '#') { // Submit password
      checkPassword(enteredPassword);
      enteredPassword = "";
    } else if (key == '*') { // Clear input
      enteredPassword = "";
      lcd.clear();
      lcd.print("Enter Password");
    } else {
      if (enteredPassword.length() < 4) {
        enteredPassword += key;
        lcd.setCursor(enteredPassword.length() - 1, 1);
        lcd.print("*"); // Prevent shoulder-surfing
      }
    }
  }
}