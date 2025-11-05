// #include <Arduino.h>
// #include <Wire.h>
// #include "DFRobot_OxygenSensor.h"

// // ====== Hardware & I2C ======
// const uint8_t O2_ADDR = 0x73; // alamat I2C sensor DFRobot O2 (punyamu: 0x73)
// const int SDA_PIN = 21;       // default ESP32
// const int SCL_PIN = 22;       // default ESP32

// // ====== Kalibrasi dua-mode ======
// // fase turun (eksalasi / O2 turun)
// const float slope_down = 0.964f;
// const float offset_down = 1.225f;

// // fase naik (recovery / balik ke ambient)
// const float slope_up = 1.0546f;
// const float offset_up = -1.0998f;

// // ====== Parameter pembacaan ======
// const uint16_t AVG_N = 20;      // smoothing internal library
// const uint32_t SAMPLE_MS = 200; // interval baca ~5 Hz (200 ms)
// const float DEAD_BAND = 0.002f; // deadband arah (0.002 %Vol)

// // ====== State ======
// DFRobot_OxygenSensor oxygen;

// enum TrendMode : uint8_t
// {
//     MODE_UNKNOWN = 0,
//     MODE_DOWN,
//     MODE_UP
// };
// TrendMode currentMode = MODE_UNKNOWN;

// float lastRaw = NAN; // menyimpan bacaan raw sebelumnya (untuk deteksi arah)

// // ====== Helper ======
// static inline float applyCalDown(float raw)
// {
//     // corrected = slope_down * raw + offset_down
//     return slope_down * raw + offset_down;
// }

// static inline float applyCalUp(float raw)
// {
//     // corrected = slope_up * raw + offset_up
//     return slope_up * raw + offset_up;
// }

// // tentukan mode turun/naik pakai deadband
// static inline TrendMode decideMode(float rawNow, float rawPrev, TrendMode prevMode)
// {
//     if (isnan(rawPrev))
//         return MODE_DOWN; // default awal: anggap turun
//     if (rawNow < rawPrev - DEAD_BAND)
//         return MODE_DOWN; // jelas menurun
//     if (rawNow > rawPrev + DEAD_BAND)
//         return MODE_UP; // jelas meningkat
//     return prevMode;    // dalam deadband -> pertahankan mode sebelumnya
// }

// void setup()
// {
//     Serial.begin(115200);
//     delay(200);

//     Wire.begin(SDA_PIN, SCL_PIN);

//     if (!oxygen.begin(O2_ADDR))
//     {
//         Serial.println("Init O2 gagal! Cek wiring/alamat.");
//         while (1)
//             delay(1000);
//     }

//     Serial.println("== O2 CAL Dua-Mode (DOWN/UP) ==");
//     Serial.printf("DOWN: slope=%.6f  offset=%.6f\n", slope_down, offset_down);
//     Serial.printf("UP  : slope=%.6f  offset=%.6f\n", slope_up, offset_up);
//     Serial.printf("DEAD_BAND=%.3f %%Vol, SAMPLE=%lu ms\n\n",
//                   DEAD_BAND, (unsigned long)SAMPLE_MS);
//     Serial.println("Header: raw(%Vol)\tmode\tcorrected(%Vol)");
// }

// void loop()
// {
//     // 1) Baca raw dari sensor (library sudah averaging internal via AVG_N)
//     float rawNow = oxygen.getOxygenData(AVG_N);

//     // 2) Tentukan mode (turun/naik) dengan deadband
//     currentMode = decideMode(rawNow, lastRaw, currentMode);

//     // 3) Terapkan kalibrasi sesuai mode
//     float corrected;
//     const char *modeStr;
//     if (currentMode == MODE_UP)
//     {
//         corrected = applyCalUp(rawNow);
//         modeStr = "UP";
//     }
//     else
//     { // MODE_DOWN & MODE_UNKNOWN -> pakai DOWN
//         corrected = applyCalDown(rawNow);
//         modeStr = (currentMode == MODE_DOWN) ? "DOWN" : "INIT(DOWN)";
//     }

//     // 4) Print ringkas (raw, mode, corrected)
//     Serial.printf("%.3f\t%s\t%.3f\n", rawNow, modeStr, corrected);

//     // 5) Simpan raw untuk deteksi arah berikutnya
//     lastRaw = rawNow;

//     delay(SAMPLE_MS);
// }
