/*
 * ------------------------------------------------------
 * Скетч устройства для ввода паролей
 * 
 * Автор: Семенов Александр Сергеевич
 * ------------------------------------------------------
 * Arduino Micro становится USB HID устройством ввода.
 * Для корретной работы необходимо настроить сочетание клавиш для смены раскладки, например CTRL+SHIFT+0 - переключение на английский язык
 * 
 */
#include <SPI.h>
#include <Keyboard.h> // Импорт библиотеки для клавиатуры
#include <MFRC522.h> // Импорт библиотеки "RFID".
#define SS_PIN 10 // Пин для порта Serial, к которому подключается RC522
#define RST_PIN 9 // Пин для сброса, к которому подключается RC522
const int led_pin = 4; // Светодиод для индикации активности устройства
// Пароль для ввода (при вводе пароля на русском используются все равно английские клавиши)
// Разделен на 4 секции для удобства добавления модификаторов клавиш (шифт)
#define password1 "aaaa" // 1 секция пароля
#define password2 "bbbb" // 2 секция пароля (с шифитом)
#define password3 "cccc" // 3 секция пароля
#define password4 "dddd" // 4 секция пароля

MFRC522 mfrc522(SS_PIN, RST_PIN); // Назначаем пины
unsigned long uidDec, uidDecTemp;  // Переменная для хранения номера метки в десятичном формате
void setup() {
  pinMode(led_pin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Ожидание карты...");
  SPI.begin();  // Инициализация SPI
  mfrc522.PCD_Init();     // Инициализация MFRC522 
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
  //Serial.println(uidDec); // Выводим UID метки в консоль. (ВНИМАНИЕ! Отключить после настройки!)
  if (uidDec == 12345678) // Сравниваем Uid метки, если он равен заданому то вводим пароль.
  {
    digitalWrite(led_pin, HIGH);
    Serial.println("Добро пожаловать!");
    Serial.println("Выполняется ввод пароля.");
    // Смена раскладки:
    Keyboard.press(KEY_LEFT_CTRL); // Нажатие клавиши CTRL 
    delay (1000);
    digitalWrite(led_pin, LOW);
    Keyboard.press(KEY_LEFT_SHIFT); // Нажатие клавиши SHIFT 
    Keyboard.print("0"); // Нажатие клавиши номера языка
    Keyboard.release(KEY_LEFT_CTRL); // Отжатие клавиши SHIFT 
    Keyboard.release(KEY_LEFT_SHIFT); // Отжатие клавиши SHIFT 
    //delay (100);
    
    // Начало ввода пароля
    
    // Ввод первой секции
    // Keyboard.press(KEY_LEFT_SHIFT); // Нажатие клавиши SHIFT 
    Keyboard.print(password1);
    // Keyboard.release(KEY_LEFT_SHIFT); // Отжатие клавиши SHIFT 

    // Ввод второй секции
    Keyboard.press(KEY_LEFT_SHIFT); // Нажатие клавиши SHIFT 
    Keyboard.print(password2);
    Keyboard.release(KEY_LEFT_SHIFT); // Отжатие клавиши SHIFT 
    
    // Ввод третьей секции
    // Keyboard.press(KEY_LEFT_SHIFT); // Нажатие клавиши SHIFT 
    Keyboard.print(password3);
    // Keyboard.release(KEY_LEFT_SHIFT); // Отжатие клавиши SHIFT 
    
    // Ввод четвертой секции
    // Keyboard.press(KEY_LEFT_SHIFT); // Нажатие клавиши SHIFT 
    Keyboard.print(password4);
    // Keyboard.release(KEY_LEFT_SHIFT); // Отжатие клавиши SHIFT 

    //Световая индикация
    digitalWrite(led_pin, HIGH);
    //Нажатие Enter
    Keyboard.press(KEY_KP_ENTER); // Нажатие Enter
    delay (100);
    Keyboard.release(KEY_KP_ENTER); // Отпускание Enter
    digitalWrite(led_pin, LOW);
    // Ждем 2 секунды перед новой итерацией цикла
  }
  else 
  {
    Serial.println("Посторонним вход воспрещен.");
  }
  delay (2000);
}
