
/* //////////////// DEFINE PINS ///////////// */
/* LIQUID CRYSTAL DISPLAY PINS */
#define CryD7 A3
#define CryD6 A2
#define CryD5 A1
#define CryD4 A0
#define CryEn 5
#define CryRW 4
#define CryRegSel 3
#define CryContrast A5

/* 4 ROWS, 4 COLS FOR INPUT SCAN/OUTPUT */
#define c1 6
#define c2 7
#define c3 8
#define c4 9
#define r1 10
#define r2 11
#define r3 12
#define r4 13

#include <LiquidCrystal.h>
LiquidCrystal lcd(CryRegSel, CryEn, CryD4, CryD5, CryD6, CryD7);

/* Current settings & limits
  [0] - (Not used)
  [1] - set unit price
  [2] - set pump speed
  [3] - set demand price
  [4] - set demand litter
*/
unsigned long settingMode[5] = {0, 21365, 20, 51056, 0};
unsigned long limitMode[2][5] = {{0, 10000, 10, 0, 0},{0, 60000, 1000, 7000000, 999}};

/* Mode indicators
  0 - working
  1,2,3,4 - currently setting for these index
*/
byte MODE = 1;
byte prevMODE = 0;

/* Minimum time between two input reading procedure, in millisecond(s) */
unsigned long debounceTime = 40;
bool block; // input reading enable

/* Value for LCD printing */
String line1[2];         // String line 1
String line2[2];         // String line 2
unsigned long nline2[2]; // Number line 2

/* Current state, Previous state, Row, Col */
int state[15][4];
unsigned long buttonDebounce[13];

float currentPrice[2] = {40000, 0};
float currentLitter[2] = {0, 0};

void setup() {
  // 4 COLs
  pinMode(c1, INPUT); // Switches 7, 4, 1, Set
  pinMode(c2, INPUT); // Switches 8, 5, 2, 0
  pinMode(c3, INPUT); // Switches 9, 6, 3, Del
  pinMode(c4, INPUT); // Switches Mode1, Led, Mode2, Pump

  // 4 ROWs
  pinMode(r1, INPUT);
  pinMode(r2, INPUT);
  pinMode(r3, INPUT);
  pinMode(r4, INPUT);

  state[0][2] = r4;             state[0][3] = c2;
  state[1][2] = r3;             state[1][3] = c1;
  state[2][2] = r3;             state[2][3] = c2;
  state[3][2] = r3;             state[3][3] = c3;
  state[4][2] = r2;             state[4][3] = c1;
  state[5][2] = r2;             state[5][3] = c2;
  state[6][2] = r2;             state[6][3] = c3;
  state[7][2] = r1;             state[7][3] = c1;
  state[8][2] = r1;             state[8][3] = c2;
  state[9][2] = r1;             state[9][3] = c3;
  state[10][2] = r4;/* Pump */  state[10][3] = c4;
  state[11][2] = r4;/* Del  */  state[11][3] = c3;
  state[12][2] = r4;/* Set  */  state[12][3] = c1;
  /* 13 - LED */
  /* 14-Switch 2 terminals r1,r3 - c4 */

  // LCD Setup
  pinMode(CryContrast, OUTPUT);
  pinMode(CryRW, OUTPUT);
  digitalWrite(CryRW, 0);
  analogWrite(CryContrast, 1000);
  lcd.begin(16, 2);
  LCDprint2("   DO AN TKLL   ", "GIA LAP CAY XANG");
  delay(2000);
  lcd.clear();

  // Initialization
  for (int i = 0; i < 15; i++) {
    state[i][0] = 1;
    state[i][1] = 1;
    buttonDebounce[i] = millis();
  }

  Serial.begin(9600);
}

