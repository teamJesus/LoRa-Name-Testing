# LoRa Name Tester

A PlatformIO project for testing LoRa communication between two Arduino Nano units with 1602 LCD displays. One unit transmits text messages, and the other receives and displays them on its LCD screen.

## Hardware Requirements

- 2x Arduino Nano (ATmega168 or ATmega328)
- 2x 1602 LCD displays with I2C backpack
- 2x RFM9x LoRa transceiver modules
- Breadboards, jumper wires, power supplies

## Wiring

### Arduino Nano Pins

**I2C (LCD):**
- SDA → A4
- SCL → A5

**LoRa Module (RFM9x):**
- VIN → 3.3V (with voltage regulator if needed)
- GND → GND
- SCK → D13
- MISO → D12
- MOSI → D11
- CS/NSS → D15 (configurable in code)
- RST → D2 (configurable in code)
- G0/DIO0 → D3 (configurable in code)

**LCD (I2C):**
- VCC → 5V
- GND → GND
- SDA → A4
- SCL → A5

## Configuration

Edit the pin definitions in `src/main.cpp`:

```cpp
#define IS_TRANSMITTER true  // Change to false for receiver unit
#define LORA_SS 15           // Chip select pin
#define LORA_RST 2           // Reset pin
#define LORA_DIO0 3          // Interrupt pin (DIO0/G0)
#define LCD_ADDR 0x27        // I2C address of LCD backpack
```

## Building & Uploading

```bash
# Build for Arduino Nano ATmega168
pio run

# Upload to device
pio run -t upload

# Monitor serial output at 9600 baud
pio device monitor -b 9600
```

## Operation

**Transmitter Unit:**
- Displays "TX: Ready" on LCD
- Sends counter messages ("TX: 0", "TX: 1", etc.) every 2 seconds
- Prints debug output to Serial

**Receiver Unit:**
- Displays "RX: Ready" on LCD
- Listens for incoming LoRa packets
- Displays received text on LCD as "RX: <message>"
- Prints RSSI (signal strength) to Serial

## Serial Output

Both units print debug information at 9600 baud:
- Initialization status
- TX/RX messages
- RSSI (signal strength) on receiver
- Error messages if LoRa or LCD init fails

## Libraries

- `LiquidCrystal_I2C` — I2C LCD control
- `sandeepmistry/LoRa` — LoRa communication

## Troubleshooting

**LCD not displaying:**
- Check I2C address (default 0x27, verify with I2C scanner if needed)
- Verify SDA/SCL wiring to A4/A5
- Check serial output for initialization messages

**LoRa not working:**
- Verify 3.3V power supply to module
- Check CS, RST, DIO0 pin connections
- Both units must use same frequency (915E6 MHz in code)
- Check RSSI output on receiver

**Serial not printing:**
- Verify 9600 baud rate in monitor
- Power cycle Arduino after upload
- Allow 2 seconds for USB connection to establish

## License

MIT
