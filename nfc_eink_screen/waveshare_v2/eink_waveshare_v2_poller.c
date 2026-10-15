#include "eink_waveshare_v2_i.h"

/*
 * Waveshare V2 - Beschreiben des Tags.
 *
 * Ablauf (fuer alle Groessen gleich):
 *   1. Auth          -> Verbindung freischalten
 *   2. Reset         -> Reset-Leitung des Displays ein/aus
 *   3. Init          -> Init-Liste aus dem Profil abarbeiten
 *   4. SendImage     -> Bild in Paketen schicken (ein Paket pro Durchlauf,
 *                       damit der Fortschrittsbalken mitlaeuft). Manche
 *                       Displays (7.5") brauchen das Bild zweimal.
 *   5. Refresh       -> Refresh-Liste aus dem Profil abarbeiten
 *   6. WaitReady     -> warten, bis das Display fertig ist
 *
 * Was genau ans Display geht, steht im Profil (eink_waveshare_v2_profiles.c).
 */

#define TAG "WSV2_Poller"

#define EINK_WAVESHARE_V2_MAX_DATA_LEN (255)

typedef NfcCommand (
    *EinkWaveshareV2StateHandler)(Iso14443_4aPoller* poller, NfcEinkScreen* screen);

/* ---------------------------------------------------------------------- */
/* Senden                                                                  */
/* ---------------------------------------------------------------------- */

/* Schickt einen Frame. check_ok: Antwort muss mit 90 00 beginnen.
   first_rx (optional): bekommt das erste Antwort-Byte. */
static bool eink_waveshare_v2_xfer(
    Iso14443_4aPoller* poller,
    NfcEinkScreen* screen,
    const uint8_t* data,
    size_t len,
    bool check_ok,
    uint8_t* first_rx) {
    bit_buffer_reset(screen->tx_buf);
    bit_buffer_reset(screen->rx_buf);
    bit_buffer_append_bytes(screen->tx_buf, data, len);

    Iso14443_4aError error = iso14443_4a_poller_send_block(poller, screen->tx_buf, screen->rx_buf);
    if(error != Iso14443_4aErrorNone) {
        FURI_LOG_E(TAG, "Iso14443_4aError: %d", error);
        return false;
    }

    size_t rx_len = bit_buffer_get_size_bytes(screen->rx_buf);
    const uint8_t* rx = bit_buffer_get_data(screen->rx_buf);

    if(first_rx != NULL) *first_rx = (rx_len >= 1) ? rx[0] : 0xFF;

    if(check_ok && (rx_len < 2 || rx[0] != 0x90 || rx[1] != 0x00)) {
        if(rx_len >= 2) {
            FURI_LOG_E(TAG, "Bad response: %02X %02X", rx[0], rx[1]);
        } else {
            FURI_LOG_E(TAG, "Response too short: %u", (unsigned)rx_len);
        }
        return false;
    }
    return true;
}

/* Befehl an den Display-Controller */
static bool eink_waveshare_v2_cmd(Iso14443_4aPoller* poller, NfcEinkScreen* screen, uint8_t cmd) {
    const uint8_t frame[] = {
        EINK_WAVESHARE_V2_PREFIX, EINK_WAVESHARE_V2_CMD_DISP, 0x00, 0x0D, 0x01, cmd};
    return eink_waveshare_v2_xfer(poller, screen, frame, sizeof(frame), true, NULL);
}

/* Daten an den Display-Controller (ohne Trailer, wie im Prototyp) */
static bool eink_waveshare_v2_data(
    Iso14443_4aPoller* poller,
    NfcEinkScreen* screen,
    const uint8_t* d,
    uint8_t n) {
    uint8_t frame[5 + EINK_WAVESHARE_V2_MAX_DATA_LEN];
    frame[0] = EINK_WAVESHARE_V2_PREFIX;
    frame[1] = EINK_WAVESHARE_V2_CMD_DATA;
    frame[2] = 0x00;
    frame[3] = 0x0E;
    frame[4] = n;
    memcpy(&frame[5], d, n);
    return eink_waveshare_v2_xfer(poller, screen, frame, 5 + n, true, NULL);
}

/* Fragt die Busy-Leitung ab, bis sie "fertig" meldet.
   Gibt nur bei einem Funkfehler false zurueck, nicht bei Zeitablauf. */
