#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_OxygenSensor.h"

// ====== Hardware & I2C ======
const uint8_t O2_ADDR = 0x73; // alamat I2C sensor O2 kamu (0x73 dari scanner)
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ====== Kalibrasi dasar (dari hasil analisis) ======
// fase turun (eksalasi / O2 turun) -> hasil 2 titik
const float slope_down_base = 0.964f;
const float offset_down_base = 1.225f;

// fase naik (recovery / balik ke ambient) -> hasil global
const float slope_up_base = 1.0546f;
const float offset_up_base = -1.0998f;

// ====== Parameter pembacaan ======
const uint16_t AVG_N = 20;              // averaging internal library
const uint32_t SAMPLE_MS = 200;         // interval baca ~5 Hz
const float DEAD_BAND = 0.002f;         // deadband arah (%Vol)
const uint32_t FRC_DURATION = 120000UL; // 2 menit fase FRC (ms)

// ====== State & objek ======
DFRobot_OxygenSensor oxygen;

enum TrendMode : uint8_t
{
    MODE_UNKNOWN = 0,
    MODE_DOWN,
    MODE_UP
};
enum Phase : uint8_t
{
    PHASE_FRC = 0,
    PHASE_DYNAMIC
};

TrendMode currentMode = MODE_UNKNOWN;
Phase currentPhase = PHASE_FRC;

float lastRaw = NAN;

// baseline (hasil rata-rata fase FRC)
float baseRaw = NAN;  // rata-rata raw
float baseTrue = NAN; // rata-rata corrected (FRC)

// offset efektif (setelah di-anchoring ke baseline)
float slope_down = slope_down_base;
float offset_down = offset_down_base;
float slope_up = slope_up_base;
float offset_up = offset_up_base;

// akumulasi untuk rata-rata FRC
double sumRawFRC = 0.0;
double sumTrueFRC = 0.0;
uint32_t countFRC = 0;

uint32_t startMillis = 0;

// ====== Helper ======
static inline float applyCalDown(float raw)
{
    return slope_down * raw + offset_down;
}

static inline float applyCalUp(float raw)
{
    return slope_up * raw + offset_up;
}

// pilih mode naik/turun berdasarkan perubahan raw + deadband
static inline TrendMode decideMode(float rawNow, float rawPrev, TrendMode prevMode)
{
    if (isnan(rawPrev))
        return MODE_DOWN; // default awal
    if (rawNow < rawPrev - DEAD_BAND)
        return MODE_DOWN;
    if (rawNow > rawPrev + DEAD_BAND)
        return MODE_UP;
    return prevMode; // dalam deadband -> jangan gonta-ganti mode
}

void setup()
{
    Serial.begin(115200);
    delay(200);

    Wire.begin(SDA_PIN, SCL_PIN);

    if (!oxygen.begin(O2_ADDR))
    {
        Serial.println("Init O2 gagal! Cek wiring / alamat I2C.");
        while (1)
            delay(1000);
    }

    startMillis = millis();
    currentPhase = PHASE_FRC;

    Serial.println("== O2 CAL: FRC 2 menit + Dua-Mode DOWN/UP ==");
    Serial.printf("DOWN base: slope=%.6f  offset=%.6f\n", slope_down_base, offset_down_base);
    Serial.printf("UP   base: slope=%.6f  offset=%.6f\n", slope_up_base, offset_up_base);
    Serial.printf("FRC duration: %lu ms (%.1f menit)\n",
                  (unsigned long)FRC_DURATION, FRC_DURATION / 60000.0f);
    Serial.println("Phase awal: FRC, pakai kalibrasi DOWN saja.\n");
    Serial.println("Header: raw(%Vol)\tmode\tcorrected(%Vol)");
}

void loop()
{
    // 1) Baca raw dari sensor
    float rawNow = oxygen.getOxygenData(AVG_N);

    uint32_t now = millis();

    if (currentPhase == PHASE_FRC)
    {
        // ========= PHASE FRC (2 MENIT PERTAMA) =========
        // pakai garis DOWN base sebagai koreksi
        float corrDown = slope_down_base * rawNow + offset_down_base;

        // akumulasi untuk rata-rata baseline
        sumRawFRC += rawNow;
        sumTrueFRC += corrDown;
        countFRC++;

        // print pakai label FRC (biar keliatan di serial)
        Serial.printf("%.3f\tFRC\t%.3f\n", rawNow, corrDown);

        // kalau sudah lewat 2 menit dan punya cukup data -> kunci baseline
        if (now - startMillis >= FRC_DURATION && countFRC > 10)
        {
            baseRaw = (float)(sumRawFRC / (double)countFRC);
            baseTrue = (float)(sumTrueFRC / (double)countFRC);

            // re-anchoring: paksa kedua garis (DOWN & UP) lewat titik (baseRaw, baseTrue)
            slope_down = slope_down_base;
            slope_up = slope_up_base;
            offset_down = baseTrue - slope_down * baseRaw;
            offset_up = baseTrue - slope_up * baseRaw;

            Serial.println("\n== FRC PHASE DONE ==");
            Serial.printf("Samples FRC : %lu\n", (unsigned long)countFRC);
            Serial.printf("baseRaw     : %.6f %%Vol (rata-rata raw)\n", baseRaw);
            Serial.printf("baseTrue    : %.6f %%Vol (rata-rata corrected/FRC)\n", baseTrue);
            Serial.println("Garis kalibrasi di-ANCHOR ke baseline ini:");
            Serial.printf("  DOWN: corrected = %.6f * raw + %.6f\n", slope_down, offset_down);
            Serial.printf("  UP  : corrected = %.6f * raw + %.6f\n", slope_up, offset_up);
            Serial.println("Keduanya melewati (baseRaw, baseTrue). Masuk PHASE_DYNAMIC.\n");

            currentPhase = PHASE_DYNAMIC;
            currentMode = MODE_DOWN; // mulai dari DOWN saja
            lastRaw = rawNow;        // inisialisasi untuk deteksi arah
        }
    }
    else
    {
        // ========= PHASE DYNAMIC (setelah FRC) =========

        // 1) tentukan mode naik/turun
        currentMode = decideMode(rawNow, lastRaw, currentMode);

        // 2) apply kalibrasi sesuai mode, tapi SUDAH di-anchor ke baseline
        float corrected;
        const char *modeStr;
        if (currentMode == MODE_UP)
        {
            corrected = applyCalUp(rawNow);
            modeStr = "UP";
        }
        else
        { // MODE_DOWN & MODE_UNKNOWN -> pakai DOWN
            corrected = applyCalDown(rawNow);
            modeStr = (currentMode == MODE_DOWN) ? "DOWN" : "INIT(DOWN)";
        }

        Serial.printf("%.3f\t%s\t%.3f\n", rawNow, modeStr, corrected);

        lastRaw = rawNow;
    }

    delay(SAMPLE_MS);
}
