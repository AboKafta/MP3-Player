// Code by Georgy Rached
// ESP32 MP3 Player - boot anim, stars, marquee, icons, progress bar, transitions
// Wiring (ESP32 DOIT DevKit V1):
//   DFPlayer: VCC->VIN(5V), GND->GND, RX->GPIO17(via resistor), TX->GPIO16
//   OLED:     SDA->GPIO21, SCL->GPIO22, VCC->3.3V, GND->GND
//   Buttons:  GPIO25=Up  GPIO26=Down  GPIO27=Select  GPIO14=Back
//   (double-press UP = next track, double-press DOWN = previous track)

#include <DFRobotDFPlayerMini.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DFRobotDFPlayerMini player;

const int buttonPins[4] = { 25, 26, 27, 14 };
bool          btnLast[4] = { HIGH, HIGH, HIGH, HIGH };
unsigned long btnTime[4] = { 0, 0, 0, 0 };
const unsigned long DEBOUNCE = 50;

const unsigned long DBL_WINDOW = 400;
unsigned long lastTapTime[2] = { 0, 0 };
bool          pendingTap[2]  = { false, false };

const char* albumNames[] = { "Laufey", "The Strokes" };
const int   trackCounts[] = { 8, 4 };
const int   ALBUM_COUNT = 2;
const char* laufeyTracks[] = {
  "Fragile", "From The Start", "Wish You Love", "Dreamer",
  "Bored", "Haunted", "Letter to My 13", "Stranger"
};
const char* strokesTracks[] = {
  "Threat of Joy", "Is This It", "You Only Live Once", "Trying Your Luck"
};
const int laufeyLen[]  = { 200, 175, 190, 210, 165, 180, 150, 195 };
const int strokesLen[] = { 220, 145, 180, 215 };

const char* trackName(int a, int t) { return a == 0 ? laufeyTracks[t] : strokesTracks[t]; }
int trackLen(int a, int t) { return a == 0 ? laufeyLen[t] : strokesLen[t]; }

enum Screen { ALBUM_MENU, TRACK_MENU, NOW_PLAYING };
Screen screen = ALBUM_MENU;
void draw();

int  albumSel = 0, trackSel = 0, viewTop = 0;
int  playingAlbum = -1, playingTrack = -1;
bool isPlaying = false;
const int VISIBLE = 3;

#define NUM_STARS 14
struct Star { float x, y, speed; uint8_t size; };
Star stars[NUM_STARS];
unsigned long lastAnim = 0;
const int ANIM_MS = 40;

int marqueeX = 0;
unsigned long lastMarquee = 0;
unsigned long trackStart = 0;

void initStars() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(0, SCREEN_WIDTH);
    stars[i].y = random(0, SCREEN_HEIGHT);
    stars[i].speed = random(3, 12) / 10.0;
    stars[i].size = random(0, 3);
  }
}

void drawStar(int x, int y, uint8_t size) {
  if (size == 0) { display.drawPixel(x, y, SSD1306_WHITE); }
  else if (size == 1) {
    display.drawPixel(x, y, SSD1306_WHITE);
    display.drawPixel(x-1, y, SSD1306_WHITE); display.drawPixel(x+1, y, SSD1306_WHITE);
    display.drawPixel(x, y-1, SSD1306_WHITE); display.drawPixel(x, y+1, SSD1306_WHITE);
  } else {
    display.drawPixel(x, y, SSD1306_WHITE);
    display.drawPixel(x-2, y, SSD1306_WHITE); display.drawPixel(x+2, y, SSD1306_WHITE);
    display.drawPixel(x, y-2, SSD1306_WHITE); display.drawPixel(x, y+2, SSD1306_WHITE);
    display.drawPixel(x-1, y-1, SSD1306_WHITE); display.drawPixel(x+1, y+1, SSD1306_WHITE);
  }
}

void drawStarIcon(int x, int y, bool white) {
  uint16_t c = white ? SSD1306_WHITE : SSD1306_BLACK;
  display.drawPixel(x, y, c);
  display.drawPixel(x-2, y, c); display.drawPixel(x+2, y, c);
  display.drawPixel(x, y-2, c); display.drawPixel(x, y+2, c);
  display.drawPixel(x-1, y-1, c); display.drawPixel(x+1, y+1, c);
  display.drawPixel(x+1, y-1, c); display.drawPixel(x-1, y+1, c);
}

