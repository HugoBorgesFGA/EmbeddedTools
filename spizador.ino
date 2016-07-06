#include <SPI.h>
#include <string.h>

#define ARRAY_SIZE(X) (sizeof(X)/sizeof(X[0]))

/* ###################################### PIN CONFIG ######################################### */
#define PIN_LED           13
#define PIN_SPINSS        10
#define PIN_MOSI          11
#define PIN_MISO          12
#define PIN_SPICLOCK      13
/* ########################################################################################### */

typedef void (*command_function_t)();
typedef struct
{
  String trigger;
  command_function_t callback;
}command_t;

int ledPinState;
String bufferRX_serial;
uint8_t bufferRX_spi[100];
uint16_t bufferRX_spi_len;
uint8_t bufferTX_spi[100];
uint16_t bufferTX_spi_len;

static int transceive_spi_data(uint8_t tx_len, uint16_t *rx_len)
{
  send_spi_data(tx_len);
  
  delayMicroseconds(100);
  //while(digitalRead(PIN_READ) == HIGH);
  
  read_spi_data(rx_len);

#if 0
  Serial.print("Received: ");
  for(uint16_t i = 0; i < *rx_len; i++)
  {
    Serial.print(bufferRX_spi[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
#endif
}

static int send_spi_data(uint8_t len)
{
  bufferRX_spi_len = 0x00;
  digitalWrite(PIN_SPINSS, LOW);
  for(size_t i = 0; i < len; i++)
  {
   SPI.transfer(bufferTX_spi[i]); 
  }
  digitalWrite(PIN_SPINSS, HIGH);
}

static int read_spi_data(uint16_t *rx_len)
{
  uint16_t i = 0x00;
  
  digitalWrite(PIN_SPINSS, LOW);
  SPI.transfer(0xFF);

  *rx_len = (uint16_t) SPI.transfer(0xFF);
  *rx_len |= (uint16_t) (SPI.transfer(0xFF) << 8);
  SPI.transfer(0xFF);
  SPI.transfer(0xFF);
  
  for(i = 0; i < *rx_len; i++)
  {
   bufferRX_spi[i] = SPI.transfer(0xFF); 
  }

  bufferRX_spi_len = *rx_len;
  digitalWrite(PIN_SPINSS, HIGH);
}

/* ###################################### COMMANDS ######################################### */
void cmd_blink();
void cmd_help();
void cmd_spi_get_cfg();

command_t commands[] = {
  {"help", cmd_help},
  {"blink", cmd_blink},
  {"get_cfg", cmd_spi_get_cfg},
};

void cmd_blink() {
 ledPinState ^= 1;
 digitalWrite(PIN_LED, ledPinState);
 delay(500);
}

void cmd_help() {
 Serial.println("List of available commands: \n");
 for(int i = 0; i < ARRAY_SIZE(commands); i++) Serial.println(commands[i].trigger);
}


void cmd_spi_get_cfg() {
  uint16_t rx_len;
  const uint8_t cmd[] =
  {
   0x00, //This byte is as per SPI standard
   0x05, 0x00, 0x00, 0x00, //These 4 bytes indicate the length of the pay load
   0xFF, 0xF8, 0x00, 0x00, 0x00
  };

  memcpy(bufferTX_spi, cmd, sizeof(cmd));
  transceive_spi_data((uint8_t) sizeof(cmd), &rx_len);
}

void setup() {
  Serial.begin(9600);
  
  SPI.begin();

  pinMode(PIN_SPINSS, OUTPUT);
  digitalWrite(PIN_SPINSS, HIGH);
  
  // make the pins outputs:
  ledPinState = LOW;
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, ledPinState);
}

void loop() {
  uint8_t found = 0x00;
  
  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    found = 0x00;
    bufferRX_serial = Serial.readString();
    
    for(int i = 0; i < ARRAY_SIZE(commands); i++)
    {
       if (bufferRX_serial.equals(commands[i].trigger))
       {
         Serial.println("Executing the " + commands[i].trigger + " command"); 
         commands[i].callback();
         found = 0x01;
       }
    }
    
    if (found == 0x00) Serial.println("Command not found...");
  }
}








