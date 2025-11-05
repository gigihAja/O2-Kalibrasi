// // #include <Arduino.h>
// // #include <Wire.h>
// // #include <Preferences.h>
// // #include "DFRobot_OxygenSensor.h"

// // const uint8_t O2_ADDR = 0x73;
// // const int SDA_PIN = 21;
// // const int SCL_PIN = 22;
// // const int BTN_PIN = 18;

// // DFRobot_OxygenSensor oxygen;
// // Preferences prefs;

// // float offsetCal = 0.0f; // %Vol
// // float refValue = 20.6f; // <<<<< GANTI ini sesuai angka dari alat referensi saat kamu mau kalibrasi
// //                         // kamu bisa ubah ini jadi variabel yang diset lewat Serial biar fleksibel

// // unsigned long lastPrint = 0;

// // void loadOffsetFromNVS()
// // {
// //     prefs.begin("o2cal1pt", true); // read-only
// //     offsetCal = prefs.getFloat("offset", 0.0f);
// //     prefs.end();
// // }

// // void saveOffsetToNVS()
// // {
// //     prefs.begin("o2cal1pt", false); // write
// //     prefs.putFloat("offset", offsetCal);
// //     prefs.end();
// // }

// // float readRawO2()
// // {
// //     return oxygen.getOxygenData(20);
// // }

// // float readCorrectedO2()
// // {
// //     return readRawO2() + offsetCal;
// // }

// // void setup()
// // {
// //     Serial.begin(115200);
// //     delay(200);

// //     Wire.begin(SDA_PIN, SCL_PIN);

// //     if (!oxygen.begin(O2_ADDR))
// //     {
// //         Serial.println("Init O2 gagal, cek wiring!");
// //         while (1)
// //             delay(1000);
// //     }

// //     pinMode(BTN_PIN, INPUT_PULLUP); // aktif LOW

// //     loadOffsetFromNVS();

// //     Serial.println("=== STARTUP (single-point offset cal) ===");
// //     Serial.printf("Loaded offsetCal=%.4f from NVS\n", offsetCal);
// //     Serial.println("Tekan tombol untuk FORCE MATCH ke refValue.");
// //     Serial.printf("refValue sekarang diset: %.3f %%Vol\n", refValue);
// //     Serial.println("Ubah refValue di kode/serial sesuai angka alat referensi sebelum pencet tombol.");
// // }

// // void loop()
// // {
// //     // tampilkan bacaan tiap ~500ms
// //     if (millis() - lastPrint > 500)
// //     {
// //         float raw = readRawO2();
// //         float cor = raw + offsetCal;
// //         Serial.printf("O2 raw=%.2f  corr=%.2f %%Vol (offset=%.3f)\n", raw, cor, offsetCal);
// //         lastPrint = millis();
// //     }

// //     // cek tombol
// //     bool pressed = (digitalRead(BTN_PIN) == LOW);
// //     static bool prevPressed = false;

// //     if (pressed && !prevPressed)
// //     {
// //         // tombol baru ditekan -> lakukan kalibrasi offset satu titik
// //         float nowRaw = readRawO2();
// //         offsetCal = refValue - nowRaw;
// //         saveOffsetToNVS();

// //         Serial.println("=== SINGLE-POINT CAL DONE ===");
// //         Serial.printf("refValue=%.3f\n", refValue);
// //         Serial.printf("nowRaw =%.3f\n", nowRaw);
// //         Serial.printf("offsetCal (baru)=%.3f\n", offsetCal);
// //         Serial.println("Offset disimpan ke flash (NVS). Setelah reboot akan dipakai lagi.");
// //         delay(500); // debounce kecil buat hindari retrigger cepat
// //     }

// //     prevPressed = pressed;
// // }

// // nitip 2
// #include <Arduino.h>
// #include <Wire.h>
// #include <Preferences.h>
// #include "DFRobot_OxygenSensor.h"

// // ================== KONFIGURASI HARDWARE ==================
// const uint8_t O2_ADDR = 0x73; // alamat I2C sensor O2 kamu
// const int SDA_PIN = 21;
// const int SCL_PIN = 22;
// const int BTN_PIN = 18; // tombol ke GND, internal pull-up