void drawPlayIcon(int x, int y) { display.fillTriangle(x, y-4, x, y+4, x+7, y, SSD1306_WHITE); }
void drawPauseIcon(int x, int y) {
  display.fillRect(x, y-4, 3, 9, SSD1306_WHITE);
  display.fillRect(x+5, y-4, 3, 9, SSD1306_WHITE);
}

// skip / back arrows. active = solid white; inactive = greyed (dotted outline)
void drawSkipIcon(int x, int y, bool active) {
  if (active) {
    display.fillTriangle(x, y-4, x, y+4, x+5, y, SSD1306_WHITE);
    display.fillTriangle(x+5, y-4, x+5, y+4, x+10, y, SSD1306_WHITE);
    display.fillRect(x+10, y-4, 2, 9, SSD1306_WHITE);
  } else {
    // greyed: just outlines
    display.drawTriangle(x, y-4, x, y+4, x+5, y, SSD1306_WHITE);
    display.drawTriangle(x+5, y-4, x+5, y+4, x+10, y, SSD1306_WHITE);
    display.drawRect(x+10, y-4, 2, 9, SSD1306_WHITE);
  }
}
void drawBackIcon(int x, int y, bool active) {
  if (active) {
    display.fillRect(x, y-4, 2, 9, SSD1306_WHITE);
    display.fillTriangle(x+2, y, x+7, y-4, x+7, y+4, SSD1306_WHITE);
    display.fillTriangle(x+7, y, x+12, y-4, x+12, y+4, SSD1306_WHITE);
  } else {
    display.drawRect(x, y-4, 2, 9, SSD1306_WHITE);
    display.drawTriangle(x+2, y, x+7, y-4, x+7, y+4, SSD1306_WHITE);
    display.drawTriangle(x+7, y, x+12, y-4, x+12, y+4, SSD1306_WHITE);
  }
}

bool pressed(int i) {
  bool reading = digitalRead(buttonPins[i]);
  if (reading != btnLast[i] && (millis() - btnTime[i]) > DEBOUNCE) {
    btnTime[i] = millis(); btnLast[i] = reading;
    if (reading == LOW) return true;
  }
  return false;
}

void adjustView() {
  if (trackSel < viewTop) viewTop = trackSel;
  if (trackSel >= viewTop + VISIBLE) viewTop = trackSel - VISIBLE + 1;
  if (viewTop < 0) viewTop = 0;
}

void playTrack(int a, int t) {
  player.playFolder(a + 1, t + 1);
  playingAlbum = a; playingTrack = t; isPlaying = true;
  marqueeX = SCREEN_WIDTH; trackStart = millis();
}

void nextTrack() {
  if (playingAlbum < 0) return;
  int n = playingTrack + 1;
  if (n >= trackCounts[playingAlbum]) n = 0;
  playTrack(playingAlbum, n);
  draw();
}
void prevTrack() {
  if (playingAlbum < 0) return;
  int n = playingTrack - 1;
  if (n < 0) n = trackCounts[playingAlbum] - 1;
  playTrack(playingAlbum, n);
  draw();
}

void bootAnimation() {
  for (int frame = 0; frame < 35; frame++) {
    display.clearDisplay();
    for (int i = 0; i < NUM_STARS; i++) {
      int sx = (stars[i].x + (64 - stars[i].x) * frame / 35);
      int sy = (stars[i].y + (32 - stars[i].y) * frame / 35);
      drawStar(sx, sy, stars[i].size);
    }
    display.display();
    delay(25);
  }
  for (int r = 0; r < 30; r += 3) {
    display.clearDisplay();
    display.drawCircle(64, 26, r, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(16, 16);
    display.print("MP3");
    display.setTextSize(1);
    display.setCursor(78, 22);
    display.print("PLAYER");
    display.display();
    delay(35);
  }
  for (int frame = 0; frame < 30; frame++) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(16, 14);
    display.print("MP3");
    display.setTextSize(1);
    display.setCursor(78, 20);
    display.print("PLAYER");
    drawStar(8, 12, 2); drawStar(120, 14, 2);
    drawStar(12, 40, 1); drawStar(116, 42, 1);
    if ((frame / 5) % 2 == 0) drawStar(64, 8, 2);
    if (frame > 10) {
      display.drawLine(20, 50, 108, 50, SSD1306_WHITE);
      display.setCursor(22, 54);
      display.print("by Georgy Rached");
    }
    display.display();
    delay(60);
  }
  delay(900);
}

