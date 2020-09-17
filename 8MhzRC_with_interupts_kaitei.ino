#include <LcdCore.h>
#include <LCD_ACM1602NI.h>
#include <Wire.h>
#include <SD.h>
#include <EEPROM.h>
#include <DS3232RTC.h>
#include <TimeLib.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

LCD_ACM1602NI lcd(0xa0);

//charset register
byte charData_3[8] = {
    B00000,
    B11100,
    B10100,
    B10100,
    B10100,
    B11100,
    B00000,
};
byte charData_4[8] = {
    B00000,
    B00001,
    B00001,
    B00001,
    B00001,
    B00001,
    B00000,
    B00000,
};
byte charData_5[8] = {
    B00000,
    B00111,
    B00001,
    B00111,
    B00100,
    B00111,
    B00000,
    B00000,
};
byte charData_6[8] = {
    B00000,
    B00111,
    B00001,
    B00111,
    B00001,
    B00111,
    B00000,
    B00000,
};
byte charData_b1[8] = {
    B10000,
    B10000,
    B10000,
    B10000,
    B10000,
    B10000,
    B10000,
    B10000,
};
byte charData_b2[8] = {
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
};
byte charData_b3[8] = {
    B11100,
    B11100,
    B11100,
    B11100,
    B11100,
    B11100,
    B11100,
    B11100,
};
byte charData_b4[8] = {
    B11110,
    B11110,
    B11110,
    B11110,
    B11110,
    B11110,
    B11110,
    B11110,
};

//Hardware pin define
#define BT1 2
#define BT2 4
#define BT3 3
#define sense 8
#define Vbat 3
#define Temp 2
#define backlight 0
#define FETSW 15

//weight
#define weight 48;

//senser status
//char nowhole = 0;
//char lasthole;
//char detect;
//cycle time
//long count, lastcycle = 0;
//long cycle = 1000;

//in interrupt
volatile unsigned int count100 = 0;
unsigned long cycle = 0;
volatile unsigned char lcdflag = 1;
volatile unsigned char calculateflag = 0;
unsigned char sreg;
volatile unsigned long count100ms = 0;
volatile unsigned char buttonflag = 0;

//SD
char sdtrue = 0;
char filename[9];

//timer
volatile long timercount1 = 0, timercount2 = 0;
char timerstatus1, timerstatus2 = 0; //0がリセット状態、1がカウント中、2が一時停止状態
char clockh1, clockm1, clocks1;
char clockh2, clockm2, clocks2;

//lastbutton
unsigned char lastbutton1 = 1, lastbutton2 = 1, lastbutton3 = 1;
//push down time
unsigned int pushdown1, pushdown2, pushdown3;

//long 18 =72byte
//int 16 =32byte
//char 42 =42byte
//byte 56 =56byte
//total 200byte

ISR(TIMER1_COMPA_vect) //100ms
{
  count100++;   // 100ms count for cycle time
  count100ms++; //100ms count (Never reset)
  if (count100ms % 5 == 0)
    lcdflag = 1;
  if (timerstatus1 == 1)
    timercount1 += 100;
  if (timerstatus2 == 1)
    timercount2 += 100;
}

ISR(PCINT0_vect) //senser
{
  calculateflag = 1;
}

ISR(PCINT2_vect) //input button
{
  buttonflag = 1;
}

int returntemp(void)
{
  int a;
  a = (2.547 / 1024.0) * (analogRead(Temp) - 240.22) / 0.001;
  return a;
}

void dateTime(uint16_t *date, uint16_t *time)
{
  tmElements_t tm;
  uint16_t year = 2000;
  uint8_t month = 1, day = 1, hour = 0, minute = 0, second = 0;
  RTC.read(tm);
  year = tm.Year + 1970;
  month = tm.Month;
  day = tm.Day;
  hour = tm.Hour;
  minute = tm.Minute;
  second = tm.Second;
  *date = FAT_DATE(year, month, day);
  *time = FAT_TIME(hour, minute, second);
}

