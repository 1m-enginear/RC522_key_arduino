/*
 * ------------------------------------------------------
 * Скетч устройства для ввода паролей
 * Используется Bluetooth молуль HM10 на пинах RX1 и TX0 для смены пароля
 * Автор: Семенов Александр Сергеевич
 * ------------------------------------------------------
 * Arduino Micro становится USB HID устройством ввода.
 * Для корретной работы необходимо настроить сочетание клавиш для смены раскладки, например CTRL+SHIFT+0 - переключение на английский язык
 * 
 */
 
#include <SPI.h>
#include <EEPROM.h> //* Хранение данных в EEPROM
#include <Keyboard.h> // Импорт библиотеки для клавиатуры
#include <MFRC522.h> // Импорт библиотеки "RFID".
#define SS_PIN 10 // Пин для порта Serial, к которому подключается RC522
#define RST_PIN 9 // Пин для сброса, к которому подключается RC522
const int led_pin = 4; // Светодиод для индикации активности устройства

// Пароль для ввода
String password;

#define EEPROM_ADDR 0 // Адрес хранения пароля в EEPROM

// Функция сохранения пароля в EEPROM
void savePassword(String newPassword) {
  // Записываем длину строки
  EEPROM.update(EEPROM_ADDR, newPassword.length());

  // Записываем символы
  for (int i = 0; i < newPassword.length(); i++) {
    EEPROM.update(EEPROM_ADDR + 1 + i, newPassword[i]);
  }
}

// Функция загрузки пароля из EEPROM
String loadPassword() {
  int length = EEPROM.read(EEPROM_ADDR);

  // Проверяем корректность длины
  if (length <= 0 || length > 50) {
    return "";
  }

  String result = "";

  for (int i = 0; i < length; i++) {
    result += char(EEPROM.read(EEPROM_ADDR + 1 + i));
  }

  return result;
}


// Логин и пароль для администратора
#define admin_username "admin_username"
#define admin_password "admin_password"
MFRC522 mfrc522(SS_PIN, RST_PIN); // Назначаем пины
unsigned long uidDec, uidDecTemp;  // Переменная для хранения номера метки в десятичном формате
void setup() {
  pinMode(led_pin, OUTPUT);
  Serial.begin(9600); // USB Serial
  Serial1.begin(9600); //* Bluetooth HM-10
  //  Serial1.print("AT+NAMERFID-Key_Name"); // Нужен только для настройки имени Bluetooth модуля
  
  password = loadPassword(); //* Загрузка пароля из EEPROM
  //* Если пароль отсутствует то ставим стандартный
  if (password.length()== 0){
    password = "0000";
    savePassword(password);
  }
  
  Serial.println("Ожидание карты...");
  SPI.begin();  // Инициализация SPI
  mfrc522.PCD_Init();     // Инициализация MFRC522 
}
void loop() {

  //* Получение команды из Bluetooth
  if (Serial1.available()){
    String command = Serial1.readStringUntil('\n');
    command.trim();
    // Команда установки пароля
    if (command.startsWith("set_user_password ")){
      
      String newPassword = command.substring(18);
      if (newPassword.length()>0){
        password = newPassword;

        //* Сохраняем пароль
        savePassword(password);
        Serial1.println("Succes!");
        digitalWrite(led_pin, HIGH);
        delay (500);
        digitalWrite(led_pin, LOW);
        delay (200);
        digitalWrite(led_pin, HIGH);
        delay (500);
        digitalWrite(led_pin, LOW);
        delay (200);
        digitalWrite(led_pin, HIGH); 
        delay (500);
        digitalWrite(led_pin, LOW);
      }
      else {
        Serial1.println("Error");
      }
    }
    
    //! Команда установки имени устройства (Пока не работает в текущем виде)
    if(command.startsWith("set_ble_name ")){
      String newName = command.substring(13);
      newName.trim();
      Serial1.print("AT+NAME" + newName);
      Serial.println("New name: " + newName);
    }
    
    // Команда вывода сообщений в консоль
    if(command.startsWith("print ")){
      String text = command.substring(6);
      Serial.println("Принято: " + text);
    }
    
    // Команда ввода текста напрямую в ПК (пока работает только с латинницей)
    // Для безопасности указывается ID метки RFID
    if(command.startsWith("type 01234567")){
      String text = command.substring(13);
      Serial.println("Ввод текста на ПК: " + text);
      Keyboard.print(text);
    }
    
    // Команда блокировки ПК (пока работает только с латинницей)
    // Для безопасности указывается ID метки RFID
    if(command.startsWith("lock 01234567")){
      delay (100);
      Keyboard.press(KEY_RIGHT_GUI); // Нажатие клавиши WIN
      delay (100);
      Keyboard.print("l");
      delay (100);
      Keyboard.release(KEY_RIGHT_GUI);
      digitalWrite(led_pin, HIGH);
      delay (2000);
      digitalWrite(led_pin, LOW);
    }
  }

  // Поиск новой метки
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
  //Serial.println("UID карты: ");
  //Serial.println(uidDec); // Выводим UID метки в консоль. (ВНИМАНИЕ! Отключить после настройки!)
  if (uidDec == 01234567) // Сравниваем Uid метки, если он равен заданому то вводим пароль.
  {
    digitalWrite(led_pin, HIGH);
    Serial.println("Добро пожаловать!");
    Serial.println("Выполняется ввод пароля.");
    // Смена раскладки:
    Keyboard.press(KEY_LEFT_CTRL); // Нажатие клавиши CTRL
    delay (1000);
    digitalWrite(led_pin, LOW);
    delay (200);
    
    // Одиночный сигнал
    digitalWrite(led_pin, HIGH);
    delay (200);
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
  if (uidDec == 76543210)
  {
    digitalWrite(led_pin, HIGH);
    Serial.println("Добро пожаловать!");
    Serial.println("Выполняется ввод пароля.");
    // Смена раскладки:
    Keyboard.press(KEY_LEFT_CTRL); // Нажатие клавиши CTRL
    delay (1000);
    digitalWrite(led_pin, LOW);
    delay (200);
    
    // Двойной сигнал
    digitalWrite(led_pin, HIGH);
    delay (100);
    digitalWrite(led_pin, LOW);
    delay (100);
    digitalWrite(led_pin, HIGH);
    delay (200);
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
}