void drawList(const char* title) {
  display.fillRoundRect(0, 0, SCREEN_WIDTH, 14, 2, SSD1306_WHITE);
  drawStarIcon(6, 6, false);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(14, 3);
  display.print(title);

  int count = (screen == ALBUM_MENU) ? ALBUM_COUNT : trackCounts[albumSel];
  int sel   = (screen == ALBUM_MENU) ? albumSel : trackSel;
  int top   = (screen == ALBUM_MENU) ? 0 : viewTop;

  for (int row = 0; row < VISIBLE; row++) {
    int i = top + row;
    if (i >= count) break;
    int y = 18 + row * 13;
    const char* name = (screen == ALBUM_MENU) ? albumNames[i] : trackName(albumSel, i);

    if (i == sel) {
      display.fillRoundRect(0, y - 1, SCREEN_WIDTH - 5, 12, 4, SSD1306_WHITE);
      display.fillTriangle(3, y + 2, 3, y + 8, 7, y + 5, SSD1306_BLACK);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(11, y + 1);
    } else {
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(11, y + 1);
    }

    if (screen == TRACK_MENU && albumSel == playingAlbum && i == playingTrack) {
      display.print(isPlaying ? "\x0E" : "=");
      display.print(" ");
    }
    display.print(name);
  }

  if (count > VISIBLE) {
    int barTop = 16, barH = 38;
    display.drawRect(SCREEN_WIDTH-3, barTop, 3, barH, SSD1306_WHITE);
    int thumbH = barH * VISIBLE / count;
    int thumbY = barTop + (long)(barH - thumbH) * top / (count - VISIBLE);
    display.fillRect(SCREEN_WIDTH-3, thumbY, 3, thumbH, SSD1306_WHITE);
  }

  display.drawLine(0, 55, SCREEN_WIDTH, 55, SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 57);
  if (screen == ALBUM_MENU) display.print("UP/DN  SEL:open");
  else                      display.print("UP/DN SEL:play BK");
}

void drawNowPlaying() {
  display.clearDisplay();
  for (int i = 0; i < NUM_STARS; i++) drawStar((int)stars[i].x, (int)stars[i].y, stars[i].size);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(albumNames[playingAlbum]);
  display.drawLine(0, 13, SCREEN_WIDTH, 13, SSD1306_WHITE);

  const char* nm = trackName(playingAlbum, playingTrack);
  int len = strlen(nm) * 6;
  if (len <= SCREEN_WIDTH - 4) {
    display.setCursor((SCREEN_WIDTH - len) / 2, 24);
    display.print(nm);
  } else {
    display.setCursor(marqueeX, 24);
    display.print(nm);
  }

  // --- transport row: back arrow | play/pause | skip arrow ---
  bool backActive = (playingTrack > 0);                              // greyed on first song
  bool skipActive = (playingTrack < trackCounts[playingAlbum] - 1);  // greyed on last song
  int rowY = 42;
  drawBackIcon(16, rowY, backActive);
  if (isPlaying) drawPlayIcon(58, rowY); else drawPauseIcon(56, rowY);
  drawSkipIcon(100, rowY, skipActive);

  // progress bar
  int elapsed = (millis() - trackStart) / 1000;
  int total = trackLen(playingAlbum, playingTrack);
  int pct = (total > 0) ? (elapsed * 100 / total) : 0;
  if (pct > 100) pct = 100;
  display.drawRoundRect(8, 56, SCREEN_WIDTH - 16, 6, 2, SSD1306_WHITE);
  int fillW = (SCREEN_WIDTH - 20) * pct / 100;
  display.fillRoundRect(10, 58, fillW, 2, 1, SSD1306_WHITE);

  display.display();
}

