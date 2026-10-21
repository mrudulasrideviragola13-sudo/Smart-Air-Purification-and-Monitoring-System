#define BLYNK_TEMPLATE_ID "TMPL3-PdRTJM8"
#define BLYNK_TEMPLATE_NAME "Air Monitoring System"
  // paste your Template Name
#define BLYNK_AUTH_TOKEN "aF8fW0Kpt-Qa6Uuvi7BJO1isJKkxBRaN" 

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// WiFi Credentials
// =====================================================
char ssid[] = "Redmi 13C 5G";
char pass[] = "12345678M";

// =====================================================
// LCD
// =====================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);
// If LCD does not work, try 0x3F

// =====================================================
// Pin Definitions
// =====================================================
#define MQ135_PIN 34
#define MQ7_PIN   35
#define RELAY_PIN 26

// =====================================================
// Blynk Timer
// =====================================================
BlynkTimer timer;


// =====================================================
// READ SENSOR AVERAGE
// Takes 10 readings and calculates average
// This reduces rapid fluctuations
// =====================================================
int readAverage(int pin)
{
  long total = 0;

  for (int i = 0; i < 10; i++)
  {
    total += analogRead(pin);
    delay(10);
  }

  return total / 10;
}


// =====================================================
// MQ135 STATUS
// Raw ESP32 ADC range: 0 - 4095
// =====================================================
String getMQ135Status(int value)
{
  if (value <= 1000)
  {
    return "Clear";
  }
  else if (value <= 2000)
  {
    return "Moderate";
  }
  else if (value <= 3000)
  {
    return "Poor";
  }
  else
  {
    return "Danger";
  }
}


// =====================================================
// MQ7 STATUS
// Raw ESP32 ADC range: 0 - 4095
// =====================================================
String getMQ7Status(int value)
{
  if (value <= 1000)
  {
    return "Clear";
  }
  else if (value <= 2000)
  {
    return "Moderate";
  }
  else if (value <= 3000)
  {
    return "Poor";
  }
  else
  {
    return "Danger";
  }
}


// =====================================================
// MAIN SENSOR FUNCTION
// =====================================================
void sendSensorData()
{
  // ---------------------------------------------------
  // Read averaged sensor values
  // ---------------------------------------------------
  int mq135Value = readAverage(MQ135_PIN);
  int mq7Value   = readAverage(MQ7_PIN);


  // ---------------------------------------------------
  // Get individual sensor status
  // ---------------------------------------------------
  String status135 = getMQ135Status(mq135Value);
  String status7   = getMQ7Status(mq7Value);


  // ---------------------------------------------------
  // Determine overall air quality
  // The WORST sensor condition wins
  // ---------------------------------------------------
  String overallStatus = "Clear";

  if (status135 == "Danger" || status7 == "Danger")
  {
    overallStatus = "Danger";
  }
  else if (status135 == "Poor" || status7 == "Poor")
  {
    overallStatus = "Poor";
  }
  else if (status135 == "Moderate" || status7 == "Moderate")
  {
    overallStatus = "Moderate";
  }
  else
  {
    overallStatus = "Clear";
  }


  // ---------------------------------------------------
  // FAN CONTROL
  //
  // CLEAR    -> OFF
  // MODERATE -> OFF
  // POOR     -> ON
  // DANGER   -> ON
  //
  // Relay is ACTIVE-LOW:
  // LOW  = ON
  // HIGH = OFF
  // ---------------------------------------------------
  bool airIsBad = false;

  if (overallStatus == "Poor" ||
      overallStatus == "Danger")
  {
    airIsBad = true;
  }


  if (airIsBad)
  {
    digitalWrite(RELAY_PIN, LOW);   // FAN ON
  }
  else
  {
    digitalWrite(RELAY_PIN, HIGH);  // FAN OFF
  }


  // ---------------------------------------------------
  // SERIAL MONITOR
  // ---------------------------------------------------
  Serial.println("--------------------------------");

  Serial.print("MQ135 Value: ");
  Serial.print(mq135Value);
  Serial.print(" | Status: ");
  Serial.println(status135);

  Serial.print("MQ7 Value:   ");
  Serial.print(mq7Value);
  Serial.print(" | Status: ");
  Serial.println(status7);

  Serial.print("Overall: ");
  Serial.println(overallStatus);

  Serial.print("Fan: ");
  if (airIsBad)
  {
    Serial.println("ON");
  }
  else
  {
    Serial.println("OFF");
  }


  // ---------------------------------------------------
  // LCD DISPLAY
  // ---------------------------------------------------

  // First line
  lcd.setCursor(0, 0);

  lcd.print("AQ:");
  lcd.print(mq135Value);

  lcd.print(" CO:");
  lcd.print(mq7Value);

  lcd.print("   ");


  // Second line
  lcd.setCursor(0, 1);

  lcd.print("Status:");
  lcd.print(overallStatus);

  lcd.print("       ");


  // ---------------------------------------------------
  // BLYNK DASHBOARD
  // ---------------------------------------------------
  if (Blynk.connected())
  {
    // V0 = MQ135 raw value
    Blynk.virtualWrite(V0, mq135Value);

    // V1 = MQ7 raw value
    Blynk.virtualWrite(V1, mq7Value);

    // V2 = Overall status
    Blynk.virtualWrite(V2, overallStatus);

    // V3 = Fan status
    // 1 = ON
    // 0 = OFF
    Blynk.virtualWrite(V3, airIsBad ? 1 : 0);
  }
}


