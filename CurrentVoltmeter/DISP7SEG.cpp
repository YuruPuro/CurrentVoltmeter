/*
 * SSD1306/SSD1315に７セグメントっぽいフォントで表示
 * 
 */
#include <Arduino.h>
#include <Wire.h>
#include "bitmapdata.h"
#include "DISP7SEG.h"

// SSD1306の初期化コマンド群
static uint8_t DISP7SEG_SSD1306_WIDTH = 128;  // OLEDの幅(ピクセル数)
static uint8_t DISP7SEG_SSD1306_HOGHT = 64;   // OLEDの高さ(ピクセル数)
static uint8_t DISP7SEG_SSD1306_PAGES = 8;    // OLEDのページ数(ピクセル数÷８)
static uint8_t DISP7SEG_SSD1306_ADDR = 0x3C;  // SSD1306のI2Cアドレス
static uint8_t DISP7SEG_SSD1306_CMD  = 0x80;  // SSD1306の1Byteコマンド
static uint8_t DISP7SEG_SSD1306_CMDS = 0x00;  // SSD1306の複数Byteコマンド
static uint8_t DISP7SEG_SSD1306_DATA = 0x40;  // SSD1306のデータバイト
const uint8_t DISP7SEG_SSD1306_INIT[] = {
  0xC8,         // C0/C8:上下の描画方向
  0xA1,         // A0/A1:左右の描画方向
  0x22,         // 20/21/22/23:アドレッシングモード
  0xA8,0x3F,    // 画面の解像度
  0xD3,0x00,    // 縦方向のオフセット
  0x40,         // 縦方向の描画開始位置
  0xA6,         // 表示方法 - A7にすると画面反転
  0x8D, 0x14    // OLEDパネルの供給電圧と思われる 14hに設定しないとダメらしい
};

// 初期化
void DISP7SEG::init() {
  // OLED設定コマンド送信
  oledSendCommands(sizeof(DISP7SEG_SSD1306_INIT), DISP7SEG_SSD1306_INIT) ;  // 初期化
  oledSendCommand(0xAF) ; // 画面表示
  cls() ;
}

void DISP7SEG::cls() {
  oledSendCommand(0xAE) ; // 画面表示
  oledFillData(0) ;
  oledSendCommand(0xAF) ; // 画面表示
}

void DISP7SEG::dispSwitch(bool sw) {
  if (sw) {
    oledSendCommand(0xAF) ; // 画面表示
  } else {
    oledSendCommand(0xAE) ; // 画面表示
  }
}

// OLEDに1バイトコマンドを送信
void DISP7SEG::oledSendCommand(uint8_t command) {
  Wire.beginTransmission(DISP7SEG_SSD1306_ADDR);
  Wire.write(DISP7SEG_SSD1306_CMD);
  Wire.write(command);
  Wire.endTransmission();
}

// OLEDに2バイトコマンドを送信
void DISP7SEG::oledSendCommand2(uint8_t command1,uint8_t command2) {
  Wire.beginTransmission(DISP7SEG_SSD1306_ADDR);
  Wire.write(DISP7SEG_SSD1306_CMDS);
  Wire.write(command1);
  Wire.write(command2);
  Wire.endTransmission();
}

// OLEDに3バイトコマンドを送信
void DISP7SEG::oledSendCommand3(uint8_t command1,uint8_t command2,uint8_t command3) {
  Wire.beginTransmission(DISP7SEG_SSD1306_ADDR);
  Wire.write(DISP7SEG_SSD1306_CMDS);
  Wire.write(command1);
  Wire.write(command2);
  Wire.write(command3);
  Wire.endTransmission();
}

// OLEDに複数のコマンドを送信::Wireクラスの制限により最大32Byte
// 長さのチェックはしてないのでプログラマ責任で上手い事やってちょ
void DISP7SEG::oledSendCommands(int commandLen,uint8_t commands[]) {
  Wire.beginTransmission(DISP7SEG_SSD1306_ADDR);
  Wire.write(DISP7SEG_SSD1306_CMDS);
  for (int i = 0; i < commandLen; i++) {
    Wire.write(commands[i]);
  }
   Wire.endTransmission();
}

