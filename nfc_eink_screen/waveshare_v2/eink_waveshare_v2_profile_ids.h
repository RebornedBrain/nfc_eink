#pragma once

/* Profil-Nummern der Waveshare-V2-Bildschirme. Sie stehen im Feld
   protocol_type_field der Bildschirm-Liste (nfc_eink_screen_infos.c).
   0x100 + n, damit sie nie mit den alten Waveshare-Typnummern (4..16) kollidieren.
   Eigene kleine Datei, damit die Bildschirm-Liste sie ohne weitere Header einbinden kann. */
typedef enum {
    EinkWaveshareV2Profile4n2 = 0x101,
    EinkWaveshareV2Profile2n13 = 0x102,
    EinkWaveshareV2Profile2n9 = 0x103,
    EinkWaveshareV2Profile2n7 = 0x104,
    EinkWaveshareV2Profile7n5 = 0x105,
} EinkWaveshareV2ProfileId;
