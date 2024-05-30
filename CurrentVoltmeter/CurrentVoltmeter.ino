/**
 * 電流電圧計　For ATTiny85 / ArduinoUNO + INA219　/ SSD1306(128x64)
 * ※ピン番号を変更すれば ArduinoIDE 対応のマイコンなら大体動きます
 * @YoruPuro
 */

#include <Wire.h>
#include "DISP7SEG.h"
DISP7SEG disp ;

#define INA219_ADDR 0x40

// 計測レンジ切り替えスイッチ
#define SW_PIN PB3      // ArduinoUNO:8 / ATTiny85:PB3
// 計測レンジ表示ＬＥＤ
#define LED_PIN PB4     // ArduinoUNO:9 / ATTiny85:PB4

// -------------- 
int rangeMode = 1 ; // 0:32V/320mA 1:16V/400mA
bool rangeModeFlag = false ;  // 計測レンジ切り替えスイッチのチャタリング防止用

double preDispValV = 0.0 ; // 書き換え抑止用
double preDispValA = 0.0 ; // 書き換え抑止用
#define measuModeA 0  // 電流計
#define measuModeV 1  // 電圧計

static uint8_t conf[] = {
  0x399F , // 32V / 320mA / 12Bit 
  0x019F , // 16V / 40mA / 12Bit  ※ディフォルト
 } ;

// シャント抵抗1Ωの場合のキャリブレーション値
static uint16_t calibratin[] = {
  4194 ,  // 32V / 320mA
  13422 , // 16V / 40mA
} ;

const double rangeV = 0.001 ; // 電圧分解能
const double rangeA = 0.1 ;   // 電流分解能

/*
 * 計測値表示
 * 
 * mode: measuModeV-電圧値表示　measuModeA-電流値表示
 */
void dispConv(int mode) {
                    // 01234567 01234567
  int  dispSeg[9] ; // xxx.xxxV ±xxx.xmA

  for (int i=0;i<3;i++) {
    dispSeg[i] = 416 ;  // 空白で埋める
  }
  for (int i=3;i<8;i++) {
    dispSeg[i] = 22 ;  // ８
  }
  dispSeg[4] = 20 ; // DOT

  if (mode == measuModeV) {
    // --- 電圧表示
    dispSeg[7] = 24 ; // V
  } else {
    // --- 電流表示
    dispSeg[6] = 25 ; // m
    dispSeg[7] = 23 ; // A
  }
  // --- 表示
  int x = 0 ;
  for (int i=0;i<8;i++) {
    disp.disp7SEG(x,(mode==measuModeV)?0:4,dispSeg[i]) ;
    x += (dispSeg[i] == 20) ? 8 : 16 ; // DOTだけ幅を狭める
  } 
}

void dispConv(double dispVal,int mode) {
                    // 01234567 01234567
  int  dispSeg[9] ; // xxx.xxxV ±xxx.xmA

  if (mode == measuModeV) {
    // --- 電圧表示
    for (int i=0;i<8;i++) {
      dispSeg[i] = 416 ;  // 空白で埋める
    }
    dispSeg[7] = 24 ; // V

    int range1 = dispVal ;
    int range2 = dispVal * 1000.0 ;

    int pos1 = 2 ;
    int pos2 = 4 ;
    int pos2l = 3 ;
    dispSeg[3] = 20 ; // DOT
    for (int i=pos1;i>=0;i--) {
      dispSeg[i] = range1 % 10 ;
      range1 /= 10 ;
      if (range1 == 0) break ;
    }
    for (int i=pos2l-1;i>=0;i--) {
      dispSeg[pos2+i] = range2 % 10 ;
      range2 /= 10 ;
    }
  } else
  if (mode == measuModeA) {
    // --- 電流表示
    for (int i=0;i<8;i++) {
      dispSeg[i] = 416 ;  // 空白で埋める
    }
    dispSeg[6] = 25 ; // m
    dispSeg[7] = 23 ; // A

    int range1 = dispVal ;
    int range2 = dispVal * 10.0 ;

    int pos1 = 3 ;
    int pos1e = 0 ;
    int pos2 = 5 ;
    int pos2l =1 ;

    if (dispVal < 0) {
      range1 = -1 * range1 ;
      range2 = -1 * range2 ;
    } else {
      pos1e = 1 ;
      dispSeg[0] = 21 ; // -
    }

    dispSeg[4] = 20 ; // DOT
    for (int i=pos1;i>=pos1e;i--) {
      dispSeg[i] = range1 % 10 ;
      range1 /= 10 ;
      if (range1 == 0) break ;
    }

    for (int i=pos2l-1;i>=0;i--) {
      dispSeg[pos2+i] = range2 % 10 ;
      range2 /= 10 ;
    }
  } else {
    // --- 初期画面 8888.888 表示
    for (int i=0;i<8;i++) {
      dispSeg[i] = 22 ;
    }
    dispSeg[4] = 20 ; // DOT
    disp.cls() ;
  }

  // --- 表示
  int x = 0 ;
  for (int i=0;i<8;i++) {
    disp.disp7SEG(x,(mode==measuModeV)?0:4,dispSeg[i]) ;
    x += (dispSeg[i] == 20) ? 8 : 16 ; // DOTだけ幅を狭める
  } 
}

