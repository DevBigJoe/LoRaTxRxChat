//--------------------------------------------- Includes -------------------------------------------
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h> // Display
#include "HT_SSD1306Wire.h" // Display

//--------------------------------------------- LoRa Settings --------------------------------------
#define RF_FREQUENCY           868000000
#define TX_OUTPUT_POWER        14
#define LORA_BANDWIDTH         0
#define LORA_SPREADING_FACTOR  7
#define LORA_CODINGRATE        1
#define LORA_PREAMBLE_LENGTH   8
#define LORA_SYMBOL_TIMEOUT    0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON   false

#define BUFFER_SIZE 512  // Large enough for long messages
#define MAX_TTL 15       // Maximum number of hops

//--------------------------------------------- Objects --------------------------------------------
static RadioEvents_t RadioEvents;
SSD1306Wire myDisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

//--------------------------------------------- Buffers --------------------------------------------
char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];
String EingabeTastatur;
char deviceID[32];

//--------------------------------------------- Prototypes -----------------------------------------
void OnTxDone(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void startReceive();
void sendMessage(const char* message);
void showMessage(const char* sender, const char* receiver, const char* message);

//--------------------------------------------- Display --------------------------------------------
void showMessage(const char* sender, const char* receiver, const char* message) {
    myDisplay.clear();
    myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
    myDisplay.setFont(ArialMT_Plain_10);

    // Display layout
    const int lineHeight = 12; // Each line height in pixels
    const int maxLines = 6;    // 64px display / 12px per line

    // First line: From and To
    String header = String("From: ") + sender + " To: " + receiver;
    myDisplay.drawString(0, 0, header);

    // Wrap message to remaining lines
    int y = lineHeight;
    int maxLineWidth = 128;

    String msg = String(message);
    int startIdx = 0;

    while (startIdx < msg.length() && y < lineHeight * maxLines) {
        int len = 0;

        // Find max substring that fits in line
        while (len + startIdx < msg.length() &&
               myDisplay.getStringWidth(msg.substring(startIdx, startIdx + len + 1)) < maxLineWidth) {
            len++;
        }

        if (len == 0) len = 1; // Force at least one char per line

        myDisplay.drawString(0, y, msg.substring(startIdx, startIdx + len));
        startIdx += len;
        y += lineHeight;
    }

    myDisplay.display();
}

//--------------------------------------------- Send -----------------------------------------------
void sendMessage(const char* message) {
  strncpy(txpacket, message, BUFFER_SIZE - 1);
  txpacket[BUFFER_SIZE - 1] = '\0';
  Radio.Send((uint8_t*)txpacket, strlen(txpacket));
}

//--------------------------------------------- Receive --------------------------------------------
void startReceive() {
  Radio.Rx(0);
}

//--------------------------------------------- Setup ----------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  myDisplay.init();
  myDisplay.setFont(ArialMT_Plain_10);

  Mcu.begin();

  // Force user to enter ID on every boot
  Serial.println("Enter your ID:");
  while (Serial.available() == 0) { delay(10); }
  size_t len = Serial.readBytesUntil('\n', deviceID, 31);
  deviceID[len] = '\0';

  Serial.print("Your ID is: ");
  Serial.println(deviceID);

  // Register LoRa callbacks
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA,
                    TX_OUTPUT_POWER,
                    0,
                    LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH,
                    LORA_FIX_LENGTH_PAYLOAD_ON,
                    true,
                    0,
                    0,
                    LORA_IQ_INVERSION_ON,
                    3000);

  Radio.SetRxConfig(MODEM_LORA,
                    LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE,
                    0,
                    LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT,
                    LORA_FIX_LENGTH_PAYLOAD_ON,
                    0,
                    true,
                    0,
                    0,
                    LORA_IQ_INVERSION_ON,
                    true);

  startReceive();
}

//--------------------------------------------- TX Callback ----------------------------------------
void OnTxDone(void) {
  Serial.println("TX done");
  startReceive();
}

//--------------------------------------------- RX Callback ----------------------------------------
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (size >= BUFFER_SIZE) size = BUFFER_SIZE - 1;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Serial.print("Full RX: ");
  Serial.println(rxpacket);

  char sender[32];
  char receiver[32];
  int ttl;

  // Parse the first and second pipe symbols
  char* firstPipe = strchr(rxpacket, '|');
  char* secondPipe = firstPipe ? strchr(firstPipe + 1, '|') : nullptr;

  if (!firstPipe || !secondPipe) { startReceive(); return; }

  // Extract sender (up to " to ")
  char* toPos = strstr(rxpacket, " to ");
  if (!toPos || toPos >= firstPipe) { startReceive(); return; }
  size_t senderLen = toPos - rxpacket;
  strncpy(sender, rxpacket, senderLen);
  sender[senderLen] = '\0';

  // Extract receiver (between "to " and first "|")
  size_t receiverLen = firstPipe - (toPos + 4);
  strncpy(receiver, toPos + 4, receiverLen);
  receiver[receiverLen] = '\0';

  // Extract TTL
  ttl = atoi(firstPipe + 1);
  if (ttl <= 0) { startReceive(); return; }
  ttl--;

  // Extract message (after second "|")
  char message[BUFFER_SIZE];
  strncpy(message, secondPipe + 1, BUFFER_SIZE - 1);
  message[BUFFER_SIZE - 1] = '\0';

  // Display sender, receiver, and message
  showMessage(sender, receiver, message);

  // Forward if not intended for this device
  if (strcmp(receiver, deviceID) != 0) {
    char forwardPacket[BUFFER_SIZE];
    snprintf(forwardPacket, BUFFER_SIZE,
             "%s to %s | %d | %s",
             sender, receiver, ttl, message);
    sendMessage(forwardPacket);
  }

  startReceive();
}

//--------------------------------------------- Main Loop ------------------------------------------
void loop() {
  if (Serial.available() > 0) {
    EingabeTastatur = Serial.readStringUntil('\n');

    int sepIndex = EingabeTastatur.indexOf("***");
    if (sepIndex == -1) {
      Serial.println("Invalid format. Use: message ***Receiver");
      return;
    }

    String message = EingabeTastatur.substring(0, sepIndex);
    String receiver = EingabeTastatur.substring(sepIndex + 3);

    String packet = String(deviceID) + " to " + receiver + " | " + MAX_TTL + " | " + message;
    sendMessage(packet.c_str());

    Serial.print("Sent: ");
    Serial.println(packet);
  }

  Radio.IrqProcess();
}