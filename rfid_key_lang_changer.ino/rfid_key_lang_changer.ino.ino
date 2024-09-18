#include <SPI.h>
#include <Keyboard.h> // Импорт библиотеки для клавиатуры
#include <MFRC522.h> // библиотека "RFID".
#define SS_PIN 10
#define RST_PIN 9
#define password "d6lv;wdke[5wh4f5" // Пароль для ввода
int inByte = 0; // Переменная для чтения раскладки
const int led_pin = 4;

MFRC522 mfrc522(SS_PIN, RST_PIN);
unsigned long uidDec, uidDecTemp;  // для храниения номера метки в десятичном формате
void setup() {
  pinMode(led_pin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Ожидание карты...");
  SPI.begin();  //  инициализация SPI / Init SPI bus.
  mfrc522.PCD_Init();     // инициализация MFRC522 / Init MFRC522 card.
}
void loop() {
  // Поиск новой метки
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Выбор метки
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  uidDec = 0;
  // Выдача серийного номера метки.
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    uidDecTemp = mfrc522.uid.uidByte[i];
    uidDec = uidDec * 256 + uidDecTemp;
  }
  Serial.println("Считывание карты...");
  //Serial.println("UID карты: ");
  //Serial.println(uidDec); // Выводим UID метки в консоль.
  if (uidDec == 64598042) // Сравниваем Uid метки, если он равен заданому то вводим пароль.
  {
    digitalWrite(led_pin, HIGH);
    // Автоматическое определение раскладки:
    if (Serial.available() > 0) {
      delay(100);
      inByte = Serial.read();
      Serial.println(inByte);
      if (inByte == '2') { // Раскладка RU, меняем
        Serial.println("Меняем раскладку на EN");
        Keyboard.press(KEY_LEFT_SHIFT); // Нажатие клавиши Shift для смены раскладки
        Keyboard.press(KEY_LEFT_ALT); // Нажатие клавиши Alt для смены раскладки
        delay (100);
      }
    }
    digitalWrite(led_pin, LOW);
    Serial.println("Добро пожаловать!");
    Serial.println("Ввожу пароль с нажатием Enter.");
    Keyboard.press(KEY_RIGHT_CTRL); // Нажатие клавиши Ctrl для разблокировки экрана
    delay (100);
    Keyboard.release(KEY_RIGHT_CTRL); // Отпускание Ctrl
    digitalWrite(led_pin, HIGH);
    delay (1000);
    Keyboard.print(password); // Ввод пароля
    digitalWrite(led_pin, LOW);
    delay (100);
    Keyboard.press(KEY_KP_ENTER); // Нажатие Enter
    digitalWrite(led_pin, HIGH);
    delay (100);
    Keyboard.release(KEY_KP_ENTER); // Отпускание Enter
    digitalWrite(led_pin, LOW);
    delay (2000);
  }
  else 
  {
    Serial.println("Посторонним вход воспрещен.");
    delay (2000);
  }
  
}
