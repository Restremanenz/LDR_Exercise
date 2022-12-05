#include <Arduino.h>

#define LDR_PIN A0
#define ADC_RESOLUTION 1024  // 10 bit
#define MEASURE_INTERVAL 10 // ms
#define SUPPLY_VOLTAGE 5.f
#define BAUD_RATE 115200

#define VOLT_TO_LUX(V) ((float)(23.4548121 * pow(M_E, 1.19824556*(V))))

#define FILTER_WEIGHT1 0.95f
#define FILTER_WEIGHT2 (1.f - FILTER_WEIGHT1)

#define PRINT_INTERVAL 15  //ms


typedef struct {
  uint16_t adc;
  float voltage;
  float lux;
  float luxf;
} ldr_t;

ldr_t ldr = {0};

unsigned long mills, last_measured, last_printed;

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

    ldr.adc     = analogRead(LDR_PIN);
    ldr.voltage = SUPPLY_VOLTAGE / (ADC_RESOLUTION - 1) * (float)ldr.adc;
    ldr.lux     = VOLT_TO_LUX(ldr.voltage); 
    ldr.lux     = ldr.lux > 999.f ? 999.f : ldr.lux;
    ldr.luxf    = FILTER_WEIGHT1 * ldr.luxf + FILTER_WEIGHT2 * ldr.lux;
  }

  if (mills - last_printed >= PRINT_INTERVAL) {
  	last_printed = mills;

    print_vals();
  }
}

void print_vals(void)
{
  Serial.write('$');
  Serial.print(ldr.lux, 3);
  Serial.write(' ');
  Serial.print(ldr.luxf, 3);
  Serial.write(';');
}