// =====================================================
// SETUP
// =====================================================
void setup()
{
  // ---------------------------------------------------
  // Serial
  // ---------------------------------------------------
  Serial.begin(115200);

  // ---------------------------------------------------
  // I2C
  // ---------------------------------------------------
  Wire.begin();

  // ---------------------------------------------------
  // Relay
  // ---------------------------------------------------
  pinMode(RELAY_PIN, OUTPUT);

  // Active-LOW relay
  // Start with fan OFF
  digitalWrite(RELAY_PIN, HIGH);


  // ---------------------------------------------------
  // LCD
  // ---------------------------------------------------
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Air Purifier");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  delay(1500);


  // ---------------------------------------------------
  // WiFi
  // ---------------------------------------------------
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  Serial.println();
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, pass);

  int wifiAttempts = 0;

  while (WiFi.status() != WL_CONNECTED &&
         wifiAttempts < 20)
  {
    delay(500);

    Serial.print(".");

    wifiAttempts++;
  }


  // ---------------------------------------------------
  // Check WiFi
  // ---------------------------------------------------
  lcd.clear();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println();
    Serial.println("WiFi Connected!");

    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");

    // -------------------------------------------------
    // Blynk
    // -------------------------------------------------
    Blynk.config(BLYNK_AUTH_TOKEN);

    Serial.println("Connecting to Blynk...");

    if (Blynk.connect(5000))
    {
      Serial.println("Blynk Connected!");

      lcd.setCursor(0, 1);
      lcd.print("Blynk Connected");
    }
    else
    {
      Serial.println("Blynk connection failed");

      lcd.setCursor(0, 1);
      lcd.print("Blynk Failed");
    }
  }
  else
  {
    Serial.println();
    Serial.println("WiFi connection failed.");
    Serial.println("Running in offline mode.");

    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");

    lcd.setCursor(0, 1);
    lcd.print("Offline Mode");
  }


  delay(1500);

  lcd.clear();


  // ---------------------------------------------------
  // Read sensors every 2 seconds
  // ---------------------------------------------------
  timer.setInterval(2000L, sendSensorData);
}


// =====================================================
// LOOP
// =====================================================
void loop()
{
  // ---------------------------------------------------
  // Blynk
  // ---------------------------------------------------
  if (WiFi.status() == WL_CONNECTED)
  {
    Blynk.run();
  }

  // ---------------------------------------------------
  // Timer
  // ---------------------------------------------------
  timer.run();
}
