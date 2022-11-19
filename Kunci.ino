#include <Wire.h> // gunakan pustaka untuk protokol i2c
#include <LiquidCrystal_I2C.h> // gunakan library untuk tampilan lcd1602 di i2c
//#include <LiquidCrystal.h>  //this library is included in the Arduino IDE

#include <RTClib.h>
#include <Keypad.h> // library untuk keyboard http://playground.arduino.cc/uploads/Code/keypad.zip
#include "Password.h" // perpustakaan untuk kata sandi http://playground.arduino.cc/uploads/Code/Password.zip
Password password = Password( "1234" ); // katw sandi primer.
Password password1 = Password( "5678" ); // katw sandi sekunder.

// tampilkan 1602 di i2c dengan alamat 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

/*
  LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
                              -------------------
                                      |  LCD  | Arduino |
                                      -------------------
  LCD RS pin to digital pin 12         |  RS   |   D12   |
  LCD Enable pin to digital pin 11     |  E    |   D11   |
  LCD D4 pin to digital pin 5          |  D4   |   D6    |
  LCD D5 pin to digital pin 4          |  D5   |   D4    |
  LCD D6 pin to digital pin 3          |  D6   |   D3    |
  LCD D7 pin to digital pin 2          |  D7   |   D2    |
  LCD R/W pin to ground                |  R/W  |   GND   |
                                      -------------------
*/

// Modul RTC dengan DS1307 atau DS3231.
RTC_DS1307 RTC;

const byte rows = 4; // jumlah baris keypad
const byte cols = 3; // jumlah kolom keypad
char keys[rows][cols] = { // tombol pada keypad
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// byte rowPins[rows] = {R0, R1, R2, R3};
//byte colPins[cols] = {C0, C1, C2};

// keypad datar - tipe Adafruit
//byte rowPins[rows] = {1, 2, 3, 4};
//byte colPins[cols] = {5, 6, 7};

// keypaddengan tombol timbul - model KB304-PAW
byte rowPins[rows] = {2, 7, 6, 4};
// baris 1 = pin 2 mcu
// baris 2 = pin 7 mcu
// baris 3 = pin 6 mcu
// baris 4 = pin 4 mcu
byte colPins[cols] = {3, 1, 5};
// kolom 1 = pin 3 mcu
// kolom 2 = pin 1 mcu
// kolom 3 = pin 5 mcu

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);


#define tombol 12 // pushbutton terhubung ke pin digital 12 mcu
#define ind_unlock 10 // indikator led satus kunci terbuka
#define ind_lock 9 // indikator led satus dikunci
#define ind_locked 8 // indikator led satus terkunci permanen
#define mastercod 11
#define scurt 500 // waktu dalam ms untuk di hapus
#define tpactuator 700 // waktu dalam ms untuk masa aktif aktuator


// http://arduino.cc/en/Reference/LiquidCrystalCreateChar
byte gembok[8] = {
  B01110,
  B10001,
  B00001,
  B00001,
  B11111,
  B11111,
  B11111,
};

byte kunci[8] = {
  B00110,
  B01001,
  B00110,
  B00010,
  B01110,
  B00010,
  B01110,
};

byte jam1[8] = {
  B00000,
  B01110,
  B10101,
  B10101,
  B10001,
  B01110,
  B00000,
};

byte jam2[8] = {
  B00000,
  B01110,
  B10001,
  B10111,
  B10001,
  B01110,
  B00000,
};

byte jam3[8] = {
  B00000,
  B01110,
  B10001,
  B10101,
  B10101,
  B01110,
  B00000,
};

byte jam4[8] = {
  B00000,
  B01110,
  B10001,
  B11101,
  B10001,
  B01110,
  B00000,
};

byte TutupJam = 21; // batas jam tutup akses otomatos pintu
byte TutupMenit = 10; // batas menit tutup akses pintu
byte BukaJam = 7; // mulai jam buka akses otomatis pintu
byte BukaMenit = 30; // mulai menit buka akses otomatis pintu

