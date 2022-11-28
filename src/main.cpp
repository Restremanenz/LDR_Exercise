#include <Arduino.h>

#define LDR_PIN A0
#define ADC_RESOLUTION 1024  // 10 bit
#define MEASURE_INTERVAL 100 // ms
#define SUPPLY_VOLTAGE 5.f

#define VOLT_TO_LUX(V) ((float)(23.4548121 * pow(M_E, 1.19824556*(V))))

#define PRINT_INTERVAL 500 //ms


typedef struct {
  uint16_t adc;
  float voltage;
  float lux;
} ldr_t;

ldr_t ldr = {0};

unsigned long mills, last_measured, last_printed;

void print_vals(void);

void setup() {
  Serial.begin(9600);
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
  }

  if (mills - last_printed >= PRINT_INTERVAL) {
  	last_printed = mills;

    print_vals();
  }
}

void print_vals(void)
{
    Serial.print("ADC = ");
    Serial.print(ldr.adc);
    Serial.print("    ");
    Serial.print("V = ");
    Serial.print(ldr.voltage, 3);
    Serial.print("    ");
    Serial.print("LUX = ");
    Serial.print(ldr.lux, 3);
    Serial.print("         \r");
}