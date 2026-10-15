#include "eink_waveshare_v2_i.h"

/*
 * Waveshare V2 - Profile pro Displaygroesse.
 *
 * DIESE DATEI WIRD VON EINEM SKRIPT ERZEUGT (Byte-Anzahlen sind gezaehlt).
 *
 * Quellen: oeffentliche Waveshare-Treiber, github.com/waveshareteam/e-Paper
 *   4.2"  -> getesteter Prototyp (entspricht EPD_4in2_V2 Init_Fast)
 *   2.13" -> EPD_2in13_V4.c  (Init_Fast + TurnOnDisplay_Fast)   UNGETESTET
 *   2.9"  -> EPD_2in9_V2.c   (Init mit LUT WS_20_30)            UNGETESTET
 *   2.7"  -> EPD_2in7_V2.c   (Init_Fast + TurnOnDisplay_Fast)   UNGETESTET
 *   7.5"  -> EPD_7in5_V2.c   (Init, Chip UC8179)                UNGETESTET
 */

#define OP_END   EINK_WAVESHARE_V2_OP_END
#define OP_CMD   EINK_WAVESHARE_V2_OP_CMD
#define OP_DATA  EINK_WAVESHARE_V2_OP_DATA
#define OP_DELAY EINK_WAVESHARE_V2_OP_DELAY
#define OP_BUSY  EINK_WAVESHARE_V2_OP_BUSY

/* ---- 4.2 inch V2 (GETESTET) ---- */
static const uint8_t init_4n2[] = {
    OP_CMD, 0x21,  /* Display Update Control 1 (BW-RAM invertiert) */
    OP_DATA, 2, 0x48, 0x00,
    OP_CMD, 0x3C,  /* Border */
    OP_DATA, 1, 0x05,
    OP_CMD, 0x1A,  /* Temperatur fuer Schnellmodus */
    OP_DATA, 1, 0x6E,
    OP_CMD, 0x22,  /* Temperatur laden */
    OP_DATA, 1, 0x91,
    OP_CMD, 0x20,  /* Aktivieren */
    OP_DELAY, 0x00, 0x64,  /* 100 ms */
    OP_CMD, 0x11,  /* Data entry mode */
    OP_DATA, 1, 0x03,
    OP_CMD, 0x44,  /* RAM X: 0..49 (400 px) */
    OP_DATA, 2, 0x00, 0x31,
    OP_CMD, 0x45,  /* RAM Y: 0..299 */
    OP_DATA, 4, 0x00, 0x00, 0x2B, 0x01,
    OP_CMD, 0x4E,  /* X-Zaehler */
    OP_DATA, 1, 0x00,
    OP_CMD, 0x4F,  /* Y-Zaehler */
    OP_DATA, 2, 0x00, 0x00,
    OP_DELAY, 0x00, 0x64,  /* 100 ms */
    OP_END,
};
static const uint8_t refresh_4n2[] = {
    OP_CMD, 0x22,  /* Schnell-Refresh */
    OP_DATA, 1, 0xC7,
    OP_CMD, 0x20,  /* Aktivieren */
    OP_END,
};

/* ---- 2.13 inch V2 (test) - EPD_2in13_V4 ---- */
static const uint8_t init_2n13[] = {
    OP_CMD, 0x12,  /* Soft-Reset */
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_CMD, 0x18,  /* interner Temperatursensor */
    OP_DATA, 1, 0x80,
    OP_CMD, 0x11,  /* Data entry mode */
    OP_DATA, 1, 0x03,
    OP_CMD, 0x44,  /* RAM X: 0..15 (122 px) */
    OP_DATA, 2, 0x00, 0x0F,
    OP_CMD, 0x45,  /* RAM Y: 0..249 */
    OP_DATA, 4, 0x00, 0x00, 0xF9, 0x00,
    OP_CMD, 0x4E,  /* X-Zaehler */
    OP_DATA, 1, 0x00,
    OP_CMD, 0x4F,  /* Y-Zaehler */
    OP_DATA, 2, 0x00, 0x00,
    OP_CMD, 0x22,  /* Temperatur laden */
    OP_DATA, 1, 0xB1,
    OP_CMD, 0x20,
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_CMD, 0x1A,  /* Temperatur fuer Schnellmodus */
    OP_DATA, 2, 0x64, 0x00,
    OP_CMD, 0x22,  /* Temperatur laden */
    OP_DATA, 1, 0x91,
    OP_CMD, 0x20,
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_END,
};
static const uint8_t refresh_2n13[] = {
    OP_CMD, 0x22,  /* Schnell-Refresh */
    OP_DATA, 1, 0xC7,
    OP_CMD, 0x20,  /* Aktivieren */
    OP_END,
};

