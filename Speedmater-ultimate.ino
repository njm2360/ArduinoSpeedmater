#include <MsTimer2.h>


#include <LcdCore.h>
#include <LCD_ACM1602NI.h>
#include <Wire.h>
#include <SD.h>
#include <EEPROM.h>
LCD_ACM1602NI lcd(0xa0);

//外字登録
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

//センサ状態
int hole = 0;
int rhole;
int detect;
//気温(23.4度なら234)
int temp;
int tempsei;
int tempsyo;
//SD検出
int sdtrue = 0;
//SDファイル名
char filename[9];
int writeerror;
//サイクル
long count, rcycle = 0;
long cycle = 1000;
//オドメーター(cm)
long odo;

void detecter() //センサ割り込み
{
    hole = digitalRead(2); //センサ読み取り
    if (hole != rhole)
    {
        rhole = hole;
        cycle = count;
        count = 0;
        detect = 1;
    }
    else
        count++;
}

void setup() //Setup syntax
{
    odo=11719500;
    EEPROM.put(10,odo);
    EEPROM.get(10, odo);
    //Serial.begin(9600);
    analogReference(EXTERNAL);
    //Input settings
    pinMode(2, INPUT);
    pinMode(3, INPUT);
    pinMode(6, INPUT);
    pinMode(17, INPUT);
    lcd.begin(16, 2);
    //Splash screen
    lcd.print("Bicycle");
    lcd.setCursor(0, 1);
    lcd.print("Multi mater");
    delay(3000);
    lcd.clear();
    lcd.print("SD checking...");
    //SDチェック
    if (SD.begin(10) == false)
    {
        lcd.clear();
        lcd.print("SD unavailable!");
        sdtrue = 0;
        delay(1500);
    }
    else
    {
        lcd.clear();
        lcd.print("SD available.");
        sdtrue = 1;
    }
    //SDファイル生成
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
        lcd.print("File is ");
        lcd.print(filename);
        File logfile = SD.open(filename, FILE_WRITE);
        logfile.println("Time,Speed,Distance,Ave.spd,Temp");
        logfile.close();
        delay(4000);
    }
    //外字登録
    lcd.createChar(3, charData_3);
    lcd.createChar(4, charData_4);
    lcd.createChar(5, charData_5);
    lcd.createChar(6, charData_6);
    lcd.createChar(8, charData_b1);
    lcd.createChar(9, charData_b2);
    lcd.createChar(2, charData_b3);
    lcd.createChar(1, charData_b4);
    //割り込み
    MsTimer2::set(1, detecter);
    MsTimer2::start();
    //sense初期化
    hole = digitalRead(2);
    if (hole == HIGH)
        rhole = 0;
    else
        rhole = 1;
    //気温読み取り
    temp = (2.504 / 1024.0) * (analogRead(2) - 245.36) / 0.001;
    tempsei = temp / 10;
    tempsyo = temp % 10;
}