void loop() {
  /* //////////////// READ INPUT ///////////// */
  // READ SWITCH
  pinMode(r1, OUTPUT); digitalWrite(r1, HIGH);
  pinMode(r3, OUTPUT); digitalWrite(r3, LOW);
  pinMode(c4, INPUT_PULLUP);
  state[14][0] = digitalRead(c4);
  pinMode(r1, INPUT);
  pinMode(r3, INPUT);
  pinMode(c4, INPUT);

  // SCAN BUTTONS
  if (millis() % debounceTime > debounceTime/2) {
    if (block == false) {
      for (int i = 0; i < 13; i++) {
        pinMode(state[i][2], OUTPUT);       // ROW
        digitalWrite(state[i][2], LOW);
        pinMode(state[i][3], INPUT_PULLUP); // COL
        state[i][0] = digitalRead(state[i][3]);
        pinMode(state[i][2], INPUT);      // ROW
        pinMode(state[i][3], INPUT);      // COL
      }
    }
    block = true;
  } else block = false;


  /* //////////////// PROCESSING ///////////// */
  // ** MODE
  if ((state[12][0] != state[12][1]) && (state[12][0] == 1)) {
    MODE++;
    if (MODE > 4)
      MODE = limitVerify();
  }
  switch (MODE) {
    case 1: {
        line1[0] = "DAT DON GIA:";
        nline2[0] = settingMode[MODE];
        break;
      }
    case 2: {
        line1[0] = "DAT TOC DO BOM:";
        nline2[0] = settingMode[MODE];
        break;
      }
    case 3: {
        line1[0] = "NHAP SO TIEN:";
        nline2[0] = settingMode[MODE];
        break;
      }
    case 4: {
        line1[0] = "NHAP SO LIT:";
        nline2[0] = settingMode[MODE];
        break;
      }
    default: break;
  }

  for (int i = 0; i <= 12; i++) {
    // ** BUTTON Act
    if ((state[i][0] != state[i][1]) && (state[i][1] == 1)) {
      // Number button pressed
      if ((i >= 0) && (i <= 9) && (MODE != 0)) {
        settingMode[MODE] = settingMode[MODE] * 10 + i;

        // Prevent user from input excess the limits
        if (settingMode[MODE] > limitMode[1][MODE]) {          
          settingMode[MODE] = (settingMode[MODE] - i) / 10;
        }
        
        // If both demand litter and demand price is set, only the demand price is used for pumping.
        if ((settingMode[3] != 0) && (settingMode[4] != 0)) { 
          settingMode[4] = 0;
        }
      }

      // DELETE button pressed
      if ((i == 11) && (MODE != 0)) {
        settingMode[MODE] = (settingMode[MODE] - (settingMode[MODE] % 10)) / 10; 
      }
    }
  }

  // ** Calculations
  if (MODE == 0) { // Working Mode
    // ** LED STATE
    if (state[14][0] != state[14][1]) { // Reset pumping value on Working Mode change
      currentPrice[0] = 0;
      currentLitter[0] = 0;
    }
    if (state[10][0] == 0) {            // When press the Pump button
      state[13][0] = 1;        // set the LED on
      if (state[14][0] == 0) { // Automatic Mode
        if ((settingMode[4] == 0) && (settingMode[3] != 0)) {
          // Automatic - By Price
          currentPrice[0] += settingMode[2] * 30 + 1;
          currentLitter[0] = long(currentPrice[0] * 10000 / float(settingMode[1]));
          if (currentPrice[0] > settingMode[3]) {
            currentPrice[0] = settingMode[3];
            currentLitter[0] = long((float(settingMode[3]) / settingMode[1]) * 10000);
            state[13][0] = 0;
          }
        }
        if ((settingMode[4] != 0) && (settingMode[3] == 0)) {
          // Automatic - By Litter
          currentLitter[0] += long((settingMode[2] * 30 + 1) * 10000 / float(settingMode[1]));
          currentPrice[0] = long((currentLitter[0] / 10000) * float(settingMode[1]));
          if (currentLitter[0] > settingMode[4] * 10000) {
            currentLitter[0] = settingMode[4] * 10000;
            currentPrice[0] = settingMode[4] * settingMode[1];
            state[13][0] = 0;
          }
        }
      } else { // Manual Mode
        currentPrice[0] += settingMode[2] * 30 + 1;
        currentLitter[0] = long(currentPrice[0] * 10000 / float(settingMode[1]));
      }
    } else {                    // When release the Pump button
      state[13][0] = 0;         // Set the LED off
      if (state[14][0] == 0) {
        line2[0] = "TU DONG NGAT";
      }
      if (state[14][0] == 1) {
        line2[0] = "NGAT BANG TAY";
      }
      if (prevMODE != 0) { // Reset pumping value if user switch from Setting Mode to Working Mode
        line1[0] = "CHE DO";
        currentPrice[0] = 0; 
        currentLitter[0] = 0;
      }
    }
  } else { // Setting Mode
    nline2[0] = settingMode[MODE];
    state[13][0] = 0;
  }

  // ** Print on LCD
  if ( (line1[1] != line1[0]) || (nline2[1] != nline2[0])
       || (line2[0] != line2[1]) || (currentPrice[0] != currentPrice[1])
       || (currentLitter[0] != currentLitter[1])) {
    if (MODE == 0) { // Working Mode
      if (state[10][0] == 0) {
        LCDpump(currentPrice[0], currentLitter[0]);
        delay(5);
      } else {
        LCDprint2(line1[0], line2[0]);
      }
    } else { // Setting Mode
      LCDprint(line1[0], nline2[0]);
    }
  }

  // ** Pumping indicator LED
  pinMode(r2, OUTPUT); digitalWrite(r2, LOW);
  pinMode(c4, OUTPUT);
  if (state[13][0] == 1) {
    digitalWrite(c4, HIGH); // TURN ON LED
    delay(2);
  } else {
    digitalWrite(c4, LOW); // TURN OFF LED
  }
  pinMode(c4, INPUT);
  pinMode(r2, INPUT);

  // ** Save previous values
  line1[1] = line1[0]; nline2[1] = nline2[0]; line2[1] = line2[0];
  currentPrice[1] = currentPrice[0];
  currentLitter[1] = currentLitter[0];

  prevMODE = MODE;
  for (int i = 0; i <= 14; i++) {
    if (i != 13)
      state[i][1] = state[i][0];
  }
}