/* ---- 2.9 inch V2 (test) - EPD_2in9_V2 ---- */
static const uint8_t init_2n9[] = {
    OP_CMD, 0x12,  /* Soft-Reset */
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_CMD, 0x01,  /* Driver output control (296 Zeilen) */
    OP_DATA, 3, 0x27, 0x01, 0x00,
    OP_CMD, 0x11,  /* Data entry mode */
    OP_DATA, 1, 0x03,
    OP_CMD, 0x44,  /* RAM X: 0..15 (128 px) */
    OP_DATA, 2, 0x00, 0x0F,
    OP_CMD, 0x45,  /* RAM Y: 0..295 */
    OP_DATA, 4, 0x00, 0x00, 0x27, 0x01,
    OP_CMD, 0x21,  /* Display Update Control 1 */
    OP_DATA, 2, 0x00, 0x80,
    OP_CMD, 0x4E,  /* X-Zaehler */
    OP_DATA, 1, 0x00,
    OP_CMD, 0x4F,  /* Y-Zaehler */
    OP_DATA, 2, 0x00, 0x00,
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_CMD, 0x32,  /* Wellenform-Tabelle (LUT WS_20_30) */
    OP_DATA, 153,
        0x80, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
        0x10, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
        0x80, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
        0x10, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x14, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x0A, 0x00, 0x0A, 0x0A,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x08, 0x00, 0x01,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00,
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_CMD, 0x3F,
    OP_DATA, 1, 0x22,
    OP_CMD, 0x03,  /* Gate-Spannung */
    OP_DATA, 1, 0x17,
    OP_CMD, 0x04,  /* Source-Spannungen */
    OP_DATA, 3, 0x41, 0x00, 0x32,
    OP_CMD, 0x2C,  /* VCOM */
    OP_DATA, 1, 0x36,
    OP_END,
};
static const uint8_t refresh_2n9[] = {
    OP_CMD, 0x22,  /* Refresh */
    OP_DATA, 1, 0xC7,
    OP_CMD, 0x20,  /* Aktivieren */
    OP_END,
};

/* ---- 2.7 inch V2 (test) - EPD_2in7_V2 ---- */
static const uint8_t init_2n7[] = {
    OP_CMD, 0x12,  /* Soft-Reset */
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_CMD, 0x18,  /* interner Temperatursensor */
    OP_DATA, 1, 0x80,
    OP_CMD, 0x22,  /* Temperatur laden */
    OP_DATA, 1, 0xB1,
    OP_CMD, 0x20,
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_CMD, 0x1A,  /* Temperatur fuer Schnellmodus */
    OP_DATA, 2, 0x64, 0x00,
    OP_CMD, 0x45,  /* RAM Y: 0..263 */
    OP_DATA, 4, 0x00, 0x00, 0x07, 0x01,
    OP_CMD, 0x4F,  /* Y-Zaehler */
    OP_DATA, 2, 0x00, 0x00,
    OP_CMD, 0x11,  /* Data entry mode */
    OP_DATA, 1, 0x03,
    OP_CMD, 0x22,  /* Temperatur laden */
    OP_DATA, 1, 0x91,
    OP_CMD, 0x20,
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_END,
};
static const uint8_t refresh_2n7[] = {
    OP_CMD, 0x22,  /* Schnell-Refresh */
    OP_DATA, 1, 0xC7,
    OP_CMD, 0x20,  /* Aktivieren */
    OP_END,
};