void setup()
{
  keypad.addEventListener(keypadEvent); // objek dibuat untuk melacak penekanan tombol

  pinMode(tombol, INPUT);
  pinMode(ind_locked, OUTPUT);
  pinMode(ind_unlock, OUTPUT);
  pinMode(ind_lock, OUTPUT);
  pinMode(mastercod, OUTPUT);

  digitalWrite(tombol, HIGH); // input tombol dalam logika 1
  digitalWrite(ind_locked, HIGH);
  digitalWrite(ind_unlock, LOW);
  digitalWrite(ind_lock, LOW);
  digitalWrite(mastercod, LOW);

  lcd.begin(16, 2); // menginisialisasi tampilan
  lcd.backlight(); // aktifkan latar belakang
  lcd.createChar(0, gembok); // membuat karakter unik
  lcd.createChar(1, kunci); // membuat karakter unik
  lcd.createChar(2, jam1); // membuat karakter unik
  lcd.createChar(3, jam2); // membuat karakter unik
  lcd.createChar(4, jam3); // membuat karakter unik
  lcd.createChar(5, jam4); // membuat karakter unik
  // lcd.write(byte(0)); // perintah untuk menampilkan karakter unik

  lcd.setCursor(0, 0); // menempatkan kursor pada baris ke-1, kolom ke-1
  lcd.print("sistem akses"); // menampikan teks "sistem akses"
  lcd.setCursor(0, 1); // menempatkan kursor pada baris ke-2, kolom ke-1
  lcd.print("pintu katasandi"); // menampikan teks "pintu katasandi"

  delay(3000);
  lcd.clear();

  lcd.setCursor(0, 0); // menempatkan kursor pada baris ke-1, kolom ke-1
  // lcd.print("terkunci"); // menampikan teks "pintu terkunci"
  // lcd.write(byte(0));
  // lcd.write(byte(1)); // perintah untuk menampilkan karakter unik

  Wire.begin();
  Wire.beginTransmission(0x68);

  Wire.write(0x07); // pindahkan penunjuk ke alamat SQW
  Wire.write(0x10); // mengirim 0x10 (hex) 00010000 (biner) ke register kontrol - menyalakan gelombang persegi
  Wire.endTransmission();

  RTC.begin();

  // jika perlu menyetel jam... hapus saja baris ini

  if (! RTC.isrunning())
  {
    Serial.println("RTC is NOT running!");

    // baris berikut menyetel RTC ke tanggal & waktu
    RTC.adjust(DateTime(__DATE__, __TIME__));
    // Baris ini menyetel RTC dengan tanggal & waktu eksplisit, misalnya untuk menyetel 21 Januari 2014 jam 3 pagi:
    // RTC.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  // RTC.adjust(DateTime(__DATE__, __TIME__));
  // RTC.adjust(DateTime(2016, 9, 11, 13, 24, 0));
}

void loop()
{
  DateTime now = RTC.now();
  lcd.setCursor(3, 0);
  if ( now.hour() < 10)
  {
    lcd.print(" ");
    lcd.print(now.hour(), DEC);
  }
  else
  {
    lcd.print(now.hour(), DEC);
  }
  // lcd.print(":");

  if ( now.second() % 2 == 0)
    lcd.print(":");
  else
    lcd.print(" ");

  if ( now.minute() < 10)
  {
    lcd.print("0");
    lcd.print(now.minute(), DEC);
  }
  else
  {
    lcd.print(now.minute(), DEC);
  }

  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////

  // lcd.print(":");
  // if ( now.second() < 10)
  // {
  //  lcd.print("0");
  //  lcd.print(now.second(), DEC);
  // }
  // else
  // {
  //  lcd.print(now.second(), DEC);
  // }
  // lcd.print(" ");

  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////

  if (digitalRead(mastercod) == HIGH)
  { // jam diaktifkan
    lcd.setCursor(10, 0);
    lcd.write(byte(1)); // kunci

    if (TutupJam < 10)
      lcd.print(" ");
    lcd.print(TutupJam);
    lcd.print(":");
    if (TutupMenit < 10)
      lcd.print("0");
    lcd.print(TutupMenit);
    lcd.setCursor(10, 1);
    lcd.write(byte(0)); // kunci terbuka
    if (BukaJam < 10)
      lcd.print(" ");
    lcd.print(BukaJam);
    lcd.print(":");
    if (BukaMenit < 10)
      lcd.print("0");
    lcd.print(BukaMenit);

    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////

    // int angka = now.second();
    // lcd.setCursor(9, 0);
    // if (angka %4 == 0)
    // {
    //  lcd.write(byte(2));
    // }
    // if (angka %4 == 1)
    // {
    //  lcd.write(byte(3));
    // }
    // if (angka %4 == 2)
    // {
    //  lcd.write(byte(4));
    // }
    // if (angka %4 == 3)
    // {
    //  lcd.write(byte(5));
    // }

    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////

  }
  else
  {
    // lcd.print(" ");
    lcd.setCursor(10, 0);
    lcd.print("      ");
    lcd.setCursor(10, 1);
    lcd.print("      ");
  }

  lcd.setCursor(0, 1);
  if ( now.day() < 10)
  {
    lcd.print("0");
    lcd.print(now.day(), DEC);
  }
  else
  {
    lcd.print(now.day(), DEC);
  }

  lcd.print("/");
  if ( now.month() < 10)
  {
    lcd.print("0");
    lcd.print(now.month(), DEC);
  }
  else
  {
    lcd.print(now.month(), DEC);
  }

  lcd.print("/");
  lcd.print(now.year() - 2000, DEC);
  lcd.print("");

  keypad.getKey();

  if (digitalRead(tombol) == LOW)
  { // tombol di tekan
    // tindakan();
    pembukaan();
  }

  if (digitalRead(mastercod) == HIGH) // jika jam dinonaktifkan
  {
    if ((now.hour() == BukaJam) && now.minute() == BukaMenit)
    {
      // if (digitalRead(ind_locked) == HIGH)
      // pembukaan();
      if ((digitalRead(ind_locked) == HIGH) && (now.second() < 3))
        pembukaan();
    }

    if ((now.hour() == TutupJam) && now.minute() == TutupMenit)
    {
      // if (digitalRead(ind_locked) == LOW)
      penutupan();
      if ((digitalRead(ind_locked) == LOW) && (now.second() < 3) == LOW)
        penutupan();
    }
  }
}

//mengatur ekspresi keypad khusus
void keypadEvent(KeypadEvent eKey)
{
  switch (keypad.getState())
  {
    case PRESSED:
      // Serial.print("tekan: ");
      // Serial.println(eKey);
      switch (eKey)
      {
        case '#':
          checkPassword();
          break;
        case '*':
          password.reset();
          password1.reset();
          break;
        default:
          password.append(eKey);
          password1.append(eKey);
      }
  }
}

void checkPassword()
{
  if (password.evaluate())
  { // base dasar utama
    Serial.println("Sukses..");
    // Tambahkan kode untuk dijalankan jika berhasil
    if (digitalRead(ind_locked) == LOW)
      penutupan();
    else
      pembukaan();
    password.reset();
    password1.reset();
  }
  else if (password1.evaluate())
  {
    Serial.println("Sukses..        ");

    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////

    // Tambahkan kode untuk dijalankan jika berhasil
    // if (digitalRead(mastercod) == LOW)
    // digitalWrite(mastercod, HIGH);
    // else
    // digitalWrite(mastercod, LOW);

    // if ((digitalRead(ind_locked) == LOW) && (digitalRead(mastercod) == HIGH))
    // digitalWrite(mastercod, LOW);
    // else
    // digitalWrite(mastercod, HIGH);

    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////

    if (digitalRead(ind_locked) == LOW)
    {
      if (digitalRead(mastercod) == HIGH)
        digitalWrite(mastercod, LOW);
      else
        digitalWrite(mastercod, HIGH);
    }
    password.reset();
    password1.reset();
  }
  else
  {
    Serial.println("Salah");
    // tambahkan kode untuk dijalankan jika tidak berhasil
    password.reset();
    password1.reset();
    delay(1000);
  }
}

void pembukaan()
{
  // lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(0));
  // lcd.write(byte(1));
  digitalWrite(ind_unlock, HIGH);
  delay(tpactuator);
  digitalWrite(ind_unlock, LOW);
  digitalWrite(ind_locked, LOW);
  // lcd.clear();
}

void penutupan()
{
  digitalWrite(ind_lock, HIGH);
  delay(tpactuator);
  digitalWrite(ind_lock, LOW);
  lcd.setCursor(0, 0);
  lcd.write(byte(1));
  digitalWrite(ind_locked, HIGH);
}
