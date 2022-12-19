#include <Arduino.h>

#include "fir.h"

#define LDR_PIN A0
#define ADC_RESOLUTION 1024  // 10 bit
#define MEASURE_INTERVAL 10 // ms
#define SUPPLY_VOLTAGE 5.f
#define BAUD_RATE 115200

#define LENGTH(a) (sizeof a / sizeof a[0])

#define VOLT_TO_LUX(V) ((float)(23.4548121 * pow(M_E, 1.19824556*(V))))

#define FILTER_WEIGHT1 0.95f
#define FILTER_WEIGHT2 (1.f - FILTER_WEIGHT1)

#define NOTCH_WEIGHT1 1.5687f
#define NOTCH_WEIGHT2 -0.9391f
#define NOTCH_WEIGHT3 0.9695f
#define NOTCH_WEIGHT4 -1.5687f
#define NOTCH_WEIGHT5 0.9695f

#define PRINT_INTERVAL 15  //ms


typedef struct {
  uint16_t adc;
  float voltage;
  float lux[3 > LENGTH(fir_weights) ? 3 : LENGTH(fir_weights)];
  float luxf;
  float lux_notch[2];
  float lux_fir;
} ldr_t;

ldr_t ldr = {0};

unsigned long mills, last_measured, last_printed;

void shift_array(float array[], size_t len, float val);
void print_vals(void);

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial);

  mills = last_measured = last_printed = millis();
}

void loop() {
  mills = millis();

  if (mills - last_measured >= MEASURE_INTERVAL) {
    last_measured = mills;

    float lux, lux_notch, lux_fir;
    lux = lux_notch = lux_fir = 0.f;

    ldr.adc     = analogRead(LDR_PIN);
    ldr.voltage = SUPPLY_VOLTAGE / (ADC_RESOLUTION - 1) * (float)ldr.adc;
    lux         = VOLT_TO_LUX(ldr.voltage); 
    lux         = lux > 999.f ? 999.f : lux;
    ldr.luxf    = FILTER_WEIGHT1 * ldr.luxf + FILTER_WEIGHT2 * lux;

    shift_array(ldr.lux, LENGTH(ldr.lux), lux);
    lux_notch   = NOTCH_WEIGHT1 * ldr.lux_notch[0] + 
                  NOTCH_WEIGHT2 * ldr.lux_notch[1] + 
                  NOTCH_WEIGHT3 * ldr.lux[0] + 
                  NOTCH_WEIGHT4 * ldr.lux[1] + 
                  NOTCH_WEIGHT5 * ldr.lux[2];

    shift_array(ldr.lux_notch, LENGTH(ldr.lux_notch), lux_notch);
  
    for (size_t i = 0; i < LENGTH(fir_weights); ++i) {
      lux_fir += ldr.lux[i] * fir_weights[i];
    }
    ldr.lux_fir = lux_fir;
  }

  if (mills - last_printed >= PRINT_INTERVAL) {
  	last_printed = mills;

    print_vals();
  }
}

void shift_array(float array[], size_t len, float val)
{
  for (size_t i = len - 1; i > 0; --i) {
    array[i] = array[i - 1];
  }

  array[0] = val;
}

void print_vals(void)
{
  Serial.write('$');
  Serial.print(ldr.lux[0], 3);
  Serial.write(' ');
  Serial.print(ldr.luxf, 3);
  Serial.write(' ');
  Serial.print(ldr.lux_notch[0], 3);
  Serial.write(' ');
  Serial.print(ldr.lux_fir, 3);
  Serial.write(';');
}