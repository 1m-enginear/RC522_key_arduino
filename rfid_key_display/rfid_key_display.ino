/*
 * ------------------------------------------------------
 * Скетч устройства для ввода паролей c OLED экраном
 * 
 * Автор: Семенов Александр Сергеевич
 * ------------------------------------------------------
 * Oled экран отображает:
 * - Статус устройства
 * - Выбранный пароль (в разработке)
 * Arduino Micro становится USB HID устройством ввода.
 * Для корретной работы необходимо настроить сочетание клавиш для смены раскладки, например CTRL+SHIFT+0 - переключение на английский язык
 * 
*/
#include <SPI.h>
#include <Keyboard.h> // Импорт библиотеки для клавиатуры
#include <MFRC522.h> // Импорт библиотеки "RFID".
#include <GyverOLED.h>

#define SS_PIN 10 // Пин для порта Serial, к которому подключается RC522
#define RST_PIN 9 // Пин для сброса, к которому подключается RC522

const int led_pin = 4; // Светодиод для индикации активности устройства

// Пароль для ввода
#define password "password"

// Логин и пароль для администратора
#define admin_username "admin_username"
#define admin_password "admin_password"

MFRC522 mfrc522(SS_PIN, RST_PIN); // Назначаем пины
unsigned long uidDec, uidDecTemp;  // Переменная для хранения номера метки в десятичном формате

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

void setup() {
  oled.init();        // инициализация
  oled.clear();       // очистка
  oled.setScale(2);   // масштаб текста (1..4)
  oled.home();        // курсор в 0,0
  oled.print("Приветствие");

  delay(1000);

  oled.setScale(1);
  // курсор на начало 3 строки
  oled.setCursor(0, 3);
  oled.print("Электронный ключ");
  oled.setCursor(0, 4);
  oled.print("*************");
  oled.setCursor(0, 5);
  oled.print("...Ожидание карты...");
  
  pinMode(led_pin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Ожидание карты...");
  SPI.begin();  // Инициализация SPI
  mfrc522.PCD_Init();     // Инициализация MFRC522 
  
  delay(4000);
  oled.clear();       // очистка
}
void loop() {
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Выбор метки
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  uidDec = 0;
  // Считывание серийного номера метки.
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    uidDecTemp = mfrc522.uid.uidByte[i];
    uidDec = uidDec * 256 + uidDecTemp;
  }
  Serial.println("Считывание карты...");
  
  //Вывод на дисплей
  oled.setScale(2);
  oled.setCursor(0, 0);
  oled.print("Чтение...");
  
  //Serial.println("UID карты: ");
  //Serial.println(uidDec); // Выводим UID метки в консоль. (ВНИМАНИЕ! Отключить после настройки!)
  if (uidDec == 00000000) // Сравниваем Uid метки, если он равен заданому то вводим пароль.
  {
    Serial.println("Добро пожаловать!");
    Serial.println("Выполняется ввод пароля.");
    
    //Вывод на дисплей
    oled.setScale(2);
    oled.setCursor(0, 0);
    oled.print("Ввод пароля");
    oled.setScale(1);
    oled.setCursor(0, 3);
    oled.print("Добро пожаловать!");
    oled.setScale(1);
    oled.setCursor(0, 4);
    oled.print("Выполняется ввод");
    oled.setCursor(0, 5);
    oled.print("пароля пользователя");
    
    // Смена раскладки:
    Keyboard.press(KEY_LEFT_CTRL); // Нажатие клавиши CTRL
    // Одиночный сигнал
    digitalWrite(led_pin, HIGH);
    delay (1000);
    digitalWrite(led_pin, LOW);
    Keyboard.press(KEY_LEFT_SHIFT); // Нажатие клавиши SHIFT 
    Keyboard.print("0"); // Нажатие клавиши номера языка
    Keyboard.release(KEY_LEFT_CTRL); // Отжатие клавиши SHIFT 
    Keyboard.release(KEY_LEFT_SHIFT); // Отжатие клавиши SHIFT 
    //delay (100);
    
    // Начало ввода пароля
    Keyboard.print(password);
    
    //Нажатие Enter
    Keyboard.press(KEY_KP_ENTER); // Нажатие Enter
    delay (100);
    Keyboard.release(KEY_KP_ENTER); // Отпускание Enter
  }
  if (uidDec == 00000000)
  {
    Serial.println("Добро пожаловать!");
    Serial.println("Выполняется ввод пароля.");

    //Вывод на дисплей
    oled.setScale(2);
    oled.setCursor(0, 0);
    oled.print("Ввод пароля");
    oled.setScale(1);
    oled.setCursor(0, 3);
    oled.print("Добро пожаловать!");
    oled.setScale(1);
    oled.setCursor(0, 4);
    oled.print("Выполняется ввод");
    oled.setCursor(0, 5);
    oled.print("пароля администратора");
    // Смена раскладки:
    Keyboard.press(KEY_LEFT_CTRL); // Нажатие клавиши CTRL
    
    // Двойной сигнал
    digitalWrite(led_pin, HIGH);
    delay (300);
    digitalWrite(led_pin, LOW);
    delay (100);
    digitalWrite(led_pin, HIGH);
    delay (300);
    digitalWrite(led_pin, LOW);
    
    Keyboard.press(KEY_LEFT_SHIFT); // Нажатие клавиши SHIFT 
    Keyboard.print("0"); // Нажатие клавиши номера языка
    Keyboard.release(KEY_LEFT_CTRL); // Отжатие клавиши SHIFT 
    Keyboard.release(KEY_LEFT_SHIFT); // Отжатие клавиши SHIFT 
    
    // Начало ввода имени пользователя
    Keyboard.print(admin_username);
    delay (100);
    Keyboard.press(KEY_TAB); // Нажатие клавиши TAB для переключения на пароль
    delay (100);
    Keyboard.release(KEY_TAB); // Отпускание клавиши TAB для переключения на пароль
    
    // Начало ввода пароля
    Keyboard.print(admin_password);
    
    //Нажатие Enter
    Keyboard.press(KEY_KP_ENTER); // Нажатие Enter
    delay (100);
    Keyboard.release(KEY_KP_ENTER); // Отпускание Enter
  }
  
  // Ждем 2 секунды перед новой итерацией цикла
  delay (2000);
  oled.clear();
}
