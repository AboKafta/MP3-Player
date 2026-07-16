# **Portable MP3 Player — Engineering Report**

**Author:** Georgy Rached

**Project:** A handheld music player built from a microcontroller, an MP3 decoder module, an OLED display, and physical buttons. The display features a two-level album/track menu, animated UI, and full playback controls.

---

## **Project Overview**

The goal of the project was to build a portable MP3 player that reads off mp3 files from an SD card. I also wanted to include controls that made it easy to navigate the screen. The project was supposed to run on an Arduino Nano, but then changed to an ESP32 because the OLED was taking a lot of memory. For the final model, I decided to move the build onto perfboards to make the design permanent

---

## **Final Hardware**

- **ESP32 DOIT DevKit V1** → microcontroller
- **DFPlayer Mini (MP3-TF-16P)** → MP3 decoder + SD card playback
- **SSD1306 128x64 OLED** → display (I2C)
- **MAX4410 headphone amplifier** → drives earphones, with volume knob. Comes with Audio Jack
- **4 push buttons** → navigation/control
- **9V rechargeable battery + buck converter** → powers the entire device (stepped down to 5V)
- **ON/OFF switch** → on the battery's positive line
- Supporting parts: decoupling capacitor to reduce static noise to earphones

---

## **Issues Encountered, Solutions, and Conclusion**

1. **Fried the DFPlayer Mini (9V on a 5V module):**
I connected a 9V battery without thinking and everything stopped working and the DFplayer module started to overheat a lot. Thankfully the Arduino wasn't harmed. I added a buck converter to step down the voltage and measured the voltage with a multimeter to double check.
2. **Audio playing but no sound/really bad quality:**
The module was saying it was playing but nothing was coming out. I realized that the DFPlayer Mini didn't have a headphone amp. I decided to buy a MAX4410 headphone amplifier that fixed the sound issues. Additionally, I added a decoupling capacitor close to the DFPlayer to further reduce noise.
3. **OLED and DFPlayer unable to work together:**
I was having troubles with getting both the OLED and DFPlayer to work together. One would start while the other wouldn't or one of them would crash. New code didn't work so I decided to switch microcontrollers and that worked perfectly.
4. **Music album files played in the wrong order or didn't even play:**
I had the files named after the album names. However, the DFPlayer requires specific naming conventions, so I had to change the folders to (01, 02). The code was then modified so 01 and 02 weren't the names showing on the OLED, but the actual album names.
5. **Input failure traced to a dead breadboard contact:**
One button never responded to the microcontroller even though all the other ones worked. I tested if power was getting through with the continuity mode with my multimeter and no power was getting through. I thought it was the microcontroller at first, but both failed. The common factor was the breadboard now, so I moved the button to a different row/GPIO pin and it started to work.

This project taught me a lot about the importance of incremental integration to code, which made errors easy to localize. Additionally, while building new code and changing layouts, I kept my basic test sketches to see if the code was faulty or my wiring. I also learned a lot more on how to use the multimeter when diagnosing problems. One thing I need to improve on is housing for my projects. I forgot a lot of my Solidworks skills and decided to use an Altoid Can which turned out pretty sloppy. Additionally, perfboard migration turned out super well, but there were still too many other parts that I couldn't put on there such as the 9V battery, Amp Module, and switch, which still made the build bulky.

## **Author Note:**

After all the initial components were tested with basic sketches, the more complex code was developed with Claude Opus 4.8 (the track menu, the animated UI, and tying the playback controls together with the display). Hardware choices, wiring and physical testing were still all done by me.

**Mini Guide:**

- Features include: two level menu (browse by album and then track)
- Animated boot screen
- Playing screen with features such as play, pause, skip, background animation and progress bar
- Double press feature to skip and go back to songs
- Auto-advance to the next song
- VCC → VIN (5V)
    
    ---
    

### **Wiring**

**DFPlayer Mini** (hardware Serial2)

- VCC → VIN (5V)
- GND → GND
- RX → GPIO17
- TX → GPIO16

Add a ~500µF capacitor between VCC and GND to reduce noise.

**OLED** (I2C)

- SDA → GPIO21
- SCL → GPIO22
- VCC → 3.3V
- GND → GND

**Buttons**

- Up → GPIO25
- Down → GPIO26
- Select → GPIO27
- Back → GPIO14

**Audio Output**

- DFPlayer DAC_L / DAC_R / GND → MAX4410 amp input → earphones (or panel jack)

### **Power**

The entire device runs from a 9V rechargeable battery stepped down to 5V by a buck converter. That 5V rail powers the ESP32 (via VIN), the DFPlayer, and the amplifier; the ESP32's onboard regulator supplies 3.3V to the OLED.

---

## **SD Card Setup**

Format the microSD card as FAT32. Make sure to name the song as 01, then the songs as 001.mp3

Then update the song/album names and counts at the top of the sketch to match your music:

const char* albumNames[]  = { "Laufey", "The Strokes" };

const int   trackCounts[] = { 8, 4 };

const char* laufeyTracks[]  = { "Fragile", "From The Start", /* ... */ };

const char* strokesTracks[] = { "Threat of Joy", "Is This It", /* ... */ };

---

## **Libraries:**

Install these libraries via the Arduino Library Manager:

- DFRobotDFPlayerMini
- Adafruit GFX Library
- Adafruit SSD1306

---

### **Controls**

- Move up / down a list → Press Up / Down
- Open album / play track / play-pause → Select
- Go back a level → Back
- Skip to next track → Double-press Up
- Previous track → Double-press Down

Protoype:
<img width="3024" height="4032" alt="IMG_0585" src="https://github.com/user-attachments/assets/449cb5e2-264e-4654-9600-cb06449a603d" />

Final:
<img width="3024" height="4032" alt="IMG_0646" src="https://github.com/user-attachments/assets/055c4c03-fd47-412a-9bfe-dec3a366617a" />