// // ================== OBJEK GLOBAL ==================
// DFRobot_OxygenSensor oxygen;
// Preferences prefs;

// // ================== VARIABEL KALIBRASI ==================
// // offsetCal: koreksi yang ditambahin ke bacaan raw supaya cocok referensi
// // slopeCal : saat ini kita pakai 1.0 (single-point), tapi kita print biar future-proof
// float offsetCal = 0.0f;
// float slopeCal = 1.0f;

// // refValue: angka referensi yang kamu percaya sekarang (misal dari alat komersial).
// // NANTI bisa diubah langsung lewat Serial Monitor tanpa reflash.
// float refValue = 20.6f;

// // timing print
// unsigned long lastPrintMs = 0;

// // ================== FUNGSI UTIL ==================
// void loadOffsetFromNVS()
// {
//     prefs.begin("o2cal1pt", true); // namespace "o2cal1pt", read-only
//     offsetCal = prefs.getFloat("offset", 0.0f);
//     prefs.end();
// }

// void saveOffsetToNVS()
// {
//     prefs.begin("o2cal1pt", false); // write mode
//     prefs.putFloat("offset", offsetCal);
//     prefs.end();
// }

// // baca nilai mentah sensor (%Vol langsung dari modul DFRobot)
// float readRawO2()
// {
//     return oxygen.getOxygenData(20);
// }

// // terapkan koreksi linear (untuk sekarang slopeCal=1.0)
// float readCorrectedO2()
// {
//     float raw = readRawO2();
//     return slopeCal * raw + offsetCal;
// }

// // Parse input serial sederhana:
// // format command: "REF 20.63"
// // artinya set refValue = 20.63
// void handleSerialInput()
// {
//     static String buf;

//     while (Serial.available() > 0)
//     {
//         char c = Serial.read();

//         // newline / enter menandakan akhir command
//         if (c == '\n' || c == '\r')
//         {
//             if (buf.length() > 0)
//             {
//                 // proses command di buf
//                 // contoh: "REF 20.63"
//                 // kita pecah jadi token pertama dan angka
//                 buf.trim();

//                 // cari spasi pertama
//                 int sp = buf.indexOf(' ');
//                 String cmd, arg;
//                 if (sp == -1)
//                 {
//                     cmd = buf;
//                     arg = "";
//                 }
//                 else
//                 {
//                     cmd = buf.substring(0, sp);
//                     arg = buf.substring(sp + 1);
//                 }

//                 cmd.toUpperCase();

//                 if (cmd == "REF")
//                 {
//                     // coba convert arg ke float
//                     if (arg.length() > 0)
//                     {
//                         float newRef = arg.toFloat();
//                         if (newRef != 0.0f || arg.startsWith("0") || arg.startsWith("0."))
//                         {
//                             refValue = newRef;
//                             Serial.println();
//                             Serial.println("=== UPDATE refValue VIA SERIAL ===");
//                             Serial.printf("refValue sekarang: %.3f %%Vol\n", refValue);
//                             Serial.println("Tekan tombol untuk kalibrasi pakai nilai ini.");
//                             Serial.println();
//                         }
//                         else
//                         {
//                             Serial.println("REF gagal: argumen tidak valid.");
//                         }
//                     }
//                     else
//                     {
//                         Serial.println("REF gagal: butuh angka. Contoh: REF 20.63");
//                     }
//                 }

//                 else if (cmd == "STATUS")
//                 {
//                     Serial.println();
//                     Serial.println("=== STATUS ===");
//                     Serial.printf("refValue   : %.3f %%Vol\n", refValue);
//                     Serial.printf("offsetCal  : %.6f\n", offsetCal);
//                     Serial.printf("slopeCal   : %.6f\n", slopeCal);
//                     Serial.println("Rumus: corrected = slopeCal * raw + offsetCal");
//                     Serial.println();
//                 }

