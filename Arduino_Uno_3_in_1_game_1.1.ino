#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

// ====================== PINS ======================
#define SCLK_PIN 13
#define MOSI_PIN 11
#define DC_PIN   9
#define CS_PIN   10
#define RST_PIN  8

#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F
#define YELLOW 0xFFE0
#define PINK  0xF81F
#define CYAN  0x07FF
#define ORANGE 0xFD20

Adafruit_SSD1351 tft = Adafruit_SSD1351(128, 128, &SPI, CS_PIN, DC_PIN, RST_PIN);

const int speakerPin = 6;
const int buttonPin = 2;

// ====================== GLOBAL ======================
int currentGame = 0;
int score = 0, lives = 3, level = 1;
int joyCenter = 512;
uint16_t palette[] = {GREEN, RED, 0xF81F, BLUE, YELLOW};

// ====================== SPACE INVADERS ======================
int shipX = 60;
bool gameOverFlag = false, bActive = false, ebActive = false, sActive = false;
int bX, bY, ebX, ebY, sX;
int enemyDir = 1;
unsigned long lastMarch = 0, nextSaucer = 5000;
bool lastAnim = false;

const int MAX_ENEMIES = 36;
struct Enemy { int x, y; int type; bool alive; };
Enemy enemies[MAX_ENEMIES];

struct Shield { int x, y, health; };
Shield shields[3] = {{20,95,5},{60,95,5},{100,95,5}};

const unsigned char squid1[] PROGMEM = {0x18,0x3C,0x7E,0xDB,0xFF,0x24,0x5A,0xA5};
const unsigned char squid2[] PROGMEM = {0x18,0x3C,0x7E,0xDB,0xFF,0x24,0x5A,0x42};
const unsigned char crab1[]  PROGMEM = {0x18,0x3C,0x7E,0xDB,0xFF,0xDB,0x42,0x24};
const unsigned char crab2[]  PROGMEM = {0x18,0x3C,0x7E,0xDB,0xFF,0xDB,0x24,0x42};
const unsigned char octo1[]  PROGMEM = {0x3C,0x7E,0xFF,0xDB,0xFF,0x3C,0x66,0xC3};
const unsigned char octo2[]  PROGMEM = {0x3C,0x7E,0xFF,0xDB,0xFF,0x3C,0x5A,0x99};
const unsigned char saucer[] PROGMEM = {0x07,0xE0,0x1F,0xF8,0x3F,0xFC,0x6D,0xB6,0xFF,0xFF,0x39,0x9C,0x0C,0x30};

const int marchTones[4] = {420, 380, 340, 300};
int marchStep = 0;
int marchInterval = 220;

// ====================== BREAKOUT ======================
int paddleX = 50;
int ballX = 64, ballY = 80;
int ballDX = 2, ballDY = -3;
bool bricks[6][8];
int bricksLeft = 48;

// ====================== MS PAC-MAN ======================
int pacX = 64, pacY = 100, pacDir = 0;
int ghostX[4] = {55,72,55,72};
int ghostY[4] = {45,45,55,55};
int ghostDir[4] = {0,2,0,2};
bool powerMode = false;
unsigned long powerEnd = 0;
int dotsEaten = 0;
bool dots[9][11];
bool walls[9][11];
bool powerPellets[4] = {true,true,true,true};
uint16_t ghostColors[4] = {RED, PINK, CYAN, ORANGE};

// ====================== SETUP ======================
void setup() {
  tft.begin();
  tft.fillScreen(BLACK);
  pinMode(buttonPin, INPUT_PULLUP);

  long avg = 0;
  for(int i = 0; i < 10; i++) {
    avg += analogRead(A0);
    delay(10);
  }
  joyCenter = avg / 10;

  currentGame = 0;
  resetCurrentGame();
}

// ====================== MAIN LOOP ======================
void loop() {
  if (currentGame == 0) { showMenu(); return; }

  if (currentGame == 1) spaceInvadersLoop();
  else if (currentGame == 2) breakoutLoop();
  else if (currentGame == 3) pacmanLoop();

  // Long hold (1 second) → Menu only
  static unsigned long btnHold = 0;
  if (digitalRead(buttonPin) == LOW) {
    if (btnHold == 0) btnHold = millis();
    if (millis() - btnHold > 1000) {
      currentGame = 0;
      tft.fillScreen(BLACK);
      btnHold = 0;
    }
  } else btnHold = 0;
}

