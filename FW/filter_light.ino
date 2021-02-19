

#include <Wire.h>
#include "RTClib.h"
#include <DS3231.h>
#include <avr/wdt.h>
#include <FastLED.h>



bool SET_RTC = false;
bool USE_GAMMA_RGB = true;
bool TEST_RGB = true;

byte testPWM = 5;

#define DEBUG


#define RGBLightNum 1
#define RGBDataPin 19
#define RGBClockPin 18                                                                                                                                                                                                                                                                                                                                                 

int gamma[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

long LightCurve[][4] = {
    // {target time, target red, target green, target blue} - first time must bee 0, last time must bee 86 399 (sec), color 0,0%-100,0% (0-1000)
    {0, 0, 0, 0},           //00:00
    {57600, 0, 0, 0},       //16:00
    {57900, 700, 0, 700},   //16:05
    {75600, 700, 0, 700},   //21:00
    {75900, 0, 0, 0},       //21:05 
    {86399, 0, 0, 0}     //23:59:59
};


int NumRows;
int IndexRow;
int CurrentRow;
int TargetRow;

int RedCurr = 0;       //0-100
int RedPrev = 0;       //0-100
int RedPwm = 0;        //0-255
int RedPwmMax = 255;   //0-255

int GreenCurr = 0;
int GreenPrev = 0;
int GreenPwm = 0;
int GreenPwmMax = 255;

int BlueCurr = 0;
int BluePrev = 0;
int BluePwm = 0;
int BluePwmMax = 255;



//declarate variables
// var for date
int TimeY = 0;
int TimeMo = 0;
int TimeDay = 0;
int TimeD = 0;
int TimeH = 0;
int TimeM = 0;
int TimeS = 0;

unsigned long RtcCurrentMillis = 0;
unsigned long TimeStamp = 0;
unsigned long LenghtDay = 86399;

unsigned long DEBUG_TimeStamp = 0;

// days
char DayOfTheWeek[7][8] = {"nedele", "pondeli", "utery", "streda", "ctvrtek", "patek", "sobota"};

DS3231 rtc;
RTCDateTime DateTime;

CRGB RBGLights[RGBLightNum];

void setup(){
    wdt_enable(WDTO_2S);
    // serial comunication via USB
    Serial.begin(115200);
    Serial.println("------Start setup-----");

    FastLED.addLeds<P9813, RGBDataPin, RGBClockPin, RGB>(RBGLights, RGBLightNum); // BGR ordering is typical
    for (int i = 0; i < RGBLightNum; i++){
        RBGLights[i] = CRGB(0, 0, 0);
    }
    FastLED.show();
    TestRGB();

    rtc.begin();
    SetRTC();

    I2CScanner();

    GetTime();
    Serial.println("Current timestamp is: " + String(TimeStamp));


    Serial.println("------End setup-----");

}

//****************************************** LOOP ****************************************

void loop (){
    wdt_reset();

    if (millis() >= (RtcCurrentMillis + 1000)){
        GetTime();
        RtcCurrentMillis = millis();
    }
    PrepareShowLight();

    SerialInfo();
}

//****************************************** FUNCTION ****************************************


void GetTime(){
    DateTime = rtc.getDateTime();
    int NTimeY = DateTime.year;
    byte NTimeMo = DateTime.month;
    byte NTimeDay = DateTime.day;
    byte NTimeH = DateTime.hour;
    byte NTimeM = DateTime.minute;
    byte NTimeS = DateTime.second;
    if (((NTimeMo > 0) && (NTimeMo < 13)) && ((NTimeH >= 0) && (NTimeH < 24)) && ((NTimeM >= 0) && (NTimeM < 60))){
        TimeY = NTimeY;
        TimeMo = NTimeMo;
        TimeDay = NTimeDay;
        TimeH = NTimeH;
        TimeM = NTimeM;
        TimeS = NTimeS;
        TimeStamp = (long(TimeH) * 3600) + (long(TimeM) * 60) + long(TimeS);
        #ifdef DEBUG
            Serial.println("Correct time read");
        #endif
    }
    else{
        #ifdef DEBUG
            Serial.println("err time read");
        #endif
    }
}


void SerialInfo(){
    #ifdef DEBUG
        if (DEBUG_TimeStamp != TimeStamp){
            Serial.println();
            Serial.println("------------------------------------------------------------");
            Serial.println("-------------------Start serial info------------------------");
            Serial.print("Actual date and time " + String(TimeDay) + '/' + String(TimeMo) + '/' + String(TimeY) + ' ' + String(TimeH) + ":" + String(TimeM) + ":" + String(TimeS));
            Serial.print("\nTime Stamp (sec): " + String(TimeStamp));
            Serial.print("\nindex current row: ");    Serial.print(CurrentRow);     Serial.print("\tindex target row: ");    Serial.print(TargetRow);
            Serial.print("\nRed: ");        Serial.print(map(RedPwm, 0, RedPwmMax, 0, 100));        Serial.print(" % \tPWM: ");     Serial.print(RedPwm);       Serial.print(" \tG PWM: ");  Serial.print(gamma[RedPwm]);
            Serial.print("\t\tGreen: ");    Serial.print(map(GreenPwm, 0, GreenPwmMax, 0, 100));    Serial.print(" % \tPWM: ");     Serial.print(GreenPwm);     Serial.print(" \tG PWM: ");  Serial.print(gamma[GreenPwm]);
            Serial.print("\t\tBlue: ");     Serial.print(map(BluePwm, 0, BluePwmMax, 0, 100));      Serial.print(" % \tPWM: ");     Serial.print(BluePwm);      Serial.print(" \tG PWM: ");  Serial.print(gamma[BluePwm]);
            DEBUG_TimeStamp = TimeStamp;
            Serial.println();
            Serial.println("-------------------End serial info--------------------------");
            Serial.println("------------------------------------------------------------\n");
        }
    #endif
}

void PrepareShowLight (){
    NumRows = sizeof(LightCurve)/sizeof(LightCurve[0]);
    //Serial.println(NumRows);
    IndexRow = NumRows - 1;
    for(int i = 0; i < IndexRow; i++){
        if(TimeStamp >= LightCurve[i][0]){
            CurrentRow = i;
            TargetRow = CurrentRow + 1;
            
        }
    }

    RedCurr = map(TimeStamp, LightCurve[CurrentRow][0], LightCurve[TargetRow][0], LightCurve[CurrentRow][1], LightCurve[TargetRow][1]);
    RedPwm = map(RedCurr, 0, 1000, 0, RedPwmMax);

    GreenCurr = map(TimeStamp, LightCurve[CurrentRow][0], LightCurve[TargetRow][0], LightCurve[CurrentRow][2], LightCurve[TargetRow][2]);
    GreenPwm = map(GreenCurr, 0, 1000, 0, GreenPwmMax);

    BlueCurr = map(TimeStamp, LightCurve[CurrentRow][0], LightCurve[TargetRow][0], LightCurve[CurrentRow][3], LightCurve[TargetRow][3]);
    BluePwm = map(BlueCurr, 0, 1000, 0, BluePwmMax);


    if((RedPrev != RedCurr) || (GreenPrev != GreenCurr) || (BluePrev != BlueCurr)){
        
        ShowLight();
        Serial.print("\n********** Color Changed **********");
        Serial.print("\nRed   from: ");     Serial.print(float(RedPrev)/10);       Serial.print(" % \tto: ");   Serial.print(float(RedCurr)/10);      Serial.print(" % \tPWM: ");  Serial.print(RedPwm);    Serial.print(" \tGammma PWM: ");  Serial.print(gamma[RedPwm]);    
        Serial.print("\nGreen from: ");     Serial.print(float(GreenPrev)/10);     Serial.print(" % \tto: ");   Serial.print(float(GreenCurr)/10);    Serial.print(" % \tPWM: ");  Serial.print(GreenPwm);  Serial.print(" \tGammma PWM: ");  Serial.print(gamma[GreenPwm]);
        Serial.print("\nBlue  from: ");     Serial.print(float(BluePrev)/10);      Serial.print(" % \tto: ");   Serial.print(float(BlueCurr)/10);     Serial.print(" % \tPWM: ");  Serial.print(BluePwm);   Serial.print(" \tGammma PWM: ");  Serial.print(gamma[BluePwm]);
        Serial.print("\n********** Color Changed **********");
        RedPrev = RedCurr;
        GreenPrev = GreenCurr;
        BluePrev = BlueCurr;
    }
}

void ShowLight(){
    for (int i = 0; i < RGBLightNum; i++) {
        if(USE_GAMMA_RGB){
            RBGLights[i] = CRGB(gamma[RedPwm], gamma[GreenPwm], gamma[BluePwm]);
        }
        else{
            RBGLights[i] = CRGB(RedPwm, GreenPwm, BluePwm);
        }
    }
    FastLED.show();

    Serial.print("\nWrite color\n");

}

void SetRTC(){
    if (SET_RTC){
        rtc.setDateTime(__DATE__, __TIME__);
        SET_RTC = false;
        Serial.println();
        Serial.println("---------- Time changed ----------");
        Serial.println(String(__DATE__) + " " + String(__TIME__));
        Serial.println("---------- Time changed ----------");
    }
}

void I2CScanner(){
    Serial.println("\nI2C Scanner");
    byte error, address;
    int nDevices;
    Serial.println("Scanning...");
    nDevices = 0;
    for (address = 1; address < 127; address++){
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0){
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println("  !");

            nDevices++;
        }
        else if (error == 4){
            Serial.print("Unknown error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("All device read!\n");
}

void TestRGB(){
    if(TEST_RGB){
        Serial.println("testig rgb");
        Serial.println("r");
        for (int i = 0; i < RGBLightNum; i++) {
            RBGLights[i] = CRGB(testPWM, 0, 0);
            FastLED.show();
            wdt_reset();
            delay(200);
        }
        for (int i = 0; i < RGBLightNum; i++) {
            RBGLights[i] = CRGB(0, 0, 0);
        }
        FastLED.show();
        Serial.println("g");
        for (int i = 0; i < RGBLightNum; i++) {
            RBGLights[i] = CRGB(0, testPWM, 0);
            FastLED.show();
            wdt_reset();
            delay(200);
        }
        for (int i = 0; i < RGBLightNum; i++) {
            RBGLights[i] = CRGB(0, 0, 0);
        }
        FastLED.show();
        Serial.println("b");
        for (int i = 0; i < RGBLightNum; i++) {
            RBGLights[i] = CRGB(0, 0, testPWM);
            FastLED.show();
            wdt_reset();
            delay(200);
        }
        for (int i = 0; i < RGBLightNum; i++) {
            RBGLights[i] = CRGB(0, 0, 0);
        }
        FastLED.show();
   }  
}