static bool eink_waveshare_v2_wait_busy(
    Iso14443_4aPoller* poller,
    NfcEinkScreen* screen,
    uint8_t ready_value,
    uint16_t max_polls,
    uint16_t delay_ms) {
    static const uint8_t busy_cmd[] = {
        EINK_WAVESHARE_V2_PREFIX, EINK_WAVESHARE_V2_CMD_BUSY, 0x00, 0x0F, 0x01};

    for(uint16_t p = 0; p < max_polls; p++) {
        uint8_t busy = 0xFF;
        if(!eink_waveshare_v2_xfer(poller, screen, busy_cmd, sizeof(busy_cmd), false, &busy)) {
            return false;
        }
        if(busy == ready_value) return true;
        furi_delay_ms(delay_ms);
    }
    FURI_LOG_D(TAG, "Busy timeout");
    return true;
}

/* Arbeitet eine Befehlsliste (Skript) aus dem Profil ab */
static bool eink_waveshare_v2_run_script(
    Iso14443_4aPoller* poller,
    NfcEinkScreen* screen,
    const EinkWaveshareV2Profile* profile,
    const uint8_t* script) {
    size_t i = 0;
    while(true) {
        const uint8_t op = script[i++];
        switch(op) {
        case EINK_WAVESHARE_V2_OP_END:
            return true;
        case EINK_WAVESHARE_V2_OP_CMD:
            if(!eink_waveshare_v2_cmd(poller, screen, script[i])) return false;
            i += 1;
            break;
        case EINK_WAVESHARE_V2_OP_DATA: {
            const uint8_t n = script[i];
            if(!eink_waveshare_v2_data(poller, screen, &script[i + 1], n)) return false;
            i += 1 + n;
            break;
        }
        case EINK_WAVESHARE_V2_OP_DELAY:
            furi_delay_ms(((uint32_t)script[i] << 8) | script[i + 1]);
            i += 2;
            break;
        case EINK_WAVESHARE_V2_OP_BUSY:
            /* script[i] = max. Wartezeit in 100-ms-Schritten */
            if(!eink_waveshare_v2_wait_busy(
                   poller, screen, profile->busy_ready_value, script[i] * 2, 50))
                return false;
            i += 1;
            break;
        default:
            FURI_LOG_E(TAG, "Bad script op %02X", op);
            return false;
        }
    }
}

/* ---------------------------------------------------------------------- */
/* Zustaende                                                               */
/* ---------------------------------------------------------------------- */

static NfcCommand eink_waveshare_v2_state_auth(Iso14443_4aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Auth");
    NfcEinkWaveshareV2SpecificContext* ctx = screen->device->screen_context;

    static const uint8_t auth[] = {
        EINK_WAVESHARE_V2_PREFIX,
        EINK_WAVESHARE_V2_CMD_AUTH,
        0x00,
        0x00,
        0x08,
        0x00,
        0x11,
        0x22,
        0x33,
        0x44,
        0x55,
        0x66,
        0x77};

    ctx->profile = eink_waveshare_v2_profile_get(screen->data->base.protocol_type_field);
    if(ctx->profile == NULL) {
        FURI_LOG_E(TAG, "No profile for this screen");
        ctx->poller_state = EinkWaveshareV2PollerStateError;
        return NfcCommandContinue;
    }

    if(eink_waveshare_v2_xfer(poller, screen, auth, sizeof(auth), true, NULL)) {
        ctx->retry_cnt = 0;
        ctx->poller_state = EinkWaveshareV2PollerStateReset;
        return NfcCommandContinue;
    }

    /* Tag evtl. noch nicht bereit (zu wenig Strom) -> neu verbinden und nochmal */
    ctx->retry_cnt++;
    if(ctx->retry_cnt >= EINK_WAVESHARE_V2_MAX_RETRY) {
        ctx->poller_state = EinkWaveshareV2PollerStateError;
        return NfcCommandContinue;
    }
    return NfcCommandReset;
}

