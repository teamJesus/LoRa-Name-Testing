#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <LoRa.h>
#include <EEPROM.h>

// ============ CONFIGURATION ============
// Change IS_TRANSMITTER to false on one unit to make it a receiver
#define IS_TRANSMITTER true

// ============ PIN DEFINITIONS ============
// Button pins (active LOW with INPUT_PULLUP, matching Hello World)
#define BTN_1 8
#define BTN_2 4
#define BTN_3 5
#define BTN_4 6
#define BTN_5 7

// Buzzer pin (PWM-capable, matching Hello World)
#define PIN_BUZZER 10

// LCD config (matching Hello World)
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// ============ LORA PINS & CONFIG ============
// LoRa pins (matching Hello World)
#define PIN_LORA_SS 15      // Chip select (D15)
#define PIN_LORA_RST 2      // Reset (D2)
#define PIN_LORA_DIO0 3     // Interrupt pin (DIO0/G0) (D3)

// LoRa config
#define LORA_FREQ 915E6 // 915 MHz

// Serial config
#define BAUD_RATE 9600 // Must match monitor_speed in platformio.ini

// Naming config
#define NAME_MAX_LEN 12
#define NAME_EEPROM_ADDR 0
#define LONG_PRESS_MS 1000
#define DEBOUNCE_MS 20

// Character set: a-z, 0-9 (37 chars, no space to save RAM)
const char CHARSET[] = "abcdefghijklmnopqrstuvwxyz0123456789";
#define CHARSET_LEN 36

// Create LCD object
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// State variables
bool loRaOk = false;
char deviceName[NAME_MAX_LEN + 1];
bool namingMode = false;
int namePos = 0;

// Button state tracking (optimized for RAM)
byte btnState[4] = {HIGH, HIGH, HIGH, HIGH};
byte btnLastState[4] = {HIGH, HIGH, HIGH, HIGH};
unsigned long btnPressTime[4] = {0, 0, 0, 0};
byte btnLongPressHandled[4] = {0, 0, 0, 0};
unsigned long btnLastDebounce[4] = {0, 0, 0, 0};

void loadNameFromEEPROM()
{
    for (int i = 0; i < NAME_MAX_LEN; ++i)
    {
        char c = EEPROM.read(NAME_EEPROM_ADDR + i);
        if (c == 0xFF || c == 0)
            c = ' ';
        deviceName[i] = c;
    }
    deviceName[NAME_MAX_LEN] = '\0';
    Serial.print("Loaded name: ");
    Serial.println(deviceName);
}

void saveNameToEEPROM()
{
    for (int i = 0; i < NAME_MAX_LEN; ++i)
    {
        EEPROM.update(NAME_EEPROM_ADDR + i, deviceName[i]);
    }
    Serial.print("Saved name: ");
    Serial.println(deviceName);
}

void displayNameEditMode()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Edit name");
    lcd.setCursor(0, 1);
    // Display name with cursor under current position
    for (int i = 0; i < NAME_MAX_LEN && i < LCD_COLS; ++i)
    {
        if (i == namePos)
            lcd.print('[');
        else
            lcd.print(deviceName[i]);
    }
}

void displayMainScreen()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Name: ");
    // Display name (trimmed)
    int nameLen = 0;
    while (nameLen < NAME_MAX_LEN && deviceName[nameLen] != ' ')
        nameLen++;
    for (int i = 0; i < nameLen && i < 10; ++i)
        lcd.print(deviceName[i]);
    
    lcd.setCursor(0, 1);
    if (IS_TRANSMITTER)
        lcd.print("Press BTN4 to TX");
    else
        lcd.print("Waiting RX...");
}

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
    pinMode(PIN_LORA_RST, OUTPUT);
    digitalWrite(PIN_LORA_RST, HIGH);
    delay(50);
    digitalWrite(PIN_LORA_RST, LOW);
    delay(50);
    digitalWrite(PIN_LORA_RST, HIGH);
    delay(50);
    
    SPI.begin();
    pinMode(PIN_LORA_SS, OUTPUT);
    digitalWrite(PIN_LORA_SS, HIGH);
    
    // Set pins and initialize
    LoRa.setPins(PIN_LORA_SS, PIN_LORA_RST, PIN_LORA_DIO0);
    
    Serial.print("LoRa pins - SS:");
    Serial.print(PIN_LORA_SS);
    Serial.print(" RST:");
    Serial.print(PIN_LORA_RST);
    Serial.print(" DIO0:");
    Serial.println(PIN_LORA_DIO0);
    
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
    }
}

void setup()
{
    delay(2000);
    Serial.begin(BAUD_RATE);
    delay(500);
    
    Serial.println("\n\n=== LoRa Name Tester Starting ===");
    if (IS_TRANSMITTER)
        Serial.println("Mode: TRANSMITTER");
    else
        Serial.println("Mode: RECEIVER");
    
    // Setup button pins (matching Hello World)
    pinMode(BTN_1, INPUT_PULLUP);
    pinMode(BTN_2, INPUT_PULLUP);
    pinMode(BTN_3, INPUT_PULLUP);
    pinMode(BTN_4, INPUT_PULLUP);
    pinMode(BTN_5, INPUT_PULLUP);
    
    initLCD();
    //loadNameFromEEPROM();
    initLoRa();
    
    if (loRaOk)
    {
        displayMainScreen();
    }
}