/*
 * INA219 キャリブレーション値設定
 * 
 * mode : 0:32V/320mA 1:16V/400mA
 */
void initINA219(int mode) {
    // - 計測モード、測定レンジが切り替わった場合にCONFIG値を再設定する
    Wire.beginTransmission(INA219_ADDR);
    Wire.write(0x00); // config アドレス
    Wire.write((conf[mode] >> 8) & 0xFF);
    Wire.write(conf[mode] & 0xFF);
    Wire.endTransmission();  

    // - キャリブレーション値設定
    Wire.beginTransmission(INA219_ADDR);
    Wire.write(0x05); // config アドレス
    Wire.write((calibratin[mode] >> 8) & 0xFF);
    Wire.write(calibratin[mode] & 0xFF);
    Wire.endTransmission();  

    digitalWrite(LED_PIN,((mode==1)?LOW:HIGH)) ;
    delay(1000) ;
}

// ------------------------------
void setup() {
  pinMode(SW_PIN,INPUT_PULLUP) ;
  pinMode(LED_PIN,OUTPUT) ;
  digitalWrite(LED_PIN,LOW) ;

  Wire.begin( ) ;
  disp.init() ;

  dispConv(measuModeV) ;
  dispConv(measuModeA) ;
  initINA219(rangeMode) ;
}

void loop() {
  double dispValV = 0.0 ;
  double dispValA = 0.0 ;

  if (digitalRead(SW_PIN) == LOW && !rangeModeFlag) {
    // - 測定レンジ切り替え
    rangeModeFlag = true ;
    rangeMode = (rangeMode + 1) % 2 ;

    // - 計測モード、測定レンジが切り替わった場合にCONFIG値を再設定する
    initINA219(rangeMode) ;
  } else {
    rangeModeFlag = false ;
  }

  // ----- 電圧 -----
  Wire.beginTransmission(INA219_ADDR);
  Wire.write(0x02);                    //BusVoltageレジスタ
  Wire.endTransmission();
  Wire.requestFrom(INA219_ADDR, 2);
  while (Wire.available() < 2);
  int16_t voltI = Wire.read() << 8 | Wire.read();
  voltI = (voltI >> 3) * 4;
  dispValV = voltI * rangeV ;

  // ----- 電流 -----
  Wire.beginTransmission(INA219_ADDR);
  Wire.write(0x01);
  Wire.endTransmission();
  Wire.requestFrom(INA219_ADDR, 16);
  while (Wire.available() < 2);
  int16_t shuntVoltI = Wire.read() << 8 | Wire.read();
  dispValA = shuntVoltI * rangeA ;

  // チラツキ防止：表示に変更がない場合は表示処理を行わない
  if (preDispValV != dispValV) {
    preDispValV = dispValV ;
    if (0.9 > dispValV && dispValV > -0.9) {
      dispConv(measuModeV) ;
    } else {
      dispConv(dispValV,measuModeV) ;
    }
  }

  // チラツキ防止：表示に変更がない場合は表示処理を行わない
  if (preDispValA != dispValA) {
    preDispValA = dispValA ;
    if (0.5 > dispValA && dispValA > -0.5) {
      dispConv(measuModeA) ;
    } else {
      dispConv(dispValA,measuModeA) ;
    }
  }

  delay(200) ;
}
