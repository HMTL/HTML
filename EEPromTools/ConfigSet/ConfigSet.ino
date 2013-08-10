/*
 * Write out an HTML config if it doesn't already exist
 */
#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>


#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"


boolean wrote_config = false;

#define PIN_DEBUG_LED 13

#define MAX_OUTPUTS 3

config_hdr_t config;
output_hdr_t *outputs[MAX_OUTPUTS];

config_value_t val_output;
config_rgb_t rgb_output;
boolean force_write = false; // XXX - Should not be enabled except for debugging

void config_init() 
{
  val_output.hdr.type = HMTL_OUTPUT_VALUE;
  val_output.hdr.output = 0;
  val_output.pin = 9;
  val_output.value = 0;

  rgb_output.hdr.type = HMTL_OUTPUT_RGB;
  rgb_output.hdr.output = 1;
  rgb_output.pins[0] = 10;
  rgb_output.pins[1] = 11;
  rgb_output.pins[2] = 12;
  rgb_output.values[0] = 0;
  rgb_output.values[1] = 0;
  rgb_output.values[2] = 0;

  hmtl_default_config(&config);
  config.address = 0;
  config.num_outputs = 2;
  
  outputs[0] = &rgb_output.hdr;
  outputs[1] = &val_output.hdr;
}


void setup() 
{
  config_init();
  
  Serial.begin(9600);

  config_hdr_t readconfig;
  readconfig.address = -1;
  config_max_t readoutputs[MAX_OUTPUTS];
  if ((hmtl_read_config(&readconfig, readoutputs, MAX_OUTPUTS) < 0) ||
      (readconfig.address != config.address) ||
      force_write) {
    if (hmtl_write_config(&config, outputs) < 0) {
      DEBUG_PRINTLN(0, "Failed to write config");
    } else {
      wrote_config = true;
    }
  }

  pinMode(PIN_DEBUG_LED, OUTPUT);
}

boolean output_data = false;
void loop() 
{
  if (!output_data) {
    DEBUG_VALUE(0, "My address:", config.address);
    DEBUG_VALUELN(0, " Was address written: ", wrote_config);
    hmtl_print_config(&config, outputs);
    output_data = true;
  }

  blink_value(PIN_DEBUG_LED, config.address, 250, 4);
  delay(10);
}