/* ---- 7.5 inch V2 (test) - EPD_7in5_V2, Chip UC8179 ---- */
static const uint8_t init_7n5[] = {
    OP_CMD, 0x01,  /* Power setting */
    OP_DATA, 4, 0x07, 0x07, 0x3F, 0x3F,
    OP_CMD, 0x06,  /* Booster soft start */
    OP_DATA, 4, 0x17, 0x17, 0x28, 0x17,
    OP_CMD, 0x04,  /* Power on */
    OP_DELAY, 0x00, 0x64,  /* 100 ms */
    OP_BUSY, 50,  /* warten bis fertig (max 5,0 s) */
    OP_CMD, 0x00,  /* Panel setting (KW-Modus) */
    OP_DATA, 1, 0x1F,
    OP_CMD, 0x61,  /* Aufloesung 800 x 480 */
    OP_DATA, 4, 0x03, 0x20, 0x01, 0xE0,
    OP_CMD, 0x15,
    OP_DATA, 1, 0x00,
    OP_CMD, 0x50,  /* VCOM / Data interval */
    OP_DATA, 2, 0x10, 0x07,
    OP_CMD, 0x60,  /* TCON */
    OP_DATA, 1, 0x22,
    OP_END,
};
static const uint8_t refresh_7n5[] = {
    OP_CMD, 0x12,  /* Refresh */
    OP_DELAY, 0x00, 0x64,  /* 100 ms */
    OP_END,
};

static const EinkWaveshareV2Profile profile_4n2 = {
    .name = "4.2 V2",
    .init_script = init_4n2,
    .pass_count = 1,
    /* Mit 0x21 = 0x48 ist der BW-RAM im Display invertiert, darum hier ~
       (am echten Tag getestet: so stimmt Schwarz/Weiss) */
    .passes = {{.ram_cmd = 0x24, .invert = true}},
    .refresh_script = refresh_4n2,
    .refresh_delay_ms = 1000,
    .busy_ready_value = 0,
};

static const EinkWaveshareV2Profile profile_2n13 = {
    .name = "2.13 V2",
    .init_script = init_2n13,
    .pass_count = 1,
    .passes = {{.ram_cmd = 0x24, .invert = false}},
    .refresh_script = refresh_2n13,
    .refresh_delay_ms = 500,
    .busy_ready_value = 0,
};

static const EinkWaveshareV2Profile profile_2n9 = {
    .name = "2.9 V2",
    .init_script = init_2n9,
    .pass_count = 1,
    .passes = {{.ram_cmd = 0x24, .invert = false}},
    .refresh_script = refresh_2n9,
    .refresh_delay_ms = 500,
    .busy_ready_value = 0,
};

static const EinkWaveshareV2Profile profile_2n7 = {
    .name = "2.7 V2",
    .init_script = init_2n7,
    .pass_count = 1,
    .passes = {{.ram_cmd = 0x24, .invert = false}},
    .refresh_script = refresh_2n7,
    .refresh_delay_ms = 500,
    .busy_ready_value = 0,
};

/* UC8179: 0x10 = altes Bild (normal), 0x13 = neues Bild (umgedreht),
   Busy-Leitung ist hier LOW solange beschaeftigt -> fertig = 1 */
static const EinkWaveshareV2Profile profile_7n5 = {
    .name = "7.5 V2",
    .init_script = init_7n5,
    .pass_count = 2,
    .passes = {{.ram_cmd = 0x10, .invert = false}, {.ram_cmd = 0x13, .invert = true}},
    .refresh_script = refresh_7n5,
    .refresh_delay_ms = 1000,
    .busy_ready_value = 1,
};

const EinkWaveshareV2Profile* eink_waveshare_v2_profile_get(uint32_t profile_id) {
    switch(profile_id) {
    case EinkWaveshareV2Profile4n2:
        return &profile_4n2;
    case EinkWaveshareV2Profile2n13:
        return &profile_2n13;
    case EinkWaveshareV2Profile2n9:
        return &profile_2n9;
    case EinkWaveshareV2Profile2n7:
        return &profile_2n7;
    case EinkWaveshareV2Profile7n5:
        return &profile_7n5;
    default:
        return NULL;
    }
}