//                 else
//                 {
//                     Serial.println("Perintah tidak dikenal. Pakai:");
//                     Serial.println("  REF <angka>   -> set refValue");
//                     Serial.println("  STATUS        -> lihat kondisi kalibrasi");
//                 }
//             }
//             buf = ""; // reset buffer setelah command dieksekusi
//         }
//         else
//         {
//             // kumpulin char biasa
//             // hindari overflow, tapi String di Arduino biasanya dynamic
//             buf += c;
//         }
//     }
// }

// // ================== SETUP ==================
// void setup()
// {
//     Serial.begin(115200);
//     delay(200);

//     Wire.begin(SDA_PIN, SCL_PIN);

//     if (!oxygen.begin(O2_ADDR))
//     {
//         Serial.println("Init sensor O2 gagal! Cek wiring / alamat I2C.");
//         while (1)
//             delay(1000);
//     }

//     pinMode(BTN_PIN, INPUT_PULLUP); // tombol aktif LOW

//     // load offset lama dari NVS supaya koreksi kebawa setelah restart
//     loadOffsetFromNVS();

//     Serial.println("=== STARTUP: MODE PENGAMBILAN DATA + KALIBRASI ===");
//     Serial.printf("offsetCal dari NVS: %.6f\n", offsetCal);
//     Serial.printf("slopeCal (default): %.6f\n", slopeCal);
//     Serial.printf("refValue awal     : %.3f %%Vol\n", refValue);
//     Serial.println();
//     Serial.println("Perintah Serial:");
//     Serial.println("  REF <angka>   -> set refValue (contoh: REF 20.63)");
//     Serial.println("  STATUS        -> print status kalibrasi");
//     Serial.println();
//     Serial.println("Kalibrasi fisik:");
//     Serial.println("  1. Pastikan sensor dan alat referensi lihat gas yg sama");
//     Serial.println("  2. Ketik REF <angka_alat_referensi> lalu Enter");
//     Serial.println("  3. Tekan tombol (GPIO18 ke GND) untuk commit kalibrasi");
//     Serial.println();
//     Serial.println("Output loop: RAW(%Vol)\tCORRECTED(%Vol)");
//     Serial.println();
// }

// // ================== LOOP ==================
// void loop()
// {
//     // 1. handle input dari Serial Monitor (user bisa kirim "REF 20.63")
//     handleSerialInput();

//     // 2. cetak pembacaan tiap ~500ms
//     if (millis() - lastPrintMs > 500)
//     {
//         float rawVal = readRawO2();
//         float corVal = slopeCal * rawVal + offsetCal;

//         Serial.printf("%.3f\t%.3f\n", rawVal, corVal);

//         lastPrintMs = millis();
//     }

//     // 3. cek tombol untuk TRIGGER kalibrasi
//     static bool prevBtn = true;                  // HIGH saat idle
//     bool btnNow = (digitalRead(BTN_PIN) == LOW); // LOW = ditekan sekarang

//     if (btnNow && !prevBtn)
//     {
//         // Tombol baru ditekan ⇒ lakukan kalibrasi single-point

//         float rawNow = readRawO2();

//         // Kita mau corrected == refValue.
//         // corrected = slopeCal * rawNow + offsetCal
//         // dengan slopeCal = 1.0:
//         // offsetCal = refValue - rawNow
//         slopeCal = 1.0f;
//         offsetCal = refValue - rawNow;

//         // simpan offset ke NVS biar persist setelah reboot
//         saveOffsetToNVS();

//         Serial.println();
//         Serial.println("=== KALIBRASI DILAKUKAN ===");
//         Serial.printf("refValue (alat referensi)   : %.3f %%Vol\n", refValue);
//         Serial.printf("rawNow   (sensor kamu)      : %.3f %%Vol\n", rawNow);
//         Serial.printf("==> slopeCal (gain)         : %.6f\n", slopeCal);
//         Serial.printf("==> offsetCal               : %.6f\n", offsetCal);
//         Serial.println("Rumus aktif sekarang:");
//         Serial.println("corrected = slopeCal * raw + offsetCal");
//         Serial.println("(slopeCal=1.0 -> cuma geser offset)");
//         Serial.println("Offset disimpan. Setelah reboot masih berlaku.\n");

//         delay(250); // debounce, cegah spam
//     }

//     prevBtn = btnNow;
// }
