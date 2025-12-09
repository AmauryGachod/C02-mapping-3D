/*
 * UWB Anchor Code for ESP32 + DW1000
 * Three-Anchor Indoor Positioning System
 * 
 * Hardware: ESP32 + DW1000 UWB Module
 * 
 * IMPORTANT: Change ANCHOR_ADD for each of your 3 anchors:
 * - Anchor 1: "17:86:5B:D5:A9:9A:E2:9D" → Shows as 0x1786
 * - Anchor 2: "17:87:5B:D5:A9:9A:E2:9E" → Shows as 0x1787
 * - Anchor 3: "17:88:5B:D5:A9:9A:E2:9F" → Shows as 0x1788
 */

#include <SPI.h>
#include "DW1000Ranging.h"

// ============================================================
// CONFIGURATION - CHANGE THIS FOR EACH ANCHOR
// ============================================================
// #define ANCHOR_ADD "17:86:5B:D5:A9:9A:E2:9D"  // Anchor 1 (DEFAULT)
#define ANCHOR_ADD "17:87:5B:D5:A9:9A:E2:9E"  // Uncomment for Anchor 2
// #define ANCHOR_ADD "17:88:5B:D5:A9:9A:E2:9F"  // Uncomment for Anchor 3

// Pin definitions
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS = 4;   // spi select pin

// ============================================================
// SETUP
// ============================================================
void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("========================================");
    Serial.println("UWB Anchor Initialization");
    Serial.println("========================================");
    Serial.print("Anchor Address: ");
    Serial.println(ANCHOR_ADD);
    
    // Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    // Initialize DW1000 communication
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
    
    // CRITICAL: Antenna delay calibration (fixes absurd values)
    DW1000.setAntennaDelay(16436);
    
    // Attach callback functions
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachBlinkDevice(newBlink);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    
    // Start as anchor with STATIC addressing
    // Third parameter = false prevents random address assignment
    DW1000Ranging.startAsAnchor(ANCHOR_ADD, 
                                DW1000.MODE_SHORTDATA_FAST_LOWPOWER, 
                                false);
    
    Serial.println("Anchor ready - waiting for tags...");
    Serial.println("========================================");
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop()
{
    DW1000Ranging.loop();
}

// ============================================================
// CALLBACK FUNCTIONS
// ============================================================
void newRange()
{
    Serial.print("Tag: 0x");
    Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
    Serial.print("\t Range: ");
    Serial.print(DW1000Ranging.getDistantDevice()->getRange());
    Serial.print(" m");
    Serial.print("\t RX power: ");
    Serial.print(DW1000Ranging.getDistantDevice()->getRXPower());
    Serial.println(" dBm");
}

void newBlink(DW1000Device *device)
{
    Serial.print("New tag detected! Short address: 0x");
    Serial.println(device->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device *device)
{
    Serial.print("Tag disconnected: 0x");
    Serial.println(device->getShortAddress(), HEX);
}