// ====================== MENU ======================
void showMenu() {
  tft.fillScreen(BLACK);
  tft.setTextSize(2); tft.setTextColor(YELLOW);
  tft.setCursor(10, 15); tft.print("3 IN 1 GAME");

  tft.setTextSize(1); tft.setTextColor(WHITE);
  tft.setCursor(20, 55); tft.print("1. SPACE INVADERS");
  tft.setCursor(20, 70); tft.print("2. BREAKOUT");
  tft.setCursor(20, 85); tft.print("3. MS PAC-MAN");

  int choice = 1;
  int lastJoy = analogRead(A0);

  while (true) {
    int joy = analogRead(A0);
    if (joy < joyCenter - 60 && lastJoy >= joyCenter - 60) choice = max(1, choice-1);
    if (joy > joyCenter + 60 && lastJoy <= joyCenter + 60) choice = min(3, choice+1);
    lastJoy = joy;

    tft.fillRect(8, 52, 10, 55, BLACK);
    tft.setCursor(8, 52 + (choice-1)*15); tft.print(">");

    if (digitalRead(buttonPin) == LOW) {
      currentGame = choice;
      resetCurrentGame();
      delay(300);
      return;
    }
    delay(80);
  }
}

void resetCurrentGame() {
  score = 0; lives = 3; level = 1;
  tft.fillScreen(BLACK);

  if (currentGame == 1) resetInvaders();
  else if (currentGame == 2) resetBreakout();
  else if (currentGame == 3) resetPacman();
}

// ====================== SPACE INVADERS ======================
void resetInvaders() {
  for(int i = 0; i < MAX_ENEMIES; i++) {
    int col = i % 6; int row = i / 6;
    enemies[i] = {col * 18 + 15, row * 10 + 18, row % 3, true};
  }
  for(int s = 0; s < 3; s++) shields[s].health = 5;
  shipX = 60; enemyDir = 1;
  bActive = ebActive = sActive = false;
  lastMarch = millis(); nextSaucer = millis() + 7000;
  marchStep = 0; marchInterval = 220; gameOverFlag = false;
}

