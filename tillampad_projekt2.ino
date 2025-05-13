#if 1

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#include <TouchScreen.h>
#include <Servo.h>

#define MINPRESSURE 200
#define MAXPRESSURE 1000

// Touchscreen pin configuration and calibration values
const int XP=8,XM=A2,YP=A3,YM=9;
const int TS_LEFT=918,TS_RT=201,TS_TOP=197,TS_BOT=920;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Button setup
Adafruit_GFX_Button buttons[15];
const char* labels[15] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#"};

// Buffers and state flags
char inputBuffer[16] = "";
char correctCode[16] = "";
bool isCodeSet = false;

Servo lockServo; // Servo for lock mechanism
const int servoPin = 41;
bool isServoActivated = false;

unsigned long starPressTime = 0;
bool starPressedRecently = false;
const unsigned long comboWindow = 500; // Time window for *# combo

int pixel_x, pixel_y; // Stores last touch coordinates

// Reads the touchscreen and maps the touch to screen coordinates
bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT); // Restore pin modes
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
        pixel_x = map(p.x, TS_LEFT, TS_RT, tft.width(), 0);
        pixel_y = map(p.y, TS_TOP, TS_BOT, tft.height(), 0);
    }
    return pressed;
}

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Initializes the screen, servo, and buttons
void setup(void)
{
    Serial.begin(9600);
    lockServo.attach(servoPin);
    lockServo.write(0); // Lock position

    uint16_t ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9486;
    tft.begin(ID);
    tft.setRotation(2);
    tft.fillScreen(BLACK);

    // Draws input box
    tft.fillRect(40, 40, 240, 50, WHITE);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.setCursor(50, 60);
    tft.print(inputBuffer);

    // Initializes and draws 12 keypad buttons
    int x_positions[3] = {60, 160, 260};
    int y_positions[4] = {200, 270, 340, 410};
    int index = 0;

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            if (index < 12) {
                buttons[index].initButton(&tft, x_positions[col], y_positions[row], 80, 60, WHITE, CYAN, BLACK, labels[index], 2);
                buttons[index].drawButton(false);
                index++;
            }
        }
    }
}

// Updates the input display box with current buffer content
void updateScreen(uint16_t textColor = BLACK) {
    tft.fillRect(40, 40, 240, 50, WHITE);
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    tft.setCursor(50, 60);
    tft.print(inputBuffer);
}

// Main loop: handles touchscreen input, code entry, and servo lock/unlock
void loop(void)
{
    bool down = Touch_getXY();
    for (int i = 0; i < 12; i++) {
        buttons[i].press(down && buttons[i].contains(pixel_x, pixel_y));
        if (buttons[i].justReleased())
            buttons[i].drawButton();
        if (buttons[i].justPressed()) {
            buttons[i].drawButton(true);

            if (strcmp(labels[i], "*") == 0) {
                // Handles clear and part of reset combo so the clear actually always works
                starPressTime = millis();
                starPressedRecently = true;
                inputBuffer[0] = '\0';
                updateScreen();
            } 
            else if (strcmp(labels[i], "#") == 0) {
                // Handles reset combo and code verification
                if (isServoActivated && starPressedRecently && (millis() - starPressTime <= comboWindow)) {
                    isCodeSet = false;
                    strcpy(correctCode, "");
                    inputBuffer[0] = '\0';
                    updateScreen(YELLOW);
                    delay(1000);
                    updateScreen();
                    starPressedRecently = false;
                } else {
                    if (!isCodeSet) {
                        strcpy(correctCode, inputBuffer);
                        isCodeSet = true;
                        updateScreen(GREEN);
                        delay(1000);
                        inputBuffer[0] = '\0';
                    } else {
                        if (strcmp(inputBuffer, correctCode) == 0) {
                            updateScreen(GREEN);
                            lockServo.write(180); // Unlocked
                            isServoActivated = true;
                        } else {
                            updateScreen(RED);
                            if (isServoActivated) {
                                lockServo.write(0); // Locked
                                isServoActivated = false;
                            }
                        }
                        delay(1000);
                        inputBuffer[0] = '\0';
                    }
                    updateScreen();
                    starPressedRecently = false;
                }
            } 
            else {
                // Appends pressed digit to input buffer
                if (strlen(inputBuffer) < 15) {
                    strcat(inputBuffer, labels[i]);
                    updateScreen();
                }
            }
        }
    }
}
#endif
