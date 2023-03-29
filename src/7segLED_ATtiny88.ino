/*
 Controlling large 7-segment displays
 This code demonstrates how to post two numbers to a 3-digit display usings two large digit driver boards.
 Here's how to hook up the Arduino pins to the Large Digit Driver IN
 Arduino pin
 6 -> CLK (Green on the 6-pin cable)
 5 -> LAT (Blue)
 7 -> SER on the IN side (Yellow)
 5V -> 5V (Orange)
 Power Arduino with 12V and connect to Vin -> 12V (Red)
 GND -> GND (Black)

 There are two connectors on the Large Digit Driver. 'IN' is the input side that should be connected to
 your microcontroller (the Arduino). 'OUT' is the output side that should be connected to the 'IN' of addtional
 digits.

 Each display will use about 150mA with all segments and decimal point on.

 Digit Driver IC is TPIC6C596 Power Logic 8-Bit Shift Register
 
*/

#include <EEPROM.h>
#include <ButtonEvents.h>

//GPIO declarations
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
byte segmentClock = 4;
byte segmentLatch = 5;
byte segmentData = 3;
const int up_pin    = 6; //カウントアップボタン
const int down_pin  = 7; //カウントダウンボタン
const int reset_pin = 1; //リセットボタン
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

ButtonEvents up_button; // create an instance of the ButtonEvents class to attach to our button
ButtonEvents down_button;
ButtonEvents reset_button;

int number   = 0;
int segnumber= 0;
int button_pt = 0;
char sp = ' ';

void setup()
{
  Serial.begin(9600);
  Serial.println(EEPROM.length());
  Serial.println("Large Digit Counter ver.1.0");

  pinMode(segmentClock, OUTPUT);
  pinMode(segmentData, OUTPUT);
  pinMode(segmentLatch, OUTPUT);
  pinMode(up_pin, INPUT_PULLUP);
  pinMode(down_pin, INPUT_PULLUP);
  pinMode(reset_pin, INPUT_PULLUP);

  digitalWrite(segmentClock, LOW);
  digitalWrite(segmentData, LOW);
  digitalWrite(segmentLatch, LOW);
  EEPROM.get(0x00,number);

  // attach our ButtonEvents instance to the button pin
  up_button.attach(up_pin);
  down_button.attach(down_pin);
  reset_button.attach(reset_pin);

  //ButtonParameter Setup
  //ダブルタップ制限時間(ms)
  up_button.doubleTapTime(200);
  down_button.doubleTapTime(200);
  reset_button.doubleTapTime(200);
  //長押し判定時間(ms)
  up_button.holdTime(5000);
  down_button.holdTime(5000);
  reset_button.holdTime(5000);
}

void loop()
{
  //アップボタン(シングルタップ +1, ダブルタップ +10)
  up_button.update();
  if(up_button.tapped() == true) 
  {
    EEPROM.put(0xf0,number); //カウンター数字バックアップ
    number++;
    number %= 1000; //Reset x after 999
    EEPROM.put(0x00,number);
  }
  if(up_button.doubleTapped() == true)
  {
    EEPROM.put(0xf0,number); //カウンター数字バックアップ
    number = number + 10;
    number %= 1000; //Reset x after 999
    EEPROM.put(0x00,number);
  }

  //ダウンボタン(シングルタップ -1, ダブルタップ -10)
  down_button.update();
  if(down_button.tapped() == true)
  {
    EEPROM.put(0xf0,number); //カウンター数字バックアップ
    number--;
    if(number < 0) 
    {
      number = number + 1000;
    }
    number %= 1000; //Reset x after 999
    EEPROM.put(0x00,number);
  }
  if(down_button.doubleTapped() == true)
  {
    EEPROM.put(0xf0,number); //カウンター数字バックアップ
    number = number - 10;
    if(number < 0) 
    {
      number = number + 1000;
    }
    number %= 1000; //Reset x after 999
  }

// リセットボタン(長押し リセット, ダブルタップ リストア)
  reset_button.update();
  if(reset_button.held() == true) //リセットボタン長押しでリセット
  {
    EEPROM.put(0xf0,number); //カウンター数字バックアップ
    number = 0;
    EEPROM.put(0x00,number);
  }
  if(reset_button.doubleTapped() == true) //リセットボタンダブルタップでリストア
  {
    EEPROM.get(0xf0,number); //カウンター数字リストア
    EEPROM.put(0x00,number);
  }

//  Serial.println(number);
  showNumber(number);
}

//Takes a number and displays 3 numbers. Displays absolute value (no negatives)
void showNumber(float value)
{
  segnumber = number ;
  int segnumber = abs(value); //Remove negative signs and any decimals
  Serial.print("number: ");
  Serial.println(segnumber);
  for (byte x = 0 ; x < 3 ; x++) //桁数決定
  {
    if ((x > 0) && (segnumber == 0)) //下2桁目以上が0の場合スペース
    {
      postNumber(sp, false);
    }
    else
    {
      int remainder = segnumber % 10; //各桁の数字を抽出
      postNumber(remainder, false); //1桁ずつ送信
      segnumber /= 10;
    }
  }

  //Latch the current segment data
  digitalWrite(segmentLatch, LOW);
  digitalWrite(segmentLatch, HIGH); //Register moves storage register on the rising edge of RCK
}

//Given a number, or '-', shifts it out to the display
void postNumber(byte segnumber, boolean decimal)
{
  //    -  A
  //   / / F/B
  //    -  G
  //   / / E/C
  //    -. D/DP

#define a  1<<0
#define b  1<<6
#define c  1<<5
#define d  1<<4
#define e  1<<3
#define f  1<<1
#define g  1<<2
#define dp 1<<7

  byte segments;

  switch (segnumber)
  {
    case 1: segments = b | c; break;
    case 2: segments = a | b | d | e | g; break;
    case 3: segments = a | b | c | d | g; break;
    case 4: segments = f | g | b | c; break;
    case 5: segments = a | f | g | c | d; break;
    case 6: segments = a | f | g | e | c | d; break;
    case 7: segments = a | b | c; break;
    case 8: segments = a | b | c | d | e | f | g; break;
    case 9: segments = a | b | c | d | f | g; break;
    case 0: segments = a | b | c | d | e | f; break;
    case ' ': segments = 0; break;
    case 'c': segments = g | e | d; break;
    case '-': segments = g; break;
  }

  if (decimal) segments |= dp;

  //Clock these bits out to the drivers
  for (byte x = 0 ; x < 8 ; x++)
  {
    digitalWrite(segmentClock, LOW);
    digitalWrite(segmentData, segments & 1 << (7 - x));
    digitalWrite(segmentClock, HIGH); //Data transfers to the register on the rising edge of SRCK
  }
}