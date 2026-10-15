# NFC-Eink with Waveshare V2 tags: build and use

## What's new?

- New manufacturer **"Waveshare V2"** for tags with the new NFC chip
- Writing works, emulation does not (that's why V2 doesn't show up in the Emulate menu)
- For every image, the matching V2 size appears in the Write list, regardless of the writing mode

| Entry in the Write list | Status |
|---|---|
| Waveshare 4.2 inch V2 | **tested on a real tag** |
| Waveshare 2.13 V2 (test) | untested, from Waveshare driver EPD_2in13_V4 |
| Waveshare 2.9 V2 (test) | untested, from Waveshare driver EPD_2in9_V2 |
| Waveshare 2.7 V2 (test) | untested, from Waveshare driver EPD_2in7_V2 |
| Waveshare 7.5 V2 (test) | untested, from Waveshare driver EPD_7in5_V2 (different controller chip) |

### How it works

The NFC chip in the V2 tag is just a pass-through to the display controller:
`74 99` = command to the display, `74 9A` = data to the display, `74 97` = reset, `74 9B` = "are you done?" (busy).
What gets sent to the display is stored per size as a **profile** in
`nfc_eink_screen/waveshare_v2/eink_waveshare_v2_profiles.c`. The profiles are taken from
Waveshare's public drivers (github.com/waveshareteam/e-Paper).

### Files

| File | Purpose |
|---|---|
| `nfc_eink_screen/waveshare_v2/eink_waveshare_v2.c` | V2 driver skeleton |
| `nfc_eink_screen/waveshare_v2/eink_waveshare_v2_i.h` | Commands, state machine, settings |
| `nfc_eink_screen/waveshare_v2/eink_waveshare_v2_poller.c` | Flow: connect, reset, run profile, send image |
| `nfc_eink_screen/waveshare_v2/eink_waveshare_v2_profiles.c` | Command sequences per display size |
| `nfc_eink_screen/waveshare_v2/eink_waveshare_v2_profile_ids.h` | Profile IDs |
| `nfc_eink_screen/nfc_eink_screen_infos.c` / `.h` | V2 entries in the screen list |
| `nfc_eink_screen/nfc_eink_screen.c` / `.h` | Driver registration, emulation check |
| `scenes/scene_choose_manufacturer.c` | V2 hidden in the Emulate menu |
| `scenes/scene_choose_screen.c` | Matching V2 size always shown in the Write list |
| `application.fam` | Separate app name "NFC-Eink V2", version 1.2 |

## Building the app (one-time setup)

You need **Python 3** on your computer (python.org; on Windows, tick
"Add Python to PATH" during installation).

1. Open a terminal (Mac: "Terminal", Windows: "Command Prompt").
2. Install the build tool:
   ```
   pip install ufbt
   ```
3. Point the tool at **Momentum** (matching your firmware):
   ```
   ufbt update --index-url=https://up.momentum-fw.dev/firmware/directory.json
   ```
   For the official firmware, just run `ufbt update` instead.

## Building the app (every time)

1. Unzip the archive. In the terminal, change into the folder, for example:
   ```
   cd Downloads/nfc_eink
   ```
2. Build:
   ```
   ufbt
   ```
3. The finished file is **`dist/nfc_eink_v2.fap`**.

Tip: with the Flipper connected via USB (and qFlipper closed!), `ufbt launch`
builds the app and starts it directly on the Flipper.

## Copying to the Flipper

Copy `nfc_eink_v2.fap` to `SD Card/apps/NFC/` using qFlipper or the Flipper mobile app.
It shows up on the Flipper as a separate app **"NFC-Eink V2"** under Apps → NFC
(the built-in "NFC-Eink" stays alongside it).

If the Flipper reports "API mismatch", the firmware and the build tool don't match:
update the firmware on the Flipper to the latest version, then repeat
`ufbt update ...` (step 3 above) and rebuild.

## Usage

1. Get an image from your phone as usual: on the Flipper choose **Emulate → Waveshare**,
   select "4.2 inch" in the Waveshare app, send the image to the Flipper, then save it.
2. In the **"NFC-Eink V2"** app, choose the saved image → **Write** → **"Waveshare 4.2 inch V2"**.
3. Hold the tag against the back of the Flipper and keep it still until "Updating" has finished.

## If something isn't right

In `nfc_eink_screen/waveshare_v2/eink_waveshare_v2_profiles.c`, in the profile for that size, then rebuild:

- **Image is inverted** (black and white swapped): flip the `.invert` value under `.passes`
  (`true` ↔ `false`). For the 4.2", `true` is correct (tested).
- **Transfer aborts**: in `nfc_eink_screen_infos.c`, change `.data_block_size = 150`
  to `50` in the V2 entry (slower; both worked on the 4.2").

## Trying a "(test)" size

If you have one of these tags: get an image in the matching size (Emulate → Waveshare,
select the size in the phone app), then Write → "Waveshare … V2 (test)".
A wrong command normally won't damage the display; at worst it stays blank
or shows stripes. Feedback (a photo) helps to fix the profile.
