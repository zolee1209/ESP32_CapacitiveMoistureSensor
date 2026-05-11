/*
 * Target: ESP32-C3 @ 80 MHz  (Arduino framework, ESP32 Arduino core 3.x)
 *
 * HARDWARE:
 *   GPIO 1  – voltage divider pull-down switch (output LOW during measurement)
 *   GPIO 2  – ADC1_CH2, voltage divider mid-point (PCB pull-up to 3.3 V)
 *   GPIO 5  – ADC2_CH0
 *   GPIO 6  – 40 MHz square-wave output (Phase A)
 *   GPIO 7  – 1.25 MHz square-wave output (Phase B)
 *   GPIO 8  – active-low LED
 *
 * ESP32-C3 ADC PIN MAPPING:
 *   GPIO0 → ADC1_CH0
 *   GPIO1 → ADC1_CH1
 *   GPIO2 → ADC1_CH2  ← voltage divider mid-point
 *   GPIO3 → ADC1_CH3
 *   GPIO4 → ADC1_CH4
 *   GPIO5 → ADC2_CH0  ← square-wave ADC input (NOT ADC1!)
 *
 * IMPORTANT: ADC2 is shared with Wi-Fi. All ADC measurements are taken
 *   BEFORE WiFi/ESP-NOW is initialised to ensure reliable ADC2 readings.
 *
 * WAKE CYCLE (everything in setup(), deep sleep at end):
 *  1. ADC init
 *  2. Voltage divider: GPIO1 → output LOW, sample GPIO2 ADC1_CH2    → tartalek[0]
 *  3. Phase A: 40 MHz on GPIO6, sample GPIO5 ADC2_CH0               → tartalek[1]
 *  4. Phase B: 1.25 MHz on GPIO7, sample GPIO5 ADC2_CH0             → tartalek[2]
 *  5. LED on  (active-low → LOW)
 *  6. WiFi STA + ESP-NOW init, add peer, fill struct, send
 *  7. Wait for send callback (max 500 ms)
 *  8. LED off, deep sleep (60 s)
 *
 * DATA STRUCT (must match master):
 *   id[6]          – sender MAC address
 *   homerseklet    – -1.0  (no DHT sensor)
 *   paratartalom   – -1.0  (no DHT sensor)
 *   hoerzet        – -1.0  (no DHT sensor)
 *   tartalek[0]    – avgDiv  (voltage divider ADC raw average, 0–4095)
 *   tartalek[1]    – avgA    (40 MHz phase ADC raw average,    0–4095)
 *   tartalek[2]    – avgB    (1.25 MHz phase ADC raw average,  0–4095)
 *   tartalek[3..11]– 0.0
 *
 * LEDC (APB 80 MHz):
 *   40 MHz:   80e6 / (40e6   × 2)  = 1 → 1-bit, duty=1
 *   1.25 MHz: 80e6 / (1.25e6 × 64) = 1 → 6-bit, duty=32
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/adc.h"

// ── Pin definitions ───────────────────────────────────────────────────────────
static const uint8_t PIN_DIV_PULL = 1;   // voltage divider pull-down switch
static const uint8_t PIN_DIV_ADC  = 2;   // ADC1_CH2, voltage divider input (unused as GPIO)
static const uint8_t PIN_ADC      = 5;   // ADC2_CH0
static const uint8_t PIN_SQ_A     = 6;   // 40 MHz square-wave output
static const uint8_t PIN_SQ_B     = 7;   // 1.25 MHz square-wave output
#define LED_PIN  8                        // active-low LED

// ── LEDC resources ────────────────────────────────────────────────────────────
#define LEDC_TIMER_A   LEDC_TIMER_0
#define LEDC_TIMER_B   LEDC_TIMER_1
#define LEDC_CHAN_A    LEDC_CHANNEL_0
#define LEDC_CHAN_B    LEDC_CHANNEL_1

// ── ADC / timing constants ────────────────────────────────────────────────────
static const uint8_t  ADC_SAMPLES          = 20;
static const uint8_t  ADC_DIV_SAMPLES      = 20;
static const uint32_t DELAY_AFTER_START_MS = 100;
static const uint32_t DELAY_BETWEEN_MS     = 500;

// ── Master MAC address ────────────────────────────────────────────────────────
uint8_t kozpontMAC[] = {0xA8, 0x42, 0xE3, 0x4B, 0x7F, 0x34};

// ── TX power ──────────────────────────────────────────────────────────────────
// #define TX_POWER_VALUE   8    //  8 * 0.25 =  2.0 dBm
// #define TX_POWER_VALUE  40    // 40 * 0.25 = 10.0 dBm
 #define TX_POWER_VALUE  60    // 60 * 0.25 = 15.0 dBm
// #define TX_POWER_VALUE  80    // 80 * 0.25 = 20.0 dBm

// ── Deep sleep duration ───────────────────────────────────────────────────────
#define TIME_TO_SLEEP_US  (10ULL * 6 * 1000000ULL)

// ── Data struct (must match master) ──────────────────────────────────────────
typedef struct struct_message {
    uint8_t id[6];          // küldő MAC-cím (6 byte)
    float homerseklet;      // -1.0 (nincs DHT szenzor)
    float paratartalom;     // -1.0 (nincs DHT szenzor)
    float hoerzet;          // -1.0 (nincs DHT szenzor)
    float tartalek[12];     // [0]=avgDiv  [1]=avgA  [2]=avgB  [3..11]=0.0
} struct_message;

struct_message kuldendoAdat;

// ── Callback flags ────────────────────────────────────────────────────────────
volatile bool kuldesiKesz  = false;
volatile bool kuldesiSiker = false;

// ─────────────────────────────────────────────────────────────────────────────
// ESP-NOW send callback
// ─────────────────────────────────────────────────────────────────────────────
void OnDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    kuldesiSiker = (status == ESP_NOW_SEND_SUCCESS);
    kuldesiKesz  = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// GPIO helpers
// ─────────────────────────────────────────────────────────────────────────────
static void setInputFloating(uint8_t pin)
{
    pinMode(pin, INPUT);
    gpio_set_pull_mode((gpio_num_t)pin, GPIO_FLOATING);
}

static void setOutputLow(uint8_t pin)
{
    gpio_set_level((gpio_num_t)pin, 0);
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode((gpio_num_t)pin, GPIO_FLOATING);
}

// ─────────────────────────────────────────────────────────────────────────────
// ADC helpers
// ─────────────────────────────────────────────────────────────────────────────
static void adc2_init_ch0(void)
{
    adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_12);
    int raw = 0;
    for (int i = 0; i < 5; i++) {
        adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &raw);
        ets_delay_us(200);
    }
}

static void adc1_init_ch2(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_12);
    for (int i = 0; i < 5; i++) {
        adc1_get_raw(ADC1_CHANNEL_2);
        ets_delay_us(200);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Measurement helpers: sample and return average directly
// ─────────────────────────────────────────────────────────────────────────────
static int averageSamplesDiv(uint8_t count)
{
    int32_t sum = 0;
    for (uint8_t i = 0; i < count; i++) sum += adc1_get_raw(ADC1_CHANNEL_2);
    return (int)(sum / count);
}

static int averageSamplesADC2(uint8_t count)
{
    int32_t sum = 0;
    int raw = 0;
    uint8_t ok = 0;
    for (uint8_t i = 0; i < count; i++) {
        esp_err_t err = adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &raw);
        if (err == ESP_OK) { sum += raw; ok++; }
    }
    if (ok == 0) return -1;
    return (int)(sum / ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// LEDC helpers
// ─────────────────────────────────────────────────────────────────────────────
static void ledc_sq_start(ledc_timer_t timer, ledc_channel_t channel,
                           uint8_t pin, uint32_t freqHz, ledc_timer_bit_t resolution)
{
    const uint32_t target_duty = (1u << (uint32_t)resolution) / 2u;   // 50%

    ledc_timer_config_t tmr = {};
    tmr.speed_mode      = LEDC_LOW_SPEED_MODE;
    tmr.duty_resolution = resolution;
    tmr.timer_num       = timer;
    tmr.freq_hz         = freqHz;
    tmr.clk_cfg         = LEDC_USE_APB_CLK;   // 80 MHz APB clock
    ledc_timer_config(&tmr);

    ledc_channel_config_t ch = {};
    ch.gpio_num   = pin;
    ch.speed_mode = LEDC_LOW_SPEED_MODE;
    ch.channel    = channel;
    ch.intr_type  = LEDC_INTR_DISABLE;
    ch.timer_sel  = timer;
    ch.duty       = target_duty;
    ch.hpoint     = 0;
    ledc_channel_config(&ch);

    // Explicitly force duty update – ledc_channel_config() does call
    // ledc_update_duty() internally but on some deep-sleep wakeups the
    // shadow register does not latch correctly without a second explicit call.
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, target_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

static void ledc_stop_and_float(ledc_timer_t timer, ledc_channel_t channel, uint8_t pin)
{
    ledc_stop(LEDC_LOW_SPEED_MODE, channel, 0);
    ledc_timer_pause(LEDC_LOW_SPEED_MODE, timer);
    setInputFloating(pin);
}

// ─────────────────────────────────────────────────────────────────────────────
void setup()
{
    // LED on (active-low)
    pinMode(LED_PIN, OUTPUT);

    // ADC init – must happen BEFORE WiFi, because ADC2 is shared with Wi-Fi
    adc2_init_ch0();   // GPIO5 = ADC2_CH0
    adc1_init_ch2();   // GPIO2 = ADC1_CH2

    // Initial pin states for square-wave outputs
    setInputFloating(PIN_DIV_PULL);   // GPIO1 → floating input
    setInputFloating(PIN_SQ_A);       // GPIO6 → floating input
    setInputFloating(PIN_SQ_B);       // GPIO7 → floating input

    // ── Voltage divider measurement ──────────────────────────────────────────
    // PCB pull-up on GPIO2 + GPIO1 output LOW → divider ≈ Vcc/2
    setOutputLow(PIN_DIV_PULL);
    delay(1);                                          // settle RC divider
    int avgDiv = averageSamplesDiv(ADC_DIV_SAMPLES);
    setInputFloating(PIN_DIV_PULL);
    //delay(DELAY_BETWEEN_MS);

    // ── Phase A: 40 MHz on GPIO6 ─────────────────────────────────────────────
    // 80 MHz APB / 2 (1-bit) = 40 000 000 Hz
    ledc_sq_start(LEDC_TIMER_A, LEDC_CHAN_A, PIN_SQ_A, 40000000UL, LEDC_TIMER_1_BIT);
    delay(DELAY_AFTER_START_MS);
    int avgA = averageSamplesADC2(ADC_SAMPLES);
    ledc_stop_and_float(LEDC_TIMER_A, LEDC_CHAN_A, PIN_SQ_A);
    delay(DELAY_BETWEEN_MS);

    // ── Phase B: 1.25 MHz on GPIO7 ───────────────────────────────────────────
    // 80 MHz APB / 64 (6-bit) = 1 250 000 Hz
    // Re-warmup ADC2: ~1100 ms elapsed since adc2_init_ch0(); Phase A LEDC noise
    // may have shifted the ADC2 bias. Re-initialise to restore stable baseline.
    adc2_init_ch0();
    ledc_sq_start(LEDC_TIMER_B, LEDC_CHAN_B, PIN_SQ_B, 1250000UL, LEDC_TIMER_6_BIT);
    delay(DELAY_AFTER_START_MS);
    int avgB = averageSamplesADC2(ADC_SAMPLES);
    ledc_stop_and_float(LEDC_TIMER_B, LEDC_CHAN_B, PIN_SQ_B);
    delay(DELAY_BETWEEN_MS);

    
    digitalWrite(LED_PIN, LOW);

    // ── WiFi + ESP-NOW ──────────────────────────────────
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    //delay(100);

    esp_wifi_set_max_tx_power(TX_POWER_VALUE);

    if (esp_now_init() != ESP_OK) {
        delay(3000);
        ESP.restart();
    }

    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, kozpontMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        delay(3000);
        ESP.restart();
    }

    // ── Fill data struct ─────────────────────────────────────────────────────
    WiFi.macAddress(kuldendoAdat.id);
    kuldendoAdat.homerseklet  = -1.0f;   // no DHT sensor
    kuldendoAdat.paratartalom = -1.0f;   // no DHT sensor
    kuldendoAdat.hoerzet      = -1.0f;   // no DHT sensor
    kuldendoAdat.tartalek[0]  = (float)avgDiv;
    kuldendoAdat.tartalek[1]  = (float)avgA;
    kuldendoAdat.tartalek[2]  = (float)avgB;
    for (int i = 3; i < 12; i++) kuldendoAdat.tartalek[i] = 0.0f;

    // ── Send ─────────────────────────────────────────────────────────────────
    esp_now_send(kozpontMAC, (uint8_t *)&kuldendoAdat, sizeof(kuldendoAdat));

    // Wait for send callback
     unsigned long start = millis();
    while (!kuldesiKesz && (millis() - start < 500)) {
        delay(10);
    }
    

    digitalWrite(LED_PIN, HIGH);   // LED off before sleep

    // ── Deep sleep ───────────────────────────────────────────────────────────
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_US);
    esp_deep_sleep_start();
    // Execution never reaches here – device resets and setup() runs again
}

void loop()
{

}
