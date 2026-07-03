# 🎵 ESP32 MP3 Player

A handheld, SD-card-based MP3 player built on an ESP32, with an animated OLED interface, a two-level album/track menu, and full playback controls. Music is decoded by a DFPlayer Mini and played through a headphone amplifier.

> Built by **Georgy Rached**

---

## ✨ Features

- **Two-level menu** — browse by album, then by track
- **Animated boot screen** — converging stars + title intro
- **Now-playing screen** with:
  - Drifting star background animation
  - Scrolling marquee for long song titles
  - Play / pause icon
  - Skip / back arrows (greyed at the first/last track)
  - Approximate progress bar
- **Playback controls** — play/pause, next, previous
- **Double-press gestures** — double-tap Up to skip, double-tap Down to go back
- **Auto-advance** — automatically plays the next track when one ends
- **Custom song names** displayed on screen
- **Smooth, non-blocking UI** — animations, audio, and input all run concurrently

---

## 🛠️ Hardware

| Component | Purpose |
|-----------|---------|
| ESP32 DOIT DevKit V1 | Main microcontroller |
| DFPlayer Mini (MP3-TF-16P) | MP3 decoding + microSD playback |
| SSD1306 128×64 OLED (I2C) | Display |
| MAX4410 headphone amplifier | Drives earphones (with volume knob) |
| 3.5mm panel-mount jack | Audio output |
| 4 × push buttons | Navigation & control |
| 9V rechargeable battery + buck converter (to 5V) | Powers the whole device |
| ON/OFF switch (SPST) | Cuts power between battery and boards |
| 1× electrolytic capacitor (470–1000µF) | Power smoothing for the DFPlayer |

---

## 🔌 Wiring

![Wiring Diagram](wiring_diagram.png)

> Full pin-by-pin connections below.

### DFPlayer Mini (hardware Serial2)
| DFPlayer | ESP32 |
|----------|-------|
| VCC | VIN (5V) |
| GND | GND |
| RX  | GPIO17 |
| TX  | GPIO16 |

> Add a 470–1000µF capacitor across the DFPlayer's VCC and GND, close to the module, to prevent brownout resets and reduce audio noise.

### OLED (I2C)
| OLED | ESP32 |
|------|-------|
| SDA | GPIO21 |
| SCL | GPIO22 |
| VCC | 3.3V |
| GND | GND |

### Buttons (each: pin → button → GND, using internal pull-ups)
| Button | ESP32 |
|--------|-------|
| Up | GPIO25 |
| Down | GPIO26 |
| Select | GPIO27 |
| Back | GPIO14 |

### Audio output
DFPlayer `DAC_L` / `DAC_R` / `GND` → MAX4410 amp input → earphones (or panel jack).

### Power
The entire device runs from a **9V rechargeable battery** stepped **down** to **5V** by a **buck converter**. That 5V rail feeds the ESP32 (via VIN), the DFPlayer, and the amplifier; the ESP32's onboard regulator supplies 3.3V to the OLED.

```
9V battery → [ON/OFF switch] → Buck converter (set to 5V!) → ESP32 VIN + DFPlayer + amp
                                                            → (ESP32 regulator) → 3.3V → OLED
```

**Power wiring:**
- 9V battery **+** → **ON/OFF switch** → Buck converter **IN+**
- 9V battery **−** → Buck converter **IN−**
- Buck **OUT+** → +5V rail (ESP32 VIN, DFPlayer VCC, amp +)
- Buck **OUT−** → GND rail (common ground)

The **ON/OFF switch** sits on the battery's + line, so flipping it off powers down the whole device.

⚠️ **Set the buck converter output to 5V with a multimeter BEFORE connecting any electronics.** The DFPlayer and ESP32 will be damaged by overvoltage.

ℹ️ The 9V battery is recharged with its own dedicated charger.

---

## 💾 SD Card Setup

Format the microSD card as **FAT32**, then organize music into **numbered folders** with **3-digit track files**:

```
SD card root/
├── 01/                 (Album 1)
│   ├── 001.mp3
│   ├── 002.mp3
│   └── ...
└── 02/                 (Album 2)
    ├── 001.mp3
    ├── 002.mp3
    └── ...
```

- Folders: two digits (`01`, `02`, …)
- Tracks: three digits, restarting in each folder (`001.mp3`, `002.mp3`, …)
- Copy files in order; the DFPlayer indexes by copy order.

Then update the song/album names and counts at the top of the sketch to match your music:

```cpp
const char* albumNames[]  = { "Laufey", "The Strokes" };
const int   trackCounts[] = { 8, 4 };
const char* laufeyTracks[]  = { "Fragile", "From The Start", /* ... */ };
const char* strokesTracks[] = { "Threat of Joy", "Is This It", /* ... */ };
```

---

## 📦 Dependencies

Install these libraries via the Arduino Library Manager:

- `DFRobotDFPlayerMini`
- `Adafruit GFX Library`
- `Adafruit SSD1306`

And the **ESP32 board package** (via Boards Manager — board: *DOIT ESP32 DEVKIT V1*).

---

## 🚀 Getting Started

1. Wire everything per the tables above.
2. Prepare the SD card (FAT32, numbered folders, 3-digit track files).
3. Install the dependencies and ESP32 board package.
4. Open the sketch, update the album/track names to match your card.
5. Select **DOIT ESP32 DEVKIT V1** and the correct COM port.
6. Upload, and enjoy. 🎧

---

## 🎮 Controls

| Action | Control |
|--------|---------|
| Move up / down a list | Press **Up** / **Down** |
| Open album / play track / play-pause | **Select** |
| Go back a level | **Back** |
| Skip to next track | Double-press **Up** |
| Previous track | Double-press **Down** |

---

## 🧠 Notes & Limitations

- The progress bar is an **approximation** based on estimated track lengths (the DFPlayer doesn't report exact playback position). Adjust the length arrays in the code for better accuracy.
- This project was originally prototyped on an Arduino Nano but migrated to the ESP32 — the ESP32's hardware UARTs and larger RAM eliminate the SoftwareSerial/I2C conflicts and memory constraints encountered on the Nano.

---

## 🙏 Acknowledgments

Hardware design, wiring, and debugging done hands-on. After the initial basic test sketches, the main menu/UI code was developed with the help of **Claude (Anthropic)** as a pair-programming and debugging assistant.

---

## 📋 License

Free to use and modify. Attribution appreciated.
