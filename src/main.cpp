#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <LoRa.h>

// ============ CONFIGURATION ============
// Change IS_TRANSMITTER to false on one unit to make it a receiver
#define IS_TRANSMITTER false

// LoRa pins (adjust to match your wiring)
#define LORA_SS 15      // Chip select
#define LORA_RST 2      // Reset
#define LORA_DIO0 3     // Interrupt pin (DIO0/G0)

// LCD config
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// LoRa config
#define LORA_FREQ 915E6 // 915 MHz

// Create LCD object
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// State variables
bool loRaOk = false;
unsigned long lastTxTime = 0;
const unsigned long TX_INTERVAL = 2000; // Send every 2 seconds

void initLCD()
{
    Serial.println("Initializing LCD...");
    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    Serial.println("LCD initialized");
}

void initLoRa()
{
    Serial.println("Initializing LoRa...");
    
    // Reset LoRa module
    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, HIGH);
    delay(50);
    digitalWrite(LORA_RST, LOW);
    delay(50);
    digitalWrite(LORA_RST, HIGH);
    delay(50);
    
    SPI.begin();
    pinMode(LORA_SS, OUTPUT);
    digitalWrite(LORA_SS, HIGH);
    
    // Set pins and initialize
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    
    Serial.print("LoRa pins - SS:");
    Serial.print(LORA_SS);
    Serial.print(" RST:");
    Serial.print(LORA_RST);
    Serial.print(" DIO0:");
    Serial.println(LORA_DIO0);
    
    if (!LoRa.begin(LORA_FREQ))
    {
        Serial.println("LoRa FAILED");
        loRaOk = false;
        lcd.setCursor(0, 0);
        lcd.print("LoRa: FAILED");
    }
    else
    {
        Serial.println("LoRa OK");
        loRaOk = true;
        lcd.setCursor(0, 0);
        if (IS_TRANSMITTER)
            lcd.print("TX Mode");
        else
            lcd.print("RX Mode");
    }
}

void displayOnLCD(const char *line1, const char *line2 = NULL)
{
    lcd.clear();
    if (line1)
    {
        lcd.setCursor(0, 0);
        lcd.print(line1);
    }
    if (line2)
    {
        lcd.setCursor(0, 1);
        lcd.print(line2);
    }
}

void setup()
{
    delay(2000);
    Serial.begin(9600);
    delay(500);
    
    Serial.println("\n\n=== LoRa LCD Test Starting ===");
    if (IS_TRANSMITTER)
        Serial.println("Mode: TRANSMITTER");
    else
        Serial.println("Mode: RECEIVER");
    
    initLCD();
    initLoRa();
    
    if (loRaOk)
    {
        displayOnLCD(IS_TRANSMITTER ? "TX: Ready" : "RX: Ready");
    }
}

void loop()
{
    if (!loRaOk)
    {
        delay(100);
        return;
    }
    
    if (IS_TRANSMITTER)
    {
        // Transmitter: send text every 2 seconds
        unsigned long now = millis();
        if (now - lastTxTime >= TX_INTERVAL)
        {
            lastTxTime = now;
            
            // Build message with counter
            static int counter = 0;
            char msg[20];
            snprintf(msg, sizeof(msg), "TX: %d", counter++);
            
            Serial.print("Sending: ");
            Serial.println(msg);
            
            LoRa.beginPacket();
            LoRa.print(msg);
            LoRa.endPacket();
            
            // Display on local LCD
            displayOnLCD("TX Ready", msg);
        }
    }
    else
    {
        // Receiver: listen for incoming packets
        int packetSize = LoRa.parsePacket();
        if (packetSize)
        {
            // Read packet into buffer
            char buffer[64] = {0};
            int idx = 0;
            while (LoRa.available() && idx < (int)sizeof(buffer) - 1)
            {
                buffer[idx++] = (char)LoRa.read();
            }
            buffer[idx] = '\0';
            
            Serial.print("Received: ");
            Serial.println(buffer);
            
            // Display on LCD
            displayOnLCD("RX:", buffer);
            
            // Print signal strength
            int rssi = LoRa.packetRssi();
            Serial.print("RSSI: ");
            Serial.println(rssi);
        }
    }
    
    delay(100); // Small delay to prevent overwhelming the loop
}
