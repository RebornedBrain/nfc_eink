#pragma once

/*
 * Waveshare V2 - interne Definitionen.
 *
 * Die V2-Tags sprechen ISO14443-4A (ISO-DEP). Alle Befehle beginnen mit 0x74.
 * Der NFC-Chip im Tag ist nur ein "Durchreicher" zum Display-Controller:
 *   74 99 .. = Befehl an das Display     74 9A .. = Daten an das Display
 *   74 97 .. = Reset-Leitung             74 9B .. = Busy-Leitung abfragen
 *
 * Was genau an das Display geschickt wird, steht pro Groesse in einem
 * "Profil" (eink_waveshare_v2_profiles.c). Die Profile stammen aus den
 * oeffentlichen Waveshare-Treibern (github.com/waveshareteam/e-Paper).
 * Nur das 4.2"-Profil ist am echten Tag getestet.
 */

#include "../nfc_eink_screen_i.h"
#include <nfc/protocols/nfc_generic_event.h>
#include <nfc/protocols/iso14443_4a/iso14443_4a.h>
#include <nfc/protocols/iso14443_4a/iso14443_4a_poller.h>

/* ---- Einstellungen, die man bei Bedarf aendern kann -------------------- */

/* 1 = zwei 0x00-Bytes an jedes Bildpaket anhaengen ("Trailer", im Prototyp: ja) */
#define EINK_WAVESHARE_V2_USE_TRAILER (1)

/* Wie oft der Verbindungsaufbau wiederholt wird, bevor ein Fehler kommt */
#define EINK_WAVESHARE_V2_MAX_RETRY (5)

/* Busy-Abfrage nach dem Aktualisieren: 80 x 250 ms = 20 Sekunden */
#define EINK_WAVESHARE_V2_BUSY_MAX_POLLS (80)
#define EINK_WAVESHARE_V2_BUSY_DELAY_MS  (250)

/* ---- Befehle ----------------------------------------------------------- */

#define EINK_WAVESHARE_V2_PREFIX    0x74
#define EINK_WAVESHARE_V2_CMD_AUTH  0xB1
#define EINK_WAVESHARE_V2_CMD_RESET 0x97
#define EINK_WAVESHARE_V2_CMD_DISP  0x99 /* Befehl an den Display-Controller */
#define EINK_WAVESHARE_V2_CMD_DATA  0x9A /* Daten an den Display-Controller  */
#define EINK_WAVESHARE_V2_CMD_BUSY  0x9B /* Busy-Status abfragen             */

/* ---- Profile ----------------------------------------------------------- */

/* Kleine "Befehlslisten" (Skripte). Jeder Eintrag beginnt mit einem Op-Code:
 *   OP_CMD  c          -> Befehl c an das Display
 *   OP_DATA n d1..dn   -> n Datenbytes an das Display
 *   OP_DELAY hi lo     -> (hi*256+lo) Millisekunden warten
 *   OP_BUSY t          -> warten bis das Display nicht mehr busy ist (max t x 100 ms)
 *   OP_END             -> Ende der Liste
 */
#define EINK_WAVESHARE_V2_OP_END   0x00
#define EINK_WAVESHARE_V2_OP_CMD   0x01
#define EINK_WAVESHARE_V2_OP_DATA  0x02
#define EINK_WAVESHARE_V2_OP_DELAY 0x03
#define EINK_WAVESHARE_V2_OP_BUSY  0x04

#define EINK_WAVESHARE_V2_MAX_PASSES (2)

/* Ein "Durchgang" = das ganze Bild einmal an einen RAM-Befehl schicken */
typedef struct {
    uint8_t ram_cmd; /* z.B. 0x24 (SSD16xx) oder 0x10/0x13 (UC8179) */
    bool invert; /* Bytes vor dem Senden umdrehen (~) */
} EinkWaveshareV2ImagePass;

typedef struct {
    const char* name; /* nur fuers Log */
    const uint8_t* init_script;
    uint8_t pass_count;
    EinkWaveshareV2ImagePass passes[EINK_WAVESHARE_V2_MAX_PASSES];
    const uint8_t* refresh_script;
    uint16_t refresh_delay_ms; /* Pause nach dem Refresh-Befehl */
    uint8_t busy_ready_value; /* Antwort auf 74 9B, die "fertig" bedeutet */
} EinkWaveshareV2Profile;

#include "eink_waveshare_v2_profile_ids.h"

const EinkWaveshareV2Profile* eink_waveshare_v2_profile_get(uint32_t profile_id);

/* ---- Ablauf (Zustaende) ------------------------------------------------ */

typedef enum {
    EinkWaveshareV2PollerStateAuth,
    EinkWaveshareV2PollerStateReset,
    EinkWaveshareV2PollerStateInit,
    EinkWaveshareV2PollerStateSendImage,
    EinkWaveshareV2PollerStateRefresh,
    EinkWaveshareV2PollerStateWaitReady,
    EinkWaveshareV2PollerStateFinish,
    EinkWaveshareV2PollerStateError,

    EinkWaveshareV2PollerStateNum
} EinkWaveshareV2PollerState;

typedef struct {
    EinkWaveshareV2PollerState poller_state;
    const EinkWaveshareV2Profile* profile;
    uint8_t retry_cnt;
    uint8_t pass; /* aktueller Bild-Durchgang */
    uint16_t block_number; /* Paket-Zaehler ueber alle Durchgaenge */
    size_t data_index; /* Position im Bild (pro Durchgang) */
} NfcEinkWaveshareV2SpecificContext;

#define eink_waveshare_v2_on_done(instance) \
    nfc_eink_screen_vendor_callback(instance, NfcEinkScreenEventTypeFinish)

#define eink_waveshare_v2_on_target_detected(instance) \
    nfc_eink_screen_vendor_callback(instance, NfcEinkScreenEventTypeTargetDetected)

#define eink_waveshare_v2_on_block_processed(instance) \
    nfc_eink_screen_vendor_callback(instance, NfcEinkScreenEventTypeBlockProcessed)

#define eink_waveshare_v2_on_updating(instance) \
    nfc_eink_screen_vendor_callback(instance, NfcEinkScreenEventTypeUpdating)

NfcCommand eink_waveshare_v2_poller_callback(NfcGenericEvent event, void* context);