void spaceInvadersLoop() {
  if (gameOverFlag) {
    tft.fillScreen(BLACK);                    // Clean death screen
    tft.setCursor(30, 55); tft.setTextColor(RED);
    tft.print("GAME OVER");
    tft.setCursor(25, 75); tft.print("SCORE:"); tft.print(score);
    delay(50);
    return;                                   // No restart on button press
  }

  // Erase
  tft.fillRect(shipX, 118, 14, 10, BLACK);
  if (bActive) tft.drawFastVLine(bX, bY, 5, BLACK);
  if (ebActive) tft.drawFastVLine(ebX, ebY, 5, BLACK);
  if (sActive) tft.drawBitmap(sX, 15, saucer, 16, 7, BLACK);

  // Input - Fire only on short press
  int joyVal = analogRead(A0);
  if (joyVal < joyCenter - 40) shipX -= 4;
  else if (joyVal > joyCenter + 40) shipX += 4;
  shipX = constrain(shipX, 2, 112);

  if (digitalRead(buttonPin) == LOW && !bActive) {
    bActive = true; bX = shipX + 6; bY = 115;
    tone(speakerPin, 680, 18);
  }

  // Enemies, movement, sound, bullets, drawing (same good logic)
  bool drop = false; bool allDead = true; int aliveCount = 0;
  bool anim = (millis() / 280) % 2;
  int moveStep = 1 + (level / 3);

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].alive) {
      allDead = false; aliveCount++;
      const unsigned char* f1 = (enemies[i].type == 0 ? squid1 : (enemies[i].type == 1 ? crab1 : octo1));
      const unsigned char* f2 = (enemies[i].type == 0 ? squid2 : (enemies[i].type == 1 ? crab2 : octo2));
      tft.drawBitmap(enemies[i].x, enemies[i].y, lastAnim ? f2 : f1, 8, 8, BLACK);

      enemies[i].x += moveStep * enemyDir;
      if (enemies[i].x > 115 || enemies[i].x < 5) drop = true;
      if (enemies[i].y >= 88) {
        for (int s = 0; s < 3; s++) if (enemies[i].x >= shields[s].x - 8 && enemies[i].x <= shields[s].x + 15) shields[s].health = 0;
      }
      if (enemies[i].y >= 115) gameOverFlag = true;
    }
  }

  if (allDead) { level++; resetInvaders(); return; }
  if (drop) {
    enemyDir *= -1;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].y += 6;
    tone(speakerPin, 80, 25);
  }
  lastAnim = anim;

  marchInterval = 260 - (enemies[0].y * 1.8) - (36 - aliveCount) * 4;
  if (marchInterval < 60) marchInterval = 60;
  if (millis() - lastMarch > marchInterval) {
    tone(speakerPin, marchTones[marchStep], 32);
    marchStep = (marchStep + 1) % 4;
    lastMarch = millis();
  }

  if (!sActive && millis() > nextSaucer) { sActive = true; sX = -20; }
  if (sActive) {
    sX += 2;
    int warble = ((sX / 3) % 4 < 2) ? 120 : -120;
    tone(speakerPin, 680 + (sX * 1.4) + warble, 14);
    if (sX > 130) { sActive = false; nextSaucer = millis() + random(6500,13500); }
  }

  if (bActive) {
    bY -= 7; if (bY < 15) bActive = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (enemies[i].alive && bX >= enemies[i].x && bX <= enemies[i].x+10 && bY >= enemies[i].y && bY <= enemies[i].y+8) {
        enemies[i].alive = false; bActive = false; 
        score += (enemies[i].type==0?30:(enemies[i].type==1?20:10)); 
        tone(speakerPin,980,28); break;
      }
    }
    if (sActive && bX >= sX && bX <= sX+16 && bY <= 22) { 
      sActive=false; bActive=false; score+=200; tone(speakerPin,1200,110); 
    }
    for (int s=0; s<3; s++) if (shields[s].health>0 && bX>=shields[s].x && bX<=shields[s].x+15 && bY>=shields[s].y && bY<=shields[s].y+8) { 
      shields[s].health--; bActive=false; break; 
    }
  }

  if (!ebActive && random(13) > (9-level)) {
    int r = random(MAX_ENEMIES);
    if (enemies[r].alive) { ebActive=true; ebX=enemies[r].x+4; ebY=enemies[r].y+5; }
  }
  if (ebActive) {
    ebY += 5; if (ebY > 128) ebActive = false;
    if (ebX >= shipX && ebX <= shipX+12 && ebY >= 118) { 
      lives--; ebActive=false; tone(speakerPin,120,140); 
      if(lives<=0) gameOverFlag = true; 
    }
    for (int s=0; s<3; s++) if (shields[s].health>0 && ebX>=shields[s].x && ebX<=shields[s].x+15 && ebY>=shields[s].y && ebY<=shields[s].y+8) { 
      shields[s].health--; ebActive=false; break; 
    }
  }

  // Draw
  uint16_t alienColor = palette[level % 5];
  tft.fillRect(shipX, 122, 12, 5, GREEN);
  tft.fillRect(shipX+5, 118, 2, 4, GREEN);
  if (bActive) tft.drawFastVLine(bX, bY, 5, WHITE);
  if (ebActive) tft.drawFastVLine(ebX, ebY, 5, RED);
  if (sActive) tft.drawBitmap(sX, 15, saucer, 16, 7, YELLOW);

  for (int i = 0; i < MAX_ENEMIES; i++) if (enemies[i].alive) {
    const unsigned char* f1 = (enemies[i].type == 0 ? squid1 : (enemies[i].type == 1 ? crab1 : octo1));
    const unsigned char* f2 = (enemies[i].type == 0 ? squid2 : (enemies[i].type == 1 ? crab2 : octo2));
    tft.drawBitmap(enemies[i].x, enemies[i].y, anim ? f2 : f1, 8, 8, alienColor);
  }
  for (int s = 0; s < 3; s++) if (shields[s].health > 0) {
    tft.fillRect(shields[s].x, shields[s].y, 15, 8, (shields[s].health>2?GREEN:RED));
  }

  tft.fillRect(0,0,128,12,BLACK);
  tft.setTextSize(1); tft.setTextColor(WHITE);
  tft.setCursor(4,2); tft.print("SCORE "); tft.print(score);
  tft.setCursor(82,2); tft.print("LIVES "); tft.print(lives);

  delay(12);
}

// Breakout and Pac-Man remain the same as last version
// (Copy them from the previous message if needed)

void resetBreakout() {
  paddleX = 50; ballX=64; ballY=80; ballDX=2; ballDY=-3;
  bricksLeft = 48;
  for(int r=0;r<6;r++) for(int c=0;c<8;c++) bricks[r][c]=true;
}

void breakoutLoop() { /* your working breakout code */ 
  // ... paste your last working breakoutLoop here
}

void resetPacman() { /* your last pacman reset */ }
void pacmanLoop() { /* your last pacman loop */ }