void handleButtonPress(int btn)
{
    if (namingMode)
    {
        if (btn == 0) // Button 1: cycle to previous char
        {
            char &ch = deviceName[namePos];
            int idx = -1;
            for (int i = 0; i < CHARSET_LEN; ++i)
            {
                if (CHARSET[i] == ch)
                {
                    idx = i;
                    break;
                }
            }
            idx = (idx - 1 + CHARSET_LEN) % CHARSET_LEN;
            deviceName[namePos] = CHARSET[idx];
            displayNameEditMode();
            Serial.print("Char at pos ");
            Serial.print(namePos);
            Serial.print(": ");
            Serial.println(deviceName[namePos]);
        }
        else if (btn == 1) // Button 2: cycle to next char
        {
            char &ch = deviceName[namePos];
            int idx = -1;
            for (int i = 0; i < CHARSET_LEN; ++i)
            {
                if (CHARSET[i] == ch)
                {
                    idx = i;
                    break;
                }
            }
            idx = (idx + 1) % CHARSET_LEN;
            deviceName[namePos] = CHARSET[idx];
            displayNameEditMode();
            Serial.print("Char at pos ");
            Serial.print(namePos);
            Serial.print(": ");
            Serial.println(deviceName[namePos]);
        }
        else if (btn == 2) // Button 3: move to next position
        {
            namePos = (namePos + 1) % NAME_MAX_LEN;
            displayNameEditMode();
            Serial.print("Moved to position ");
            Serial.println(namePos);
        }
    }
    else
    {
        // Main mode
        if (btn == 3 && IS_TRANSMITTER) // Button 4: transmit name
        {
            Serial.println("Transmitting name...");
            char packet[NAME_MAX_LEN + 2];
            snprintf(packet, sizeof(packet), "N:%s", deviceName);
            LoRa.beginPacket();
            LoRa.print(packet);
            LoRa.endPacket();
            Serial.print("Sent: ");
            Serial.println(packet);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Sent!");
            delay(1000);
            displayMainScreen();
        }
    }
}

void handleLongPress(int btn)
{
    if (btn == 2) // Button 3
    {
        if (!namingMode)
        {
            // Enter naming mode
            namingMode = true;
            namePos = 0;
            Serial.println("Entering naming mode");
            displayNameEditMode();
        }
        else
        {
            // Exit naming mode and save
            namingMode = false;
            saveNameToEEPROM();
            Serial.println("Exited naming mode, saved to EEPROM");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Name saved!");
            delay(1000);
            displayMainScreen();
        }
    }
}

void updateButtons()
{
    int pins[4] = {BTN_1, BTN_2, BTN_3, BTN_4};
    
    for (int i = 0; i < 4; ++i)
    {
        int reading = digitalRead(pins[i]);
        
        // Debounce
        if (reading != btnLastState[i])
        {
            btnLastDebounce[i] = millis();
            btnLastState[i] = reading;
        }
        
        if ((millis() - btnLastDebounce[i]) > DEBOUNCE_MS)
        {
            if (reading != btnState[i])
            {
                btnState[i] = reading;
                
                if (btnState[i] == LOW) // Button pressed
                {
                    btnPressTime[i] = millis();
                    btnLongPressHandled[i] = false;
                    Serial.print("Button ");
                    Serial.print(i + 1);
                    Serial.println(" pressed");
                }
                else // Button released
                {
                    unsigned long pressDuration = millis() - btnPressTime[i];
                    if (pressDuration < LONG_PRESS_MS && !btnLongPressHandled[i])
                    {
                        handleButtonPress(i);
                    }
                    btnPressTime[i] = 0;
                    Serial.print("Button ");
                    Serial.print(i + 1);
                    Serial.println(" released");
                }
            }
        }
        
        // Long-press detection
        if (btnState[i] == LOW && btnPressTime[i] > 0 && !btnLongPressHandled[i])
        {
            if (millis() - btnPressTime[i] >= LONG_PRESS_MS)
            {
                btnLongPressHandled[i] = true;
                handleLongPress(i);
                Serial.print("Button ");
                Serial.print(i + 1);
                Serial.println(" long-pressed");
            }
        }
    }
}

void loop()
{
    if (!loRaOk)
    {
        delay(100);
        return;
    }
    
    updateButtons();
    
    if (!IS_TRANSMITTER && !namingMode)
    {
        // Receiver: listen for incoming packets
        int packetSize = LoRa.parsePacket();
        if (packetSize)
        {
            char buffer[NAME_MAX_LEN + 10] = {0};
            int idx = 0;
            while (LoRa.available() && idx < (int)sizeof(buffer) - 1)
            {
                buffer[idx++] = (char)LoRa.read();
            }
            buffer[idx] = '\0';
            
            Serial.print("Received: ");
            Serial.println(buffer);
            
            // Parse packet: "N:<name>"
            if (buffer[0] == 'N' && buffer[1] == ':')
            {
                const char *remoteName = buffer + 2;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("From:");
                lcd.setCursor(0, 1);
                lcd.print(remoteName);
                delay(3000);
                displayMainScreen();
            }
            
            int rssi = LoRa.packetRssi();
            Serial.print("RSSI: ");
            Serial.println(rssi);
        }
    }
    
    delay(50);
}