void loop()
{
    //体重
    int weight = 48;
    //リフレッシュ
    int late = 500;
    //電圧
    int vbat;
    //カロリー
    int cal;
    int mets;
    //EEPROM
    long lsave = 0;
    //clock
    int watchh, watchm, watchhadd, watchmadd;
    int dp;
    //SettingMode
    int settingmode;
    int sbuttonstatus = 0; //立ち上がりで1,普段は0
    //単位
    int tani = 0;
    //オドメーター
    int odotrue = 1;
    //速度バー
    int bar;
    int bar5;
    float bars;
    //距離(cm)
    long dis = 0.00;
    //最高速度
    int kmhmax = 0;
    //平均速度(15.6kmhなら156)
    float ave;
    int avesei;
    int avesyo;
    //速度(15.6kmhなら156)
    int sp = 0;
    int lsp = 0;
    int kmhsei = 0;
    int kmhsyo = 0;
    //最後の検出
    long ldetect = 0;
    //走行時間(ms)
    long runtime;
    int runh, runm, runs;
    //LCD
    long lrefresh = 0;
    //表示切替
    int button;
    int lbutton;
    int dispMode = 0;
    //タイマ状態
    int timermode;
    //タイマCH1用変数
    long runclock = 0;
    int irunclock = 0;
    int pushtime = 0;
    long lastrefresh = 0;
    int timerstatus = 0;
    int buttonstatus = 0;
    int nowbutton = 0;
    int lastbutton = 0;
    long pushdown, pushup = 0;
    long prestop;
    long starttime;
    long stoptime;
    int clockh, clockm, clocks;
    //タイマCH2用変数
    long runclock2 = 0;
    long irunclock2 = 0;
    int pushtime2 = 0;
    int timerstatus2 = 0;
    int buttonstatus2 = 0;
    int nowbutton2 = 0;
    int lastbutton2 = 0;
    long pushdown2, pushup2 = 0;
    long prestop2;
    long starttime2;
    long stoptime2;
    int clockh2, clockm2, clocks2;
    //ボタン読み取り
    timermode = digitalRead(3);
    nowbutton = digitalRead(17);
    nowbutton2 = digitalRead(6);
    //Setting button Read
    if (timermode == 0)
    {
        if (nowbutton2 != lastbutton2)
        {
            if (nowbutton2 == 0)
                sbuttonstatus = 1;
            lastbutton2 = nowbutton2;
        }
    }
    //設定中SW1読み取り
    if (timermode == 0 && settingmode == 1) //タイマではなく設定中にSW1を読む
    {
        if (nowbutton != lastbutton)
        {
            if (nowbutton == 0)
                buttonstatus = 1;
            lastbutton = nowbutton;
        }
    }

    if (buttonstatus == 1 && timermode == 0 && settingmode == 1)
    { //設定モードでSW1が押された
        buttonstatus = 0;
        switch (dispMode)
        {
        case 10:
            tani ^= 1;
            break;
        case 11:
            odotrue ^= 1;
            break;
        case 12:
            if (watchh != 23)
                watchhadd++;
            else
                watchhadd = 0;
            watchh = millis() / 3600000 + watchhadd;
            break;
        case 13:
            if (watchm != 59)
                watchmadd++;
            else
                watchmadd = 0;
            watchm = (millis() % 3600000) / 60000 + watchmadd;
            break;
        }
    }
    //timer（いじるな）
    if (timermode == 1)
    {
        //CH1のタイマー
        if (nowbutton != lastbutton)
        {

            if (nowbutton == 0)
            {
                pushdown = millis();
                if (timerstatus != 2)
                    buttonstatus = 1;
            }
            else
            {

                if (timerstatus == 2)
                {
                    pushup = millis();
                    pushtime = pushup - pushdown;
                    if ((pushup - pushdown) > 300)
                        buttonstatus = 2;
                    else
                        buttonstatus = 1;
                }
            }
            lastbutton = nowbutton;
        }

        if (buttonstatus == 1)
        {

            if (timerstatus == 0)
            {
                timerstatus = 1;
                starttime = pushdown;
                buttonstatus = 0;
            }
            else if (timerstatus == 1)
            {
                timerstatus = 3;
                stoptime = pushdown;
                prestop = pushdown - starttime;
                buttonstatus = 0;
            }
            else if (timerstatus == 2)
            {
                timerstatus = 1;
                starttime = pushdown - prestop;
                buttonstatus = 0;
            }
            else if (timerstatus == 3)
            {
                timerstatus = 2;
                buttonstatus = 0;
            }
        }
        if (buttonstatus == 2)
        {

            if (timerstatus == 2)
            {
                timerstatus = 0;
                runclock = 0;
            }
            buttonstatus = 0;
        }

        if (timerstatus == 1)
            runclock = (millis() - starttime) / 1000;
        else if (timerstatus == 2)
            runclock = (stoptime - starttime) / 1000;

        //CH2のタイマー
        nowbutton2 = digitalRead(6);
        if (nowbutton2 != lastbutton2)
        {

            if (nowbutton2 == 0)
            {
                pushdown2 = millis();
                if (timerstatus2 != 2)
                    buttonstatus2 = 1;
            }
            else
            {
                if (timerstatus2 == 2)
                {
                    pushup2 = millis();
                    pushtime2 = pushup2 - pushdown2;

                    if ((pushup2 - pushdown2) > 300)
                        buttonstatus2 = 2;
                    else
                        buttonstatus2 = 1;
                }
            }
            lastbutton2 = nowbutton2;
        }

        if (buttonstatus2 == 1)
        {

            if (timerstatus2 == 0)
            {
                timerstatus2 = 1;
                starttime2 = pushdown2;
                buttonstatus2 = 0;
            }
            else if (timerstatus2 == 1)
            {
                timerstatus2 = 3;
                stoptime2 = pushdown2;
                prestop2 = pushdown2 - starttime2;
                buttonstatus2 = 0;
            }
            else if (timerstatus2 == 2)
            {
                timerstatus2 = 1;
                starttime2 = pushdown2 - prestop2;
                buttonstatus2 = 0;
            }
            else if (timerstatus2 == 3)
            {
                timerstatus2 = 2;
                buttonstatus2 = 0;
            }
        }
        if (buttonstatus2 == 2)
        {

            if (timerstatus2 == 2)
            {
                timerstatus2 = 0;
                runclock2 = 0;
            }
            buttonstatus2 = 0;
        }

        if (timerstatus2 == 1)
            runclock2 = (millis() - starttime2) / 1000;
        else if (timerstatus2 == 2)
            runclock2 = (stoptime2 - starttime2) / 1000;
    }

    if (sbuttonstatus == 1)
    {
        switch (dispMode)
        {
        case 10:
            dispMode = 11;
            settingmode = 1;
            break;
        case 11:
            dispMode = 12;
            settingmode = 1;
            break;
        case 12:
            dispMode = 13;
            settingmode = 1;
            break;
        case 13:
            dispMode = 0;
            settingmode = 0;
            break;
        default:
            dispMode = 10;
            settingmode = 1;
            break;
        }
        sbuttonstatus = 0;
    }

    if (timermode == 0 && settingmode == 0) //タイマーモード時にはディスプレイ変更不可（ボタン共通のため）
    {
        button = digitalRead(17);
        if (button != lbutton && lbutton == 1)
        {

            if (dispMode == 0)
                dispMode = 1;
            else if (dispMode == 1)
                dispMode = 2;
            else if (dispMode == 2)
                dispMode = 3;
            else if (dispMode == 3)
            {
                dispMode = 4;
                late = 200;
            }
            else if (dispMode == 4)
            {
                dispMode = 5;
                late = 500;
            }
            else if (dispMode == 5)
                dispMode = 6;
            else if (dispMode == 6)
                dispMode = 0;
            lbutton = 0;
        }
        if (button == 1)
            lbutton = 1;
        else
            lbutton = 0;
    }

    if (detect == 1)
    { //検出
        detect = 0;
        ldetect = millis();
        rcycle = cycle;
        dis += 111; //disはcmで記録
        if (odotrue == 1)
            odo += 111; //odoもcmで記録
        sp = 3600.0 / cycle * 11.1;
        if ((sp / 10) > kmhmax) //最高速度
            kmhmax = (sp / 10);
        kmhsei = sp / 10;
        kmhsyo = sp % 10;
        if (sp >= 30)
            runtime += cycle; //runtimeはmsで記録
        //走行時間計算
        runh = runtime / 3600000;
        runm = (runtime % 3600000) / 60000;
        runs = (runtime / 1000) - (runm * 60) - (runh * 3600);
        Serial.println(runtime);
        //平均速度計算
        ave = ((float)dis / 100000.0) / ((float)runtime / 3600000.0) * 10.0;
        avesei = ave / 10;
        avesyo = (int)ave % 10;
    }
    if ((millis() - ldetect) >= 1500) //1500ms以上停止で0km/h
    {
        kmhsei = 0;
        kmhsyo = 0;
        sp = 0;
    }
    if ((millis() - lsave) % 30000 == 0)
    {
        lsave = millis();
        temp = (2.504 / 1024.0) * (analogRead(2) - 245.36) / 0.001;
        tempsei = temp / 10;
        tempsyo = temp % 10;
        EEPROM.put(10, odo);
    }

    if ((millis() - lrefresh) % late == 0) //500ms毎実行
    {
        //watch
        watchh = millis() / 3600000 + watchhadd;
        watchm = (millis() % 3600000) / 60000 + watchmadd;
        dp ^= 1;
        //SD save
        if (sdtrue == 1)
        {
            File logfile = SD.open(filename, FILE_WRITE);
            if (logfile)
            {
                writeerror = 0;
                logfile.print((float)millis() / 1000);
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
            {
                writeerror = 1;
            }
        }
        //VBat
        vbat = (2.5 / 1024 * analogRead(1) * 300);
        //METs
        if (ave <= 16)
            mets = 40;
        else if (ave <= 19.2)
            mets = 60;
        else if (ave <= 22.4)
            mets = 80;
        else if (ave <= 25.6)
            mets = 100;
        else if (ave <= 30.6)
            mets = 120;
        else
            mets = 158;
        cal = weight * mets / 10.0 * ((float)runtime / 3600000.0) * 1.05;
        //タイマなし
        if (timermode == 0)
        {
            switch (dispMode)
            {
            case 0:
                lcd.clear();
                lcd.print("Speed:");
                lcd.print(kmhsei);
                lcd.print(".");
                lcd.print(kmhsyo);
                lcd.print("km/h");
                lcd.setCursor(0, 1);
                lcd.print("Dis:");
                if (tani == 0)
                {
                    lcd.print((float)dis / 100000);
                    lcd.print("km");
                }
                else
                {
                    lcd.print(dis / 100);
                    lcd.print("m");
                }
                if (writeerror == 1)
                    lcd.print(" SD!");
                break;

            case 1:
                lcd.clear();
                lcd.print("Avg.Spd:");
                lcd.print(avesei);
                lcd.print(".");
                lcd.print(avesyo);
                lcd.print("km/h");
                lcd.setCursor(0, 1);
                lcd.print("RunTime:");
                lcd.print(runh);
                lcd.print(":");
                if (runm < 10)
                    lcd.print("0");
                lcd.print(runm);
                lcd.print(":");
                if (runs < 10)
                    lcd.print("0");
                lcd.print(runs);
                break;

            case 2:
                lcd.clear();
                lcd.print("Temp:");
                lcd.print(tempsei);
                lcd.print(".");
                lcd.print(tempsyo);
                lcd.write(0xDF);
                lcd.print("C");
                lcd.setCursor(0, 1);
                lcd.print("Burn:");
                lcd.print(cal);
                lcd.print("kcal");
                break;

            case 3:
                lcd.clear();
                lcd.print("Odo:");
                lcd.print((float)odo / 100000.0);
                lcd.print("km");
                lcd.setCursor(0, 1);
                lcd.print("MaxSpd:");
                lcd.print(kmhmax);
                lcd.print("km/h");
                break;

            case 4:
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
                lcd.print(kmhsei);
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

            case 5:
                lcd.clear();
                lcd.print("VBat:");
                lcd.print(vbat / 100);
                lcd.print("V");
                break;
            case 6:
                lcd.clear();
                lcd.print("Clock:");
                lcd.print(watchh);
                if (dp == 1)
                    lcd.print(":");
                else
                    lcd.print(" ");
                if (watchm < 10)
                    lcd.print("0");
                lcd.print(watchm);
                break;
            case 10:
                lcd.clear();
                lcd.print("Dist change ?");
                lcd.setCursor(0, 1);
                lcd.print("Current:");
                lcd.print(tani);
                break;
            case 11:
                lcd.clear();
                lcd.print("Odo save enable?");
                lcd.setCursor(0, 1);
                lcd.print("Current:");
                lcd.print(odotrue);
                break;
            case 12:
                lcd.clear();
                lcd.print("Clock hour?");
                lcd.setCursor(0, 1);
                lcd.print("Current:");
                lcd.print(watchh);
                break;
            case 13:
                lcd.clear();
                lcd.print("Clock minute?");
                lcd.setCursor(0, 1);
                lcd.print("Current:");
                lcd.print(watchm);
                break;
            }
        }
        else //タイマモード
        {
            clockh = runclock / 3600;
            irunclock = runclock;
            clockm = (irunclock % 3600) / 60;
            clocks = irunclock - (clockm * 60) - (clockh * 3600);
            clockh2 = runclock2 / 3600;
            irunclock2 = runclock2;
            clockm2 = (irunclock2 % 3600) / 60;
            clocks2 = irunclock2 - (clockm2 * 60) - (clockh2 * 3600);
            lcd.clear();
            lcd.print("CH1:");
            lcd.print(clockh);
            lcd.print("h");
            if (clockm < 10)
                lcd.print("0");
            lcd.print(clockm);
            lcd.print("m");
            if (clocks < 10)
                lcd.print("0");
            lcd.print(clocks);
            lcd.print("s");
            lcd.setCursor(0, 1);
            lcd.print("CH2:");
            lcd.print(clockh2);
            lcd.print("h");
            if (clockm2 < 10)
                lcd.print("0");
            lcd.print(clockm2);
            lcd.print("m");
            if (clocks2 < 10)
                lcd.print("0");
            lcd.print(clocks2);
            lcd.print("s");
        }
    }
}
