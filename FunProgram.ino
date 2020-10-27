#include "esp32-hal-ledc.h"
#include <Wire.h>

const int tranPin = 4;
const int tempPin = 34;
const int ledPin = 26;
const int switchPin = 16;

const int sdaPin = 23;
const int sclPin = 22;
const int I2Cadr = 0x3e;  // 固定
byte contrast = 35;  // コントラスト(0～63)
char powerBuf[8];
char tempBuf[8];
String power = "OFF";



const double V = 3.3; // ピン電圧

boolean funFlg = false;
boolean funFlgOld = false;

int swValOld = 0;
int swCnt = 0;

int boderTemp = 999;

int ledPower = 0;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  pinMode(tranPin, OUTPUT);
  pinMode(tempPin, INPUT);
  pinMode(switchPin, INPUT);

  ledcSetup(1, 490, 8);
  ledcAttachPin(ledPin, 1);

  // LCDディスプレイ i2c初期化
  delay(500);
  Wire.begin();
  lcd_cmd(0b00111000); // function set
  lcd_cmd(0b00111001); // function set
  lcd_cmd(0b00000100); // EntryModeSet
  lcd_cmd(0b00010100); // interval osc
  lcd_cmd(0b01110000 | (contrast & 0xF)); // contrast Low
  lcd_cmd(0b01011100 | ((contrast >> 4) & 0x3)); // contast High/icon/power
  lcd_cmd(0b01101100); // follower control
  delay(200);
  lcd_cmd(0b00111000); // function set
  lcd_cmd(0b00001100); // Display On
  lcd_cmd(0b00000001); // Clear Display
  delay(2);

  
}

void loop() {
  // put your main code here, to run repeatedly:

  // 温度センサーの値を取得
  double readVal = analogRead(tempPin);
  double tempC = (readVal/2048) * V * 100;

  // スイッチの状態を取得
  int swVal = digitalRead(switchPin);

  // 温度センサーの基準値変更
  if(swVal == 1 && swVal != swValOld){
    swCnt++;

    switch(swCnt){
      case 1: // 30度
        boderTemp = 30;
        ledPower = 10;
        power = String(boderTemp);     
        break;      
      case 2: // 28度
        boderTemp = 28;
        ledPower = 40;
        power = String(boderTemp);     
        break;
      case 3: // 25度
        boderTemp = 25;
        ledPower = 100;
        power = String(boderTemp);     
        break;
      case 4: // ON
        boderTemp = 0;
        ledPower = 254;
        power = "ON";     
        break;
      default: // OFF
        boderTemp = 999;
        ledPower = 0;
        swCnt = 0;
        power = "OFF";     
    }

    ledcWrite(1, ledPower);
  }

  // 温度が基準値を超えた場合、扇風機をONにする
  if(tempC >= boderTemp){
    funFlg = true;
  }else{
    funFlg = false;
  }

  if(funFlg != funFlgOld){
    if(funFlg){
      digitalWrite(tranPin, HIGH);    
    }else{
      digitalWrite(tranPin, LOW);          
    }
  }

  // ディスプレイ表示
  lcd_cmd(0b00000001); // Clear Display
  lcd_setCursor(0, 0);
  ("PWR:" + power).toCharArray(powerBuf,8);  
  lcd_printStr(powerBuf);
  lcd_setCursor(0, 1);
  ("TEMP:" + String((int) tempC)).toCharArray(tempBuf,8);
  lcd_printStr(tempBuf);
  delay(100);


  /*
  //デバック用
  Serial.println("温度:"+String(tempC)+"C");
  Serial.println("スイッチON OFF:"+String(swVal));
  Serial.println("扇風機ON OFF:"+String(funFlg));
  Serial.println("LED強さ:"+String(ledPower));
  delay(1000);
  */

  funFlgOld = funFlg;
  swValOld = swVal;

}


// i2cディスプレイ関連
void lcd_cmd(byte x) {
  Wire.beginTransmission(I2Cadr);
  Wire.write(0b00000000); // CO = 0,RS = 0
  Wire.write(x);
  Wire.endTransmission();
}

void lcd_contdata(byte x) {
  Wire.write(0b11000000); // CO = 1, RS = 1
  Wire.write(x);
}

void lcd_lastdata(byte x) {
  Wire.write(0b01000000); // CO = 0, RS = 1
  Wire.write(x);
}

// 文字の表示
void lcd_printStr(const char *s) {
  Wire.beginTransmission(I2Cadr);
  while (*s) {
    if (*(s + 1)) {
      lcd_contdata(*s);
    } else {
      lcd_lastdata(*s);
    }
    s++;
  }
  Wire.endTransmission();
}

// 表示位置の指定
void lcd_setCursor(byte x, byte y) {
  lcd_cmd(0x80 | (y * 0x40 + x));
}