static NfcCommand eink_waveshare_v2_state_reset(Iso14443_4aPoller* poller, NfcEinkScreen* screen) {
    NfcEinkWaveshareV2SpecificContext* ctx = screen->device->screen_context;
    FURI_LOG_D(TAG, "Reset (%s)", ctx->profile->name);
    ctx->poller_state = EinkWaveshareV2PollerStateError;

    static const uint8_t rst_high[] = {
        EINK_WAVESHARE_V2_PREFIX, EINK_WAVESHARE_V2_CMD_RESET, 0x01, 0x08, 0x00};
    static const uint8_t rst_low[] = {
        EINK_WAVESHARE_V2_PREFIX, EINK_WAVESHARE_V2_CMD_RESET, 0x00, 0x08, 0x00};

    do {
        if(!eink_waveshare_v2_xfer(poller, screen, rst_high, sizeof(rst_high), true, NULL)) break;
        furi_delay_ms(50);
        if(!eink_waveshare_v2_xfer(poller, screen, rst_low, sizeof(rst_low), true, NULL)) break;
        furi_delay_ms(50);
        if(!eink_waveshare_v2_xfer(poller, screen, rst_high, sizeof(rst_high), true, NULL)) break;
        furi_delay_ms(10);

        /* Ab hier zeigt die App den Fortschrittsbalken */
        eink_waveshare_v2_on_target_detected(screen);
        ctx->poller_state = EinkWaveshareV2PollerStateInit;
    } while(false);

    return NfcCommandContinue;
}

static NfcCommand eink_waveshare_v2_state_init(Iso14443_4aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Init");
    NfcEinkWaveshareV2SpecificContext* ctx = screen->device->screen_context;
    const EinkWaveshareV2Profile* profile = ctx->profile;

    ctx->poller_state = EinkWaveshareV2PollerStateError;
    do {
        if(!eink_waveshare_v2_run_script(poller, screen, profile, profile->init_script)) break;

        /* Ersten Bild-Durchgang starten */
        if(!eink_waveshare_v2_cmd(poller, screen, profile->passes[0].ram_cmd)) break;

        ctx->pass = 0;
        ctx->block_number = 0;
        ctx->data_index = 0;
        /* Fortschrittsbalken: Pakete aller Durchgaenge zusammen */
        screen->device->block_total = screen->device->block_total * profile->pass_count;
        ctx->poller_state = EinkWaveshareV2PollerStateSendImage;
    } while(false);

    return NfcCommandContinue;
}

static NfcCommand
    eink_waveshare_v2_state_send_image(Iso14443_4aPoller* poller, NfcEinkScreen* screen) {
    NfcEinkWaveshareV2SpecificContext* ctx = screen->device->screen_context;
    const EinkWaveshareV2Profile* profile = ctx->profile;
    const EinkWaveshareV2ImagePass* pass = &profile->passes[ctx->pass];
    const NfcEinkScreenData* data = screen->data;

    /* Paketgroesse; das letzte Paket ist evtl. kleiner (nichts ueber das Bild hinaus senden) */
    size_t remaining = data->image_size - ctx->data_index;
    uint8_t chunk = data->base.data_block_size;
    if(remaining < chunk) chunk = (uint8_t)remaining;

    /* Frame: 74 9A 00 0E <Laenge> <Bilddaten...> [00 00] */
    uint8_t frame[5 + EINK_WAVESHARE_V2_MAX_DATA_LEN + 2];
    frame[0] = EINK_WAVESHARE_V2_PREFIX;
    frame[1] = EINK_WAVESHARE_V2_CMD_DATA;
    frame[2] = 0x00;
    frame[3] = 0x0E;
    frame[4] = chunk;
    for(uint8_t i = 0; i < chunk; i++) {
        const uint8_t byte = data->image_data[ctx->data_index + i];
        frame[5 + i] = pass->invert ? (uint8_t)~byte : byte;
    }
    size_t len = 5 + chunk;
#if EINK_WAVESHARE_V2_USE_TRAILER
    frame[len++] = 0x00;
    frame[len++] = 0x00;
#endif

    FURI_LOG_D(TAG, "Pass %d block %d", ctx->pass, ctx->block_number);
    if(!eink_waveshare_v2_xfer(poller, screen, frame, len, true, NULL)) {
        ctx->poller_state = EinkWaveshareV2PollerStateError;
        return NfcCommandContinue;
    }

    ctx->data_index += chunk;
    ctx->block_number++;
    screen->device->block_current = ctx->block_number;
    eink_waveshare_v2_on_block_processed(screen);

    if(ctx->data_index >= data->image_size) {
        ctx->pass++;
        if(ctx->pass < profile->pass_count) {
            /* Naechster Durchgang (z.B. 7.5": erst 0x10, dann 0x13) */
            ctx->data_index = 0;
            if(!eink_waveshare_v2_cmd(poller, screen, profile->passes[ctx->pass].ram_cmd)) {
                ctx->poller_state = EinkWaveshareV2PollerStateError;
            }
        } else {
            ctx->poller_state = EinkWaveshareV2PollerStateRefresh;
        }
    }
    return NfcCommandContinue;
}

