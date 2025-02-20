#include <WiFi.h> // Library part
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Default LCD address LCD display size

#define dhtPin 5
#define rainSensor 34
#define buzzer 12

DHT dht(dhtPin, DHT11);

void readData() { // Functions
  float h = dht.readHumidity() + 10; // Float, int data types
  int rain = analogRead(rainSensor);
  int mapped = map(rain, 0, 4095, 100, 0);

  // Logging to serial monitor
  Serial.println("Data Readings:");
  Serial.println("Humidity: " + String(h) + " %");
  Serial.println("Rain: " + String(mapped) + " %");
}

void display() { // Can be displayed also through serial monitor
  float h = dht.readHumidity() + 10; // To read data from sensor to LCD display
  int rain = analogRead(rainSensor);
  int mapped = map(rain, 0, 4095, 100, 0);

  lcd.setCursor(0, 0);
  lcd.print("H:" + String(h) + " %");

  lcd.setCursor(0, 1);
  lcd.print("Rain:" + String(mapped) + " %");

  delay(2000); // Data showing in LCD display within 2 seconds = 2000 milliseconds
  digitalWrite(buzzer, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rain Value: " + String(mapped) + " %");
  if (mapped >= 10) { // If rain value is equal to or greater than 10% the buzzer is activated
    digitalWrite(buzzer, HIGH);
    lcd.setCursor(0, 1);
    lcd.print("Rain Detected");
  }

  delay(2000);
  lcd.clear();
}

// From here code starts working after function creation part
void setup() {
  Serial.begin(115200); // Bandwidth

  lcd.init(); // Initialization process
  lcd.backlight(); // To power on the backlight

  lcd.setCursor(0, 0); // Startup phase as coded
  lcd.print("LIVE WEATHER");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);

  dht.begin();

  pinMode(rainSensor, INPUT); // Read data from rain sensor pin 34
  pinMode(buzzer, OUTPUT); // Write means on and off and data write off from pin no 12
}

void loop() {
  readData();
  display();
  delay(100); // Run the loop with a slight delay
}
