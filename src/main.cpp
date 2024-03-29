#include <Arduino.h>
#include <SoftwareSerial.h>
#include <avr/interrupt.h>

#include "fir.h"

#define LDR_PIN A0
#define ADC_RESOLUTION 1024  // 10 bit
#define MEASURE_INTERVAL 10 // ms
#define SUPPLY_VOLTAGE 5.f
#define BAUD_RATE 115200
#define BAUD_RATE2 300

#define LENGTH(a) (sizeof a / sizeof a[0])

#define VOLT_TO_LUX(V) ((float)(23.4548121 * pow(M_E, 1.19824556*(V))))

#define FILTER_WEIGHT1 0.95f
#define FILTER_WEIGHT2 (1.f - FILTER_WEIGHT1)

#define NOTCH_WEIGHT1 1.5687f
#define NOTCH_WEIGHT2 -0.9391f
#define NOTCH_WEIGHT3 0.9695f
#define NOTCH_WEIGHT4 -1.5687f
#define NOTCH_WEIGHT5 0.9695f

#define PRINT_INTERVAL 1000  //ms
#define SEND_COMPRESSED
#define DATA_PACKET_LENGTH 11
#define WORK_STATION_NUMBER 0x2

#define INTERRUPT_PIN digitalPinToInterrupt(2)
#define SS_TX_PIN 3

SoftwareSerial sws(4, SS_TX_PIN, true);


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
uint8_t flag;

void shift_array(float array[], size_t len, float val);
void print_vals(void);
void send_data(int work_station_nr, int lux);
uint8_t calc_crc(uint8_t *data, size_t len);
void set_flag();

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial);

  sws.begin(BAUD_RATE2);
  attachInterrupt(INTERRUPT_PIN, set_flag, RISING);

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

  if (flag && mills - last_printed >= PRINT_INTERVAL) {
    flag = 0x00;
  	// last_printed = mills;
    // print_vals();

    send_data(WORK_STATION_NUMBER, ldr.lux[0]);
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

void send_data(int work_station_nr, int lux)
{
#ifdef SEND_COMPRESSED
  // a = workstation bit, A = MSB
  // b = lux bit, B = MSB
  // c = crc bit, C = MSB
  // frame = Aaaa Bbbb bbbb bbbb Cccc cccc
  // store bits big endian?
  lux = 69;
  uint8_t frame[3];
  frame[0] = ((lux >> 8) & 0xF) | ((work_station_nr & 0xF) << 4);
  frame[1] = lux & 0xFF;
  frame[2] = calc_crc(frame, 2);
  sws.write(frame[0]);
  sws.write(frame[1]);
  sws.write(frame[2]);

  for (size_t j = 0; j < 3; j++) {
    for (size_t i = 0; i < 8; i++) {
      if (i % 4 == 0) Serial.write(" ");
      Serial.write(((frame[j] >> (7- i)) & 0x1) + '0');
    }
  }
  Serial.println();
#else
  char data[DATA_PACKET_LENGTH];
  sprintf(data, "A%02dH%dC%02d!", work_station_nr, lux, 0);
  for (unsigned char c = 0; c < DATA_PACKET_LENGTH; ++c)
    sws.write(data[c]);
#endif
}

// from https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks#Bit_ordering_(endianness)
uint8_t calc_crc(uint8_t *data, size_t len)
{
    uint8_t crc = 0x0;

    size_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80))
                crc = (uint8_t)((crc << 1) ^ 0x);
            else
                crc <<= 1;
        }
    }
    return crc;
}

void set_flag()
{
  flag = 0xFF;
  last_printed = mills;
}