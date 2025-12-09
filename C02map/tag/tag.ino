/*
 * UWB Tag Code for ESP32 + DW1000 + SenseAir S8 CO2 Sensor
 * With Bluetooth LE (BLE) Communication
 * 
 * Sends JSON data via BLE to connected devices
 */

#include <SPI.h>
#include "DW1000Ranging.h"
#include "link.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ============================================================
// TAG CONFIGURATION
// ============================================================
#define TAG_ADDR "7D:00:22:EA:82:60:3B:9C"

// UWB pins
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

const uint8_t PIN_RST = 27;
const uint8_t PIN_IRQ = 34;
const uint8_t PIN_SS = 4;

// ============================================================
// BLUETOOTH LE CONFIGURATION
// ============================================================
#define BLE_DEVICE_NAME "UWB_Tag"
#define SERVICE_UUID        "12341234-1234-1234-1234-123412341234"
#define CHARACTERISTIC_UUID "12341234-1234-1234-1234-123412341236"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Callback for connection/disconnection
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("BLE: Client connected");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("BLE: Client disconnected");
    }
};

// ============================================================
// CO2 SENSOR CONFIGURATION
// ============================================================
#define TXD2 22
#define RXD2 21
#define SERIAL2_RX_BUFFER 512

byte readCO2[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================
void IRAM_ATTR newRange();
void IRAM_ATTR newDevice(DW1000Device *device);
void IRAM_ATTR inactiveDevice(DW1000Device *device);
unsigned int modbusCRC(byte* buf, int len);
void updateCO2NonBlocking();
void setupBLE();

// ============================================================
// GLOBAL VARIABLES
// ============================================================
struct MyLink *link_head;
int co2Value = 400;
unsigned long lastCO2Read = 0;
int co2ErrorCount = 0;

// Thread-safe ISR data buffer
portMUX_TYPE rangingMutex = portMUX_INITIALIZER_UNLOCKED;

typedef struct {
    uint16_t addr;
    float range;
    float dbm;
    volatile bool valid;
} RangingData_t;

RangingData_t isr_data = {0, 0.0f, 0.0f, false};

// Non-blocking CO2 state machine
enum CO2State {
    CO2_IDLE,
    CO2_WAIT_RESPONSE,
    CO2_PROCESS
};

CO2State co2_state = CO2_IDLE;
unsigned long co2_timeout_start = 0;
byte co2_response[7];
int co2_bytes_read = 0;

// ============================================================
// MODBUS CRC16 CALCULATION
// ============================================================
unsigned int modbusCRC(byte* buf, int len) {
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// ============================================================
// NON-BLOCKING CO2 READING
// ============================================================
void updateCO2NonBlocking() {
    const int expectedLength = 7;
    
    switch (co2_state) {
        case CO2_IDLE:
            if (millis() - lastCO2Read > 6000) {
                while (Serial2.available()) Serial2.read();
                delay(10);
                
                Serial2.write(readCO2, sizeof(readCO2));
                
                co2_state = CO2_WAIT_RESPONSE;
                co2_timeout_start = millis();
                co2_bytes_read = 0;
            }
            break;
            
        case CO2_WAIT_RESPONSE:
            while (Serial2.available() && co2_bytes_read < expectedLength) {
                co2_response[co2_bytes_read++] = Serial2.read();
            }
            
            if (co2_bytes_read == expectedLength) {
                co2_state = CO2_PROCESS;
            }
            else if (millis() - co2_timeout_start > 250) {
                Serial.println("CO2: Timeout");
                co2ErrorCount++;
                if (co2ErrorCount > 5) co2Value = 0;
                co2_state = CO2_IDLE;
                lastCO2Read = millis();
            }
            break;
            
        case CO2_PROCESS:
            {
                unsigned int crcReceived = co2_response[5] | ((unsigned int)co2_response[6] << 8);
                unsigned int crcCalc = modbusCRC(co2_response, 5);
                
                if (crcReceived == crcCalc) {
                    uint16_t co2ppm = ((uint16_t)co2_response[3] << 8) | co2_response[4];
                    
                    if (co2ppm <= 10000) {
                        co2Value = (int)co2ppm;
                        co2ErrorCount = 0;
                        Serial.print("CO2 = ");
                        Serial.print(co2Value);
                        Serial.println(" ppm");
                    } else {
                        Serial.println("CO2: Value out of range");
                        co2ErrorCount++;
                    }
                } else {
                    Serial.println("CO2: CRC error");
                    co2ErrorCount++;
                }
                
                co2_state = CO2_IDLE;
                lastCO2Read = millis();
            }
            break;
    }
}

// ============================================================
// BLUETOOTH LE SETUP
// ============================================================
void setupBLE() {
    // Create BLE Device
    BLEDevice::init(BLE_DEVICE_NAME);
    
    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    
    // CRITICAL FIX: Properly configure the descriptor
    BLE2902* pDescriptor = new BLE2902();
    pDescriptor->setNotifications(true);
    pDescriptor->setIndications(false);
    pCharacteristic->addDescriptor(pDescriptor);
    
    // Set initial value
    pCharacteristic->setValue("Ready");
    
    // Start service
    pService->start();
    
    // Configure advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x12);
    
    // Start advertising
    BLEDevice::startAdvertising();
    
    Serial.println("BLE: Server started, advertising as 'UWB_Tag'");
    Serial.print("BLE: Characteristic UUID: ");
    Serial.println(CHARACTERISTIC_UUID);
}


// ============================================================
// SETUP
// ============================================================
void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("========================================");
    Serial.println("UWB Tag with BLE + SenseAir S8");
    Serial.println("========================================");
    Serial.print("Tag Address: ");
    Serial.println(TAG_ADDR);
    
    // Initialize Bluetooth LE
    setupBLE();
    
    // Initialize CO2 sensor
    Serial2.setRxBufferSize(SERIAL2_RX_BUFFER);
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    Serial.print("CO2 sensor: Serial2 (RX:");
    Serial.print(RXD2);
    Serial.print(", TX:");
    Serial.print(TXD2);
    Serial.println(")");
    
    delay(2000);
    
    // Initialize linked list
    link_head = init_link();
    if (link_head == NULL)
    {
        Serial.println("FATAL: Could not initialize link list");
        while(1) { delay(1000); }
    }
    
    // Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    // Initialize DW1000
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);

    DW1000.setAntennaDelay(16436); 
    
    // Attach callbacks
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    
    // Start as TAG with static addressing
    DW1000Ranging.startAsTag(TAG_ADDR, 
                            DW1000.MODE_SHORTDATA_FAST_LOWPOWER, 
                            false);
    
    Serial.println("Tag ready - scanning for anchors...");
    Serial.println("========================================");
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop()
{
    DW1000Ranging.loop();
    
    updateCO2NonBlocking();
    
    // Handle BLE connection changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack time to get ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("BLE: Restarting advertising");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    
    // Process ISR data with critical section
    portENTER_CRITICAL(&rangingMutex);
    if (isr_data.valid) {
        uint16_t addr = isr_data.addr;
        float range = isr_data.range;
        float dbm = isr_data.dbm;
        isr_data.valid = false;
        portEXIT_CRITICAL(&rangingMutex);
        
        struct MyLink *existing = find_link(link_head, addr);
        
        if (existing == NULL) {
            add_link(link_head, addr);
            Serial.print("New anchor: 0x");
            Serial.println(addr, HEX);
        }
        
        fresh_link(link_head, addr, range, dbm);
        
        Serial.print("Anchor: 0x");
        Serial.print(addr, HEX);
        Serial.print("\t R: ");
        Serial.print(range, 2);
        Serial.print("m\t RX: ");
        Serial.print(dbm, 1);
        Serial.println(" dBm");
    } else {
        portEXIT_CRITICAL(&rangingMutex);
    }
    
    // Send JSON data via BLE every 2 seconds
    static unsigned long lastBLESend = 0;
    if (millis() - lastBLESend > 2000)
    {
        String json_data;
        make_link_json(link_head, &json_data, co2Value);
        
        if (json_data.length() > 2) {
            // Print to Serial for debugging
            Serial.println("JSON:");
            Serial.println(json_data);
            
            // Send via BLE if client is connected
            if (deviceConnected) {
                pCharacteristic->setValue(json_data.c_str());
                pCharacteristic->notify();
                Serial.println("BLE: Data sent");
            } else {
                Serial.println("BLE: No client connected");
            }
            
            Serial.println("---");
        }
        
        lastBLESend = millis();
    }
}

// ============================================================
// ISR-SAFE CALLBACKS
// ============================================================
void IRAM_ATTR newRange()
{
    portENTER_CRITICAL_ISR(&rangingMutex);
    isr_data.addr = DW1000Ranging.getDistantDevice()->getShortAddress();
    isr_data.range = DW1000Ranging.getDistantDevice()->getRange();
    isr_data.dbm = DW1000Ranging.getDistantDevice()->getRXPower();
    isr_data.valid = true;
    portEXIT_CRITICAL_ISR(&rangingMutex);
}

void IRAM_ATTR newDevice(DW1000Device *device)
{
    // Minimal ISR
}

void IRAM_ATTR inactiveDevice(DW1000Device *device)
{
    // Minimal ISR
}