void draw() {
  if (screen == NOW_PLAYING) { drawNowPlaying(); return; }
  display.clearDisplay();
  if (screen == ALBUM_MENU) drawList("ALBUMS");
  else drawList(albumNames[albumSel]);
  display.display();
}

void slideTo(int target) {
  for (int x = 0; x <= SCREEN_WIDTH; x += 16) {
    display.fillRect(x, 0, 16, SCREEN_HEIGHT, SSD1306_BLACK);
    display.display();
  }
  screen = (Screen)target;
  draw();
}

void singleNav(int btn) {
  if (btn == 0) {
    if (screen == ALBUM_MENU) { albumSel--; if (albumSel < 0) albumSel = ALBUM_COUNT-1; draw(); }
    else if (screen == TRACK_MENU) { trackSel--; if (trackSel < 0) trackSel = trackCounts[albumSel]-1; adjustView(); draw(); }
  } else {
    if (screen == ALBUM_MENU) { albumSel++; if (albumSel >= ALBUM_COUNT) albumSel = 0; draw(); }
    else if (screen == TRACK_MENU) { trackSel++; if (trackSel >= trackCounts[albumSel]) trackSel = 0; adjustView(); draw(); }
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(34));
  for (int i = 0; i < 4; i++) pinMode(buttonPins[i], INPUT_PULLUP);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println("OLED failed"); for(;;); }

  initStars();
  bootAnimation();

  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  if (!player.begin(Serial2)) {
    display.clearDisplay(); display.setCursor(0,0); display.print("DFPlayer failed"); display.display();
    for(;;);
  }
  player.volume(22);
  draw();
}

void loop() {
  unsigned long now = millis();

  if (screen == NOW_PLAYING && now - lastAnim > ANIM_MS) {
    lastAnim = now;
    for (int i = 0; i < NUM_STARS; i++) {
      stars[i].x -= stars[i].speed;
      if (stars[i].x < 0) { stars[i].x = SCREEN_WIDTH; stars[i].y = random(0, SCREEN_HEIGHT); stars[i].size = random(0,3); }
    }
    const char* nm = trackName(playingAlbum, playingTrack);
    if ((int)strlen(nm) * 6 > SCREEN_WIDTH - 4 && now - lastMarquee > 60) {
      lastMarquee = now;
      marqueeX -= 2;
      if (marqueeX < -(int)strlen(nm) * 6) marqueeX = SCREEN_WIDTH;
    }
    drawNowPlaying();
  }

  if (isPlaying && player.available()) {
    if (player.readType() == DFPlayerPlayFinished) {
      int next = playingTrack + 1;
      if (next >= trackCounts[playingAlbum]) next = 0;
      playTrack(playingAlbum, next);
      draw();
    }
  }

  for (int b = 0; b < 2; b++) {
    if (pressed(b)) {
      if (pendingTap[b] && (now - lastTapTime[b]) < DBL_WINDOW) {
        pendingTap[b] = false;
        if (b == 0) nextTrack();
        else        prevTrack();
      } else {
        pendingTap[b] = true;
        lastTapTime[b] = now;
      }
    }
    if (pendingTap[b] && (now - lastTapTime[b]) >= DBL_WINDOW) {
      pendingTap[b] = false;
      singleNav(b);
    }
  }

  if (pressed(2)) {
    if (screen == ALBUM_MENU) { screen = TRACK_MENU; trackSel = 0; viewTop = 0; draw(); }
    else if (screen == TRACK_MENU) { playTrack(albumSel, trackSel); slideTo(NOW_PLAYING); }
    else { if (isPlaying) { player.pause(); isPlaying = false; } else { player.start(); isPlaying = true; } draw(); }
  }

  if (pressed(3)) {
    if (screen == TRACK_MENU) { screen = ALBUM_MENU; draw(); }
    else if (screen == NOW_PLAYING) { slideTo(TRACK_MENU); adjustView(); }
  }
}