/* //////////////// CUSTOM FUNCTION DEFINITION ///////////// */

// Return working mode if setting values are all valid, else return the invalid setting mode.
byte limitVerify() {          
  for (int i = 4; i>=0; i--) {
    if (i == 0) return 0;
    if (settingMode[i] < limitMode[0][i]) return i;
  }
}

// find X start position for printing number
byte findStartLCD(unsigned long value, unsigned long right) { 
  byte startb = 0;
  unsigned long x = value;
  while (x > 0) {
    x /= 10;
    startb++;
  }
  return (right - startb);
}

// Print string on line 1, number on line 2
void LCDprint(String a, unsigned long b) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print (a);
  lcd.setCursor(findStartLCD(b, 16), 1);
  lcd.print(b);
  return;
}

// Print string on 2 lines
void LCDprint2(String a, String b) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print (a);
  lcd.setCursor(16 - b.length(), 1);
  lcd.print(b);
  return;
}

// Screen when it's pumping
void LCDpump(unsigned long a, unsigned long b) { 
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("VND  000,000,000");
  lcd.setCursor(0, 1); lcd.print("LIT     000.0000");

  lcd.setCursor(findStartLCD(b % 10000, 16), 1);
  lcd.print(b % 10000);

  lcd.setCursor(findStartLCD(b / 10000, 11), 1);
  lcd.print(b / 10000);
  lcd.setCursor(11, 1);
  lcd.print(".");

  lcd.setCursor(findStartLCD(a % 1000, 16), 0);
  lcd.print(a % 1000);
  lcd.setCursor(findStartLCD((a / 1000) % 1000, 12), 0);
  lcd.print((a / 1000) % 1000);
  lcd.setCursor(findStartLCD((a / 1000000) % 1000, 8), 0);
  lcd.print((a / 1000000) % 1000);
  lcd.setCursor(12, 0);
  lcd.print(",");
  lcd.setCursor(8, 0);
  lcd.print(",");
  return;
}