static NfcCommand
    eink_waveshare_v2_state_refresh(Iso14443_4aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Refresh");
    NfcEinkWaveshareV2SpecificContext* ctx = screen->device->screen_context;
    const EinkWaveshareV2Profile* profile = ctx->profile;

    eink_waveshare_v2_on_updating(screen);
    if(eink_waveshare_v2_run_script(poller, screen, profile, profile->refresh_script)) {
        furi_delay_ms(profile->refresh_delay_ms);
        ctx->poller_state = EinkWaveshareV2PollerStateWaitReady;
    } else {
        ctx->poller_state = EinkWaveshareV2PollerStateError;
    }
    return NfcCommandContinue;
}

static NfcCommand
    eink_waveshare_v2_state_wait_ready(Iso14443_4aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Wait ready");
    NfcEinkWaveshareV2SpecificContext* ctx = screen->device->screen_context;

    /* Auch wenn die Zeit ablaeuft: das Bild ist meist trotzdem da */
    bool ok = eink_waveshare_v2_wait_busy(
        poller,
        screen,
        ctx->profile->busy_ready_value,
        EINK_WAVESHARE_V2_BUSY_MAX_POLLS,
        EINK_WAVESHARE_V2_BUSY_DELAY_MS);

    ctx->poller_state = ok ? EinkWaveshareV2PollerStateFinish : EinkWaveshareV2PollerStateError;
    return NfcCommandContinue;
}

static NfcCommand
    eink_waveshare_v2_state_finish(Iso14443_4aPoller* poller, NfcEinkScreen* screen) {
    UNUSED(poller);
    FURI_LOG_D(TAG, "Finish");
    eink_waveshare_v2_on_done(screen);
    return NfcCommandStop;
}

static NfcCommand eink_waveshare_v2_state_error(Iso14443_4aPoller* poller, NfcEinkScreen* screen) {
    UNUSED(poller);
    FURI_LOG_E(TAG, "Error during writing!");
    nfc_eink_screen_set_error(screen, NfcEinkScreenErrorUnableToWrite);
    return NfcCommandStop;
}

static const EinkWaveshareV2StateHandler handlers[EinkWaveshareV2PollerStateNum] = {
    [EinkWaveshareV2PollerStateAuth] = eink_waveshare_v2_state_auth,
    [EinkWaveshareV2PollerStateReset] = eink_waveshare_v2_state_reset,
    [EinkWaveshareV2PollerStateInit] = eink_waveshare_v2_state_init,
    [EinkWaveshareV2PollerStateSendImage] = eink_waveshare_v2_state_send_image,
    [EinkWaveshareV2PollerStateRefresh] = eink_waveshare_v2_state_refresh,
    [EinkWaveshareV2PollerStateWaitReady] = eink_waveshare_v2_state_wait_ready,
    [EinkWaveshareV2PollerStateFinish] = eink_waveshare_v2_state_finish,
    [EinkWaveshareV2PollerStateError] = eink_waveshare_v2_state_error,
};

NfcCommand eink_waveshare_v2_poller_callback(NfcGenericEvent event, void* context) {
    furi_assert(context);
    NfcEinkScreen* screen = context;
    NfcCommand command = NfcCommandContinue;

    const Iso14443_4aPollerEvent* ev = event.event_data;
    Iso14443_4aPoller* poller = event.instance;

    if(ev->type == Iso14443_4aPollerEventTypeReady) {
        NfcEinkWaveshareV2SpecificContext* ctx = screen->device->screen_context;
        command = handlers[ctx->poller_state](poller, screen);
    }
    /* Bei Iso14443_4aPollerEventTypeError einfach weiter versuchen */

    if(command == NfcCommandReset || command == NfcCommandStop) iso14443_4a_poller_halt(poller);
    return command;
}