// 画面を特定のビットバターンで埋める::画面の初期化用
void DISP7SEG::oledFillData(uint8_t fillData) {
  for (uint8_t m = 0; m < DISP7SEG_SSD1306_PAGES; m++) {
    oledSendCommand3(0xB0 | (m & 0x07) , 0x10 , 0x00) ;

    Wire.beginTransmission(DISP7SEG_SSD1306_ADDR);
    Wire.write(DISP7SEG_SSD1306_DATA);
    for (uint8_t x = 0; x < DISP7SEG_SSD1306_WIDTH; x++) {
      if (Wire.write(fillData) == 0) {
        Wire.endTransmission();
        Wire.beginTransmission(DISP7SEG_SSD1306_ADDR);
        Wire.write(DISP7SEG_SSD1306_DATA);
        Wire.write(fillData);
      }
    }
    Wire.endTransmission();
  }
  oledSendCommand3(0xB0 , 0x10 , 0x00) ;
}

// 7セグメント調フォント表示
void DISP7SEG::disp7SEG(int offsetX , int offsetY , int num) {
  int width = 16 ;
  int heigh = 4 ;

  if (num < 16) {
    if (num < 0) { num = 10 ; }
    bool flag7[7] ;
    for (int i=0;i<7;i++) {
      flag7[i] = addMap[num][i] ; 
    }
    for (int i = 0; i < 16*4; i++) {
        bitMapBuff[i] = 0 ;
      if (flag7[0]) { bitMapBuff[i] |= pgm_read_word(SEGa + i) ; }  // a
      if (flag7[1]) { bitMapBuff[i] |= pgm_read_word(SEGb + i) ; }  // b
      if (flag7[2]) { bitMapBuff[i] |= pgm_read_word(SEGc + i) ; }  // c
      if (flag7[3]) { bitMapBuff[i] |= pgm_read_word(SEGd + i) ; }  // d
      if (flag7[4]) { bitMapBuff[i] |= pgm_read_word(SEGe + i) ; }  // e
      if (flag7[5]) { bitMapBuff[i] |= pgm_read_word(SEGf + i) ; }  // f
      if (flag7[6]) { bitMapBuff[i] |= pgm_read_word(SEGg + i) ; }  // g
    }
  } else
  if (num > 100) { // CLEAR
    width = num % 100 ;
    heigh = num / 100 ;
    memset(bitMapBuff,0,width*heigh) ;
  } else {
    uint8_t *bitMap ;
    switch(num) {
      case 20: bitMap = SEGdot ; break ;    // DOT
      case 21: bitMap = SEGg ; break ;      // -
      case 22: bitMap = base7SEG ; break ;  // Body
      case 23: bitMap = SEGAMP ; break ;    // A
      case 24: bitMap = SEGVOL ; break ;    // V
      case 25: bitMap = SEGm ; break ;      // m
    }
    for (int i = 0; i < width*heigh; i++) {
      bitMapBuff[i] = pgm_read_word(bitMap + i) ; 
    } 
  }

  oledBitMap(offsetX,offsetY, width , heigh , bitMapBuff) ;
}

// BITパターン表示
void DISP7SEG::oledBitMap(int offsetX , int offsetY , int width,int hight, uint8_t *bitMap) {
  int pos = 0 ;
  for (int m = 0; m < hight; m++) {
    oledSendCommand3(0xB0 | ((m + offsetY) & 0x07) , 0x10 | (offsetX & 0xf0) >> 4 , offsetX & 0x0F) ;
    Wire.beginTransmission(DISP7SEG_SSD1306_ADDR);
    Wire.write(DISP7SEG_SSD1306_DATA);
    for (int x = 0; x < width; x++) {
      int8_t byteData = bitMap[pos] ;
      if (Wire.write(byteData) == 0) {
        Wire.endTransmission();
        Wire.beginTransmission(DISP7SEG_SSD1306_ADDR);
        Wire.write(DISP7SEG_SSD1306_DATA);
        Wire.write(byteData);
      }
      pos++ ;
    }
    Wire.endTransmission();
  }
  oledSendCommand3(0xB0 , 0x10 , 0x00) ;
}