void setup()
{
  sei();
  //Aref setting
  analogReference(EXTERNAL);

  //Port init
  pinMode(BT1, INPUT_PULLUP);
  pinMode(BT2, INPUT_PULLUP);
  pinMode(BT3, INPUT_PULLUP);
  pinMode(sense, INPUT);
  pinMode(FETSW, OUTPUT);
  pinMode(backlight, OUTPUT);
  digitalWrite(FETSW, LOW);

  //odo force
  /*
  unsigned long odo=27619500;
  EEPROM.put(10,odo);
  while(true){
    ;
  }*/

  //Serian transfer init
  Serial.begin(115200);

  //lcd init
  lcd.begin(16, 2);

  //timer1 start
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= 0b00001100;
  OCR1A = 3125 - 1;
  TIMSK1 |= (1 << OCIE1A);
  sei();

  //power on
  lcd.print(F("Press any button"));
  while (true)
  {
    if (digitalRead(BT1) == 0 || digitalRead(BT2) == 0 || digitalRead(BT3) == 0)
    {
      digitalWrite(FETSW, HIGH);
      lcd.clear();
      break;
    }
  }

  //Splash screen
  lcd.print(F("Bicycle"));
  lcd.setCursor(0, 1);
  lcd.print(F("Multi mater"));
  tone(1, 950, 175);
  delay(175);
  tone(1, 783, 500);
  delay(500);
  tone(1, 320, 175);
  delay(175);
  tone(1, 471, 175);
  delay(175);
  tone(1, 641, 500);
  delay(1000);

  //SD check
  if (!SD.begin(10))
  {
    lcd.clear();
    lcd.print(F("SD unavailable!"));
    sdtrue = 0;
    delay(1500);
  }
  else
  {
    delay(500);
    lcd.clear();
    lcd.print(F("SD available."));
    sdtrue = 1;
    //Timestamp
    SdFile::dateTimeCallback(&dateTime);
  }
  //file create
  if (sdtrue == 1)
  {
    int filenum = 1;
    while (1)
    {
      sprintf(filename, "%d.csv", filenum);
      if (SD.exists(filename) == 1)
        filenum++;
      else
        break;
    }
    lcd.setCursor(0, 1);
    lcd.print(F("File is "));
    lcd.print(filename);
    File logfile = SD.open(filename, FILE_WRITE);
    logfile.println(F("Time,Speed,Distance,Ave.spd,Temp"));
    logfile.close();
    delay(2000);
  }

  //charset register
  lcd.createChar(3, charData_3);
  lcd.createChar(4, charData_4);
  lcd.createChar(5, charData_5);
  lcd.createChar(6, charData_6);
  lcd.createChar(8, charData_b1);
  lcd.createChar(9, charData_b2);
  lcd.createChar(2, charData_b3);
  lcd.createChar(1, charData_b4);

  //sense interrupt
  PCICR |= B00000001;
  PCMSK0 |= B00000001;
  //button interrupt
  PCICR |= B00000100;
  PCMSK2 |= B00011100;
  //sleep setting
  set_sleep_mode(SLEEP_MODE_IDLE);
}

