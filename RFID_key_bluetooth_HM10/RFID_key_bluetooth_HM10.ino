/*
 * ------------------------------------------------------
 * Скетч устройства для ввода паролей
 * Используется Bluetooth молуль HM10 на пинах RX1 и TX0 для смены пароля через приложение на телефоне
 * Автор: Семенов Александр Сергеевич
 * ------------------------------------------------------
 * Arduino Micro становится USB HID устройством ввода.
 * Ввод пароля происходит после обнаружении на модуле RC522 карточки с подходящим ID.
 * Для корретной работы необходимо настроить сочетание клавиш для смены раскладки, а именно CTRL+SHIFT+0 - переключение на английский язык
 * 
 */

#include <SPI.h>
#include <EEPROM.h>      // Хранение данных в EEPROM
#include <Keyboard.h>    // Импорт библиотеки для клавиатуры
#include <MFRC522.h>     // Импорт библиотеки "RFID".
#include "usb_rename.h"  // Библиотека для смены USB дескриптора устройства

#define SS_PIN 10  // Пин для порта Serial, к которому подключается RC522
#define RST_PIN 9  // Пин для сброса, к которому подключается RC522

#define card_uid_user1 01234567  // Ожидаемый UID карты для ввода пароля номер 1 пользователя
#define card_uid_user2 76543210  // Ожидаемый UID карты для ввода пароля номер 2 пользователя


// Указываем новое имя продукта, производителя и серийный номер для маскировки под обычную USB клавиатуру
USBRename dummy = USBRename("USB Keyboard", "Unknown", "2211LZK0445563");

const int led_pin = 4;  // Светодиод для индикации активности устройства

// Пароль для ввода
String user_password;

#define EEPROM_ADDR 0  // Адрес хранения пароля в EEPROM

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

// Функция ввода текста
void type_text(String password, bool type_enter){

  Serial.println("Добро пожаловать!");
  Serial.println("Выполняется ввод пароля.");

  // Начало ввода пароля
  Keyboard.print(password);

  if (type_enter){
    //Нажатие Enter
    Keyboard.press(KEY_KP_ENTER);  // Нажатие Enter
    delay(100);
    Keyboard.release(KEY_KP_ENTER);  // Отпускание Enter
  }

}

// Световая индикация диодом
void diode_blink(int count, int delay_between, int delay_after){
  for (int i = 0; i < count; i++) {
    digitalWrite(led_pin, HIGH);
    delay(delay_between);
    digitalWrite(led_pin, LOW);
    delay(delay_after);
  }
}

// Изменение раскладки
void change_keyboard_layout(String lang){
  String key;
  if (lang == "en") {key = "0";}
  else {key = "1";}
  // Смена раскладки:
  Keyboard.press(KEY_LEFT_CTRL);  // Нажатие клавиши CTRL
  Keyboard.press(KEY_LEFT_SHIFT);    // Нажатие клавиши SHIFT
  Keyboard.print(key);               // Нажатие клавиши номера языка
  Keyboard.release(KEY_LEFT_CTRL);   // Отжатие клавиши SHIFT
  Keyboard.release(KEY_LEFT_SHIFT);  // Отжатие клавиши SHIFT
}

// Логин и пароль для администратора
#define admin_username "admin_username"
#define admin_password "admin_password"
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Назначаем пины
unsigned long uidDec, uidDecTemp;  // Переменная для хранения номера метки в десятичном формате
void setup() {
  pinMode(led_pin, OUTPUT);
  Serial.begin(9600);   // USB Serial
  Serial1.begin(9600);  // Bluetooth HM-10
  // Serial1.print("AT+NAMEAirPods"); // Нужен только для настройки имени Bluetooth модуля

  user_password = loadPassword();  // Загрузка пароля из EEPROM
  // Если пароль отсутствует то ставим стандартный
  if (user_password.length() == 0) {
    user_password = "0000";
    savePassword(user_password);
  }

  Serial.println("Ожидание карты...");
  SPI.begin();         // Инициализация SPI
  mfrc522.PCD_Init();  // Инициализация MFRC522
}
void loop() {

  // Получение команды из Bluetooth
  if (Serial1.available()) {
    String command = Serial1.readStringUntil('\n');
    command.trim();
    // Команда установки пароля
    if (command.startsWith("set_user_password ")) {

      String newPassword = command.substring(18);
      if (newPassword.length() > 0) {
        user_password = newPassword;

        // Сохраняем пароль
        savePassword(user_password);
        Serial1.println("Succes!");

        // Моргаем
        diode_blink(3, 500, 200);
      } else {
        Serial1.println("Error");
      }
    }

    //! Команда установки имени устройства (Пока не работает в текущем виде)
    if (command.startsWith("set_ble_name ")) {
      String newName = command.substring(13);
      newName.trim();
      Serial1.print("AT+NAME" + newName);
      Serial.println("New name: " + newName);
    }

    // Команда вывода сообщений в консоль
    if (command.startsWith("print ")) {
      String text = command.substring(6);
      Serial.println("Принято: " + text);
    }

    // Команда ввода текста напрямую в ПК (пока работает только с латинницей)
    // Для безопасности указывается ID метки RFID
    if (command.startsWith("type " + card_uid_user1)) {
      String text = command.substring(13);
      Serial.println("Ввод текста на ПК: " + text);
      Keyboard.print(text);
    }

    // Команда разблокировки ПК (пока отключена, требуется доработка)
    // Для безопасности указывается ID метки RFID
    // if (command.startsWith("unlock " + card_uid_user1)) {
    //   // String password_number = command.substring(7 + std::to_string(card_uid_user1.strlen()).length(););
    //   password_number = 1
    //   if (password_number == 1){
    //     change_keyboard_layout("en");
    //     type_text(user_password, true);
    //     diode_blink(1, 500, 200);
    //   }
    // }

    // Команда блокировки ПК
    // Для безопасности указывается ID метки RFID
    if (command.startsWith("lock " + card_uid_user1)) {
      delay(100);
      Keyboard.press(KEY_RIGHT_GUI);  // Нажатие клавиши WIN
      delay(100);
      Keyboard.print("l");
      delay(100);
      Keyboard.release(KEY_RIGHT_GUI);
      digitalWrite(led_pin, HIGH);
      delay(2000);
      digitalWrite(led_pin, LOW);
    }
  }

  // Поиск новой метки
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Выбор метки
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  uidDec = 0;
  // Считывание серийного номера метки.
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidDecTemp = mfrc522.uid.uidByte[i];
    uidDec = uidDec * 256 + uidDecTemp;
  }
  Serial.println("Считывание карты...");
  //Serial.println("UID карты: ");
  // Serial.println(uidDec); // Выводим UID метки в консоль. (ВНИМАНИЕ! Отключить после настройки!)
  if (uidDec == card_uid_user1)  // Сравниваем Uid метки, если он равен заданому то вводим пароль.
  {
    change_keyboard_layout("en");
    type_text(user_password, true);
    diode_blink(1, 500, 200);
  }
  if (uidDec == card_uid_user2) {
    change_keyboard_layout("en");
    type_text(admin_username, false);
    Keyboard.press(KEY_TAB);  // Нажатие клавиши TAB для переключения на пароль
    delay(100);
    Keyboard.release(KEY_TAB);  // Отпускание клавиши TAB для переключения на пароль
    type_text(admin_password, true);
    diode_blink(2, 500, 200);
  }

  // Ждем 2 секунды перед новой итерацией цикла
  delay(2000);
}