void loop()
{

  //temp
  int temp = returntemp();
  //display change
  int dispMode = 0;
  //SD
  char writeerror = 0;
  //button
  char nowbutton1, nowbutton2, nowbutton3;
  char buttonstatus1, buttonstatus2, buttonstatus3;
  //odo
  unsigned long odo = 0;
  //trip
  unsigned long trip = 0;
  //EEPROM
  EEPROM.get(10, odo);
  //pushtime
  int pushdown1, pushup1, pushdown2, pushup2, pushdown3, pushup3;
  //multiplex
  unsigned int multiplex = 10;
  //Battery voltage
  int vbat;
  //Burn cal
  int cal;
  int mets;
  //Bar visual disp
  int bar;
  int bar5;
  float bars;
  //Distance(cm)
  long dis = -111;
  //Max Speed
  int kmhmax = 0;
  //tani select
  char kmenable = 1;
  //odo enable
  char odoen = 1;
  //Average Speed
  int ave = 0;
  //Speed
  int sp = 0;
  //stop detect
  unsigned long lastdetect = 0;
  //runtime(ms)
  unsigned long runtime = 0;
  int runh = 0, runm = 0, runs = 0;
  //progress disp
  unsigned int zeroprogress = 0;
  unsigned int targetprogress = 100;
  //lcd
  char disp[16];
  tmElements_t tm;

  //timer0 clock deny
  //PRR = 0b00100000;

  while (true)
  {
    if (calculateflag == 0 && lcdflag == 0 && buttonflag == 0)
    {
      sleep_mode();
    }

    if (calculateflag)
    {
      calculateflag = 0;
      unsigned char sreg;
      unsigned int timer1;
      //lcdflag = 0;
      cycle = (unsigned long)count100 * 100;
      count100 = 0;
      sreg = SREG;
      cli();
      timer1 = TCNT1;
      TCNT1 = 0b00000000;
      SREG = sreg;
      sei();
      cycle += 0.032 * timer1;

      lastdetect = millis();
      dis += 111;
      if (odoen)
        odo += 111;
      trip += 111;
      sp = 3600.0 / cycle * 11.1;
      if ((sp / 10) > kmhmax)
        kmhmax = (sp / 10);
      if (sp >= 30)
        runtime += cycle;
      runh = runtime / 3600000L;
      runm = (runtime % 3600000L) / 60000L;
      runs = (runtime / 1000) - (runm * 60) - (runh * 3600);
      if (runtime != 0)
        ave = ((float)dis / 100000.0) / ((float)runtime / 3600000.0) * 10;
    }

    if (buttonflag)
    {
      buttonflag = 0;
      //Button read
      nowbutton1 = digitalRead(BT1);
      nowbutton2 = digitalRead(BT2);
      nowbutton3 = digitalRead(BT3);
      if (nowbutton1 != lastbutton1)
      {
        lastbutton1 = nowbutton1;
        if (nowbutton1 == 0)
        {
          pushdown1 = millis();
          //Serial.println("BT1 pos edge");
        }
        else
        {
          //Serial.println("BT1 neg edge");
          //pushup1 = ()millis();
          if ((65535U + (unsigned int)millis() - pushdown1) > 500)
          {
            buttonstatus1 = 2;
            // Serial.println("BT1 long push");
          }
          else if ((65535U + (unsigned int)millis() - pushdown1) > 20)
          {
            buttonstatus1 = 1;
            //Serial.println("BT1 short push");
          }
        }
      }
      if (nowbutton2 != lastbutton2)
      {

        lastbutton2 = nowbutton2;
        if (nowbutton2 == 0)
        {
          pushdown2 = millis();
          //Serial.println("BT2 pos edge");
        }
        else
        {
          //Serial.println("BT2 neg edge");
          //pushup2 = millis();
          if ((65535U + (unsigned int)millis() - pushdown2) > 500)
          {
            buttonstatus2 = 2;
            //Serial.println("BT2 long push");
          }
          else if ((65535U + (unsigned int)millis() - pushdown2) > 20)
          {
            buttonstatus2 = 1;
            //Serial.println("BT2 short push");
          }
        }
      }
      if (nowbutton3 != lastbutton3)
      {

        lastbutton3 = nowbutton3;
        if (nowbutton3 == 0)
        {
          pushdown3 = millis();
          //Serial.println("BT3 pos edge");
        }
        else
        {
          //Serial.println("BT3 neg edge");
          //pushup3 = millis();
          if ((65535U + (unsigned int)millis() - pushdown3) > 500)
          {
            buttonstatus3 = 2;
            //Serial.println("BT3 long push");
          }
          else if ((65535U + (unsigned int)millis() - pushdown3) > 20)
          {
            buttonstatus3 = 1;
            //Serial.println("BT3 short push");
          }
        }
      }

      if (buttonstatus1 == 1) //BT1 shortpush
      {
        //Serial.println(F("BT1 shortpush"));
        buttonstatus1 = 0;
        if (dispMode <= 6 || (dispMode >= 15 && dispMode <= 23))
          dispMode++;
        else if (dispMode == 7 || dispMode == 27 || dispMode == 24)
        {
          //Serial.println(F("dispMode set to 0"));
          dispMode = 0;
        }
        else if (dispMode == 10)
        {
          if (timerstatus1 == 0)
            timerstatus1 = 1;
          else if (timerstatus1 == 1)
            timerstatus1 = 2;
          else if (timerstatus1 == 2)
            timerstatus1 = 1;
        }
        else if (dispMode == 26)
        {
          EEPROM.put(10, odo);
          //各種値を保存するの書いといて
          lcd.clear();
          lcd.print(F("    See you."));
          delay(800);
          lcd.clear();
          digitalWrite(FETSW, LOW);
        }
        lcdflag = 1;
      }

      if (buttonstatus1 == 2) //BT1 longpush
      {
        //Serial.println(F("BT1 longpush"));
        buttonstatus1 = 0;
        if (dispMode <= 5)
          dispMode = 15;
        if (dispMode == 10 && timerstatus1 == 2)
        {
          timerstatus1 = 0;
          timercount1 = 0;
        }
        lcdflag = 1;
      }

      if (buttonstatus2 == 1) //BT2 shortpush
      {
        buttonstatus2 = 0;
        if (dispMode <= 7)
          dispMode = 11;
        else if (dispMode == 10)
        {
          if (timerstatus2 == 0)
            timerstatus2 = 1;
          else if (timerstatus2 == 1)
            timerstatus2 = 2;
          else if (timerstatus2 == 2)
            timerstatus2 = 1;
        }
        else if (dispMode == 11)
          dispMode = 0;
        else if (dispMode == 17)
        {
          if (multiplex < 60)
            multiplex++;
        }
        else if (dispMode == 15)
          if (kmenable)
            kmenable = 0;
          else
            kmenable = 1;
        else if (dispMode == 16)
          if (odoen)
            odoen = 0;
          else
            odoen = 1;
        else if (dispMode == 19)
          zeroprogress++;
        else if (dispMode == 20)
          targetprogress++;
        else if (dispMode == 23)
          PORTD = PORTD ^ 0x01;
        else if (dispMode == 26)
        {
          EEPROM.put(10, odo);
          lcd.clear();
          lcd.print(F("    See you."));
          delay(800);
          lcd.clear();
          digitalWrite(FETSW, LOW);
        }
        lcdflag = 1;
      }

      if (buttonstatus2 == 2) //BT2 longpush
      {
        buttonstatus2 = 0;
        if (dispMode == 10 && timerstatus2 == 2)
        {
          timerstatus2 = 0;
          timercount2 = 0;
        }
        else if (dispMode == 18)
          trip = 0;
        else if (dispMode == 19)
          zeroprogress += 10;
        else if (dispMode == 20)
          targetprogress += 10;
        lcdflag = 1;
      }

      if (buttonstatus3 == 1) //BT3 shortpush
      {
        //Serial.println(F("BT3 shortpush"));
        buttonstatus3 = 0;
        if (dispMode <= 7)
          dispMode = 10;
        else if (dispMode == 10)
          dispMode = 0;
        else if (dispMode == 19)
        {
          if (zeroprogress > 0)
            zeroprogress--;
        }
        else if (dispMode == 20)
        {
          if (targetprogress > 0)
            targetprogress--;
        }
        else if (dispMode == 21)
        {
          if (multiplex > 10)
            multiplex--;
        }
        else if (dispMode == 22)
        {
          dispMode = 27;
          EEPROM.put(10, odo);
        }
        else if (dispMode == 24)
          dispMode = 26;
      }
      lcdflag = 1;

      if (buttonstatus3 == 2)
      {
        buttonstatus3 = 0;
        if (dispMode == 19)
        {
          if (zeroprogress > 10)
            zeroprogress -= 10;
        }

        else if (dispMode == 20)
        {
          if (targetprogress > 10)
            targetprogress -= 10;
        }
      }
    }

    if ((millis() - lastdetect) >= 1500)
      sp = 0;

    if (millis() % 32768 == 0)
      temp = returntemp();
    if (lcdflag)
    {
      lcdflag = 0;
      Serial.println(dispMode);
      Serial.print(F("zeroprogress="));
      Serial.println(zeroprogress);
      Serial.print(F("targetprogress="));
      Serial.println(targetprogress);
      
      //SD save
      if (sdtrue)
      {
        File logfile = SD.open(filename, FILE_WRITE);
        if (logfile /*&& digitalRead(DIP3)*/)
        {
          writeerror = 0;
          RTC.read(tm);
          sprintf(disp, "%d:%02d:%02d", tm.Hour, tm.Minute, tm.Second);
          logfile.print(disp);
          logfile.print(",");
          logfile.print((float)sp / 10.0);
          logfile.print(",");
          logfile.print(dis / 100);
          logfile.print(",");
          logfile.print((float)ave / 10);
          logfile.print(",");
          logfile.println((float)temp / 10.0);
          logfile.close();
        }
        else
          writeerror = 1;
      }

      switch (dispMode)
      {
      case 0:
        sprintf(disp, "Speed:%4d.%dkm/h", (sp * (multiplex / 10)) / 10, (sp * (multiplex / 10)) % 10);
        lcd.clear();
        lcd.print(disp);
        lcd.setCursor(0, 1);
        //if (digitalRead(DIP2))
        sprintf(disp, "Dis:%7ld.%02ldkm", (dis * multiplex / 10) / 100000, (dis * multiplex / 10) / 1000);
        //else
        //  sprintf(disp, "Dis:%10dm", ((dis / 100) * multiplex / 10));
        lcd.print(disp);
        break;

      case 1:
        sprintf(disp, "Avg.Spd:%d.%dkm/h", ave / 10, ave % 10);
        lcd.clear();
        lcd.print(disp);
        lcd.setCursor(0, 1);
        //Serial.println(F("Timer"));
        //Serial.println(runh);
        //Serial.println(runm);
        //Serial.println(runs);
        sprintf(disp, "RunTime:%d:%02d:%02d", runh, runm, runs);
        lcd.print(disp);
        break;

      case 2:
        sprintf(disp, "Temp:%d.%d", temp / 10, temp % 10);
        lcd.clear();
        lcd.print(disp);
        lcd.write(0xDF);
        lcd.print("C");
        lcd.setCursor(0, 1);
        if (ave <= 160)
          mets = 40;
        else if (ave <= 192)
          mets = 60;
        else if (ave <= 224)
          mets = 80;
        else if (ave <= 256)
          mets = 100;
        else if (ave <= 306)
          mets = 120;
        else
          mets = 158;
        cal = 48 * mets / 10.0 * ((float)runtime / 3600000.0) * 1.05;
        sprintf(disp, "Burn:%dkcal", cal);
        lcd.print(disp);
        break;

      case 3:
        lcd.clear();
        lcd.print(F("Odo:"));
        lcd.print((float)odo / 100000.0);
        lcd.print(F("km"));
        lcd.setCursor(0, 1);
        lcd.print(F("MaxSpd:"));
        lcd.print(kmhmax);
        lcd.print(F("km/h"));
        break;

      case 4:
        lcd.clear();
        lcd.print(F("Trip:"));
        lcd.print((float)trip / 100000.0);
        lcd.print(F("km"));
        break;

      case 5:
        lcd.clear();
        lcd.write(3);
        lcd.setCursor(3, 0);
        lcd.write(4);
        lcd.write(3);
        lcd.setCursor(7, 0);
        lcd.write(5);
        lcd.write(3);
        lcd.setCursor(11, 0);
        lcd.write(6);
        lcd.write(3);
        lcd.setCursor(14, 0);
        lcd.print((int)(sp / 10));
        lcd.setCursor(0, 1);
        bar5 = sp / 25;
        for (bar = bar5; bar > 0; bar--)
          lcd.write(0xFF);
        bars = sp % 25;
        if (bars >= 20)
          lcd.write(1);
        else if (bars >= 15)
          lcd.write(2);
        else if (bars >= 10)
          lcd.write(9);
        else if (bars >= 5)
          lcd.write(8);
        break;

      case 6:

        lcd.clear();
        lcd.print(F("Prog:"));
        lcd.print(((dis / 10000) - zeroprogress) / 10);
        lcd.print(F("."));
        lcd.print(((dis / 10000) - zeroprogress) % 10);
        lcd.print(F("/"));
        lcd.print(targetprogress / 10);
        lcd.print(F("."));
        lcd.print(targetprogress % 10);
        lcd.print(F("k"));
        lcd.setCursor(0, 1);
        bar5 = 80 * ((((float)dis / (float)10000) - zeroprogress) / (targetprogress - zeroprogress));
        //         これがkmを10倍したやつ 500mなら5
        Serial.print(F("prog:"));
        Serial.println(bar5);
        if (bar5 <= 80)
        {
          for (bar = bar5; bar >= 5; bar-=5)
            lcd.write(0xFF);
          bars = bar5 % 5;
          Serial.print(F("Bars:"));
          Serial.println(bars);
          if ((int)bars >= 4)
            lcd.write(1);
          else if ((int)bars >= 3)
            lcd.write(2);
          else if ((int)bars >=2)
            lcd.write(9);
          else if ((int)bars >=1)
            lcd.write(8);
        }
        else
          lcd.print(F("Prog:Complete"));
        break;

      case 7:
        lcd.clear();
        lcd.print(F("VBat:"));
        vbat = (2.5 / 1024 * analogRead(Vbat) * 300 * 1.021);
        lcd.print((float)vbat / 100);
        lcd.print(F("V"));
        lcd.setCursor(0, 1);

        break;

      case 10:
        clockh1 = timercount1 / 3600000;
        clockm1 = (timercount1 % 3600000) / 60000;
        clocks1 = (timercount1 - ((int)clockm1 * 60000) - ((int)clockh1 * 3600000)) / 1000;
        clockh2 = timercount2 / 3600000;
        clockm2 = (timercount2 % 3600000) / 60000;
        clocks2 = (timercount2 - ((int)clockm2 * 60000) - ((int)clockh2 * 3600000)) / 1000;
        lcd.clear();
        sprintf(disp, "CH1:%dh%02dm%02d", clockh1, clockm1, clocks1);
        lcd.print(disp);
        lcd.setCursor(0, 1);
        sprintf(disp, "CH2:%dh%02dm%02d", clockh2, clockm2, clocks2);
        lcd.print(disp);
        break;
      case 11:
        lcd.clear();
        RTC.read(tm);
        sprintf(disp, "DATE:%dY%dM%dD", tm.Year + 1970, tm.Month, tm.Day);
        lcd.print(disp);
        lcd.setCursor(0, 1);
        sprintf(disp, "TIME:%d:%02d:%02d", tm.Hour, tm.Minute, tm.Second);
        lcd.print(disp);
        break;

      case 15:
        lcd.clear();
        lcd.print(F("m or km :"));
        if (kmenable)
          lcd.print(F("km"));
        else
          lcd.print(F("m"));
        break;

      case 16:
        lcd.clear();
        lcd.print(F("Odo enable:"));
        if (odoen)
          lcd.print(F("TRUE"));
        else
          lcd.print(F("FALSE"));
        break;

      case 17:
        lcd.clear();
        lcd.print(F("SD save:"));
        lcd.print(F("TRUE"));
        lcd.setCursor(0, 1);
        if (writeerror == 1) //SD error disp
          lcd.print(F("SD Error!"));
        break;

      case 18:
        lcd.clear();
        lcd.print(F("Trip reset?"));
        lcd.setCursor(0, 1);
        lcd.print(F("BT2 long push..."));
        break;

      case 19:
        lcd.clear();
        lcd.print(F("Prog begin set"));
        lcd.setCursor(0, 1);
        sprintf(disp, "%d.%dkm", zeroprogress / 10, zeroprogress % 10);
        lcd.print(disp);
        break;

      case 20:
        lcd.clear();
        lcd.print(F("Target dist set"));
        lcd.setCursor(0, 1);
        sprintf(disp, "%d.%dkm", (targetprogress - zeroprogress) / 10, (targetprogress - zeroprogress) % 10);
        lcd.print(disp);
        break;

      case 21:
        lcd.clear();
        lcd.print(F("Multiplex:"));
        lcd.print(multiplex);
        break;

      case 22:
        lcd.clear();
        lcd.print(F("EEPROM refresh"));
        lcd.setCursor(0, 1);
        lcd.print(F("Press Button 3 !"));
        break;

      case 23:
        lcd.clear();
        lcd.print(F("Backlight"));
        lcd.setCursor(0, 1);
        if (PORTD & 0x01)
          lcd.print(F("Enable"));
        else
          lcd.print(F("Disable"));
        break;

      case 24: //Power off menu
        lcd.clear();
        lcd.print(F("Power OfF Menu"));
        lcd.setCursor(0, 1);
        lcd.print(F("Press BT3"));
        break;
      case 26: //save check
        lcd.clear();
        lcd.print(F("Save value ?"));
        lcd.setCursor(0, 1);
        lcd.print(F("Y=1 N=2 C=3"));
        break;

      case 27: //EEPROM successs
        lcd.clear();
        lcd.print(F("Save successed!"));
        lcd.setCursor(0, 1);
        lcd.print(F("Press Button 1 !"));
        break;
      }
    }
  }
}
