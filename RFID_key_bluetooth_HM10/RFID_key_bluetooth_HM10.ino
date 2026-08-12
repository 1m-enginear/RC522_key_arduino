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
#include <AESLib.h>      // Библиотека шифрования
#define SS_PIN 10  // Пин для порта Serial, к которому подключается RC522
#define RST_PIN 9  // Пин для сброса, к которому подключается RC522

#define card_uid_user1 01234567  // Ожидаемый UID карты для ввода пароля номер 1 пользователя
#define card_uid_user2 76543210  // Ожидаемый UID карты для ввода пароля номер 2 пользователя

// Настройка AES
AESLib aesLib;
// Ключ шифрования
byte aes_key[16] = {
  0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
  0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};
// Вектор инициализации
// Для EEPROM подойдет фиксированный
byte aes_iv[16] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
// Буферы для работы
byte encryptedBuffer[64];
byte decryptedBuffer[64];

// Логин и пароль для администратора
#define admin_username "admin_username"
#define admin_password "admin_password"
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Назначаем пины
unsigned long uidDec, uidDecTemp;  // Переменная для хранения номера метки в десятичном формате

// Указываем новое имя продукта, производителя и серийный номер для маскировки под обычную USB клавиатуру
USBRename dummy = USBRename("USB Keyboard", "Unknown", "2211LZK0445563");

const int led_pin = 4;  // Светодиод для индикации активности устройства

// Пароль для ввода
String user_password;

#define EEPROM_ADDR 0  // Адрес хранения пароля в EEPROM

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ (экспериментальные функции) +++++++++++++++++++++++++++++++++++++++++++++++++++++

// ============= ФУНКЦИЯ ШИФРОВАНИЯ =============
String encryptString(String input) {
  if (input.length() == 0) return "";

  // Копируем IV перед каждым шифрованием (библиотека его изменяет)
  byte iv[16];
  memcpy(iv, aes_iv, 16);

  // Шифруем в формат base64 (строка)
  int encLen = aesLib.encrypt64(
    (const byte*)input.c_str(),
    input.length(),
    (char*)encryptedBuffer,
    aes_key,
    128,      // Размер ключа в битах
    iv
  );

  // Обрезаем буфер до реальной длины
  encryptedBuffer[encLen] = '\0';
  return String((char*)encryptedBuffer);
}

// ============= ФУНКЦИЯ РАСШИФРОВАНИЯ =============
String decryptString(String encrypted) {
  if (encrypted.length() == 0) return "";

  // Копируем IV перед расшифровкой
  byte iv[16];
  memcpy(iv, aes_iv, 16);

  // Расшифровываем из base64
  int decLen = aesLib.decrypt64(
    encrypted.c_str(),
    encrypted.length(),
    decryptedBuffer,
    aes_key,
    128,
    iv
  );

  decryptedBuffer[decLen] = '\0';
  return String((char*)decryptedBuffer);
}


// Конфигурация для сохранения в EEPROM
#define EEPROM_START_ADDR 0
#define MAX_ACCOUNTS 20              // Максимум аккаунтов
#define MAX_LOGIN_LENGTH 20          // Максимальная длина логина
#define MAX_PASSWORD_LENGTH 30       // Максимальная длина пароля
#define RECORD_SIZE (1 + MAX_LOGIN_LENGTH + 1 + MAX_PASSWORD_LENGTH) 

// ============= СОХРАНЕНИЕ ПО ИНДЕКСУ =============
bool saveAccountByIndex(int index, String login, String password) {
  // Проверка индекса
  if (index < 0 || index >= MAX_ACCOUNTS) {
    return false;
  }
  
  // Проверка длины логина
  int loginLen = login.length();
  if (loginLen > MAX_LOGIN_LENGTH) {
    loginLen = MAX_LOGIN_LENGTH;
    login = login.substring(0, MAX_LOGIN_LENGTH);
  }
  
  // Проверка длины пароля
  int passLen = password.length();
  if (passLen > MAX_PASSWORD_LENGTH) {
    passLen = MAX_PASSWORD_LENGTH;
    password = password.substring(0, MAX_PASSWORD_LENGTH);
  }
  
  // Вычисляем адрес для этого индекса
  int addr = EEPROM_START_ADDR + (index * RECORD_SIZE);
  
  // Сохраняем длину логина
  EEPROM.update(addr, loginLen);
  addr++;
  
  // Сохраняем логин
  for (int i = 0; i < loginLen; i++) {
    EEPROM.update(addr + i, login[i]);
  }
  // Очищаем остаток ячейки логина
  for (int i = loginLen; i < MAX_LOGIN_LENGTH; i++) {
    EEPROM.update(addr + i, 0);
  }
  addr += MAX_LOGIN_LENGTH;
  
  // Сохраняем длину пароля
  EEPROM.update(addr, passLen);
  addr++;
  
  // Сохраняем пароль
  for (int i = 0; i < passLen; i++) {
    EEPROM.update(addr + i, password[i]);
  }
  // Очищаем остаток ячейки пароля
  for (int i = passLen; i < MAX_PASSWORD_LENGTH; i++) {
    EEPROM.update(addr + i, 0);
  }
  
  return true;
}

// ============= ЗАГРУЗКА ПО ИНДЕКСУ =============
bool loadAccountByIndex(int index, String &login, String &password) {
  // Проверка индекса
  if (index < 0 || index >= MAX_ACCOUNTS) {
    return false;
  }
  
  // Вычисляем адрес
  int addr = EEPROM_START_ADDR + (index * RECORD_SIZE);
  
  // Читаем длину логина
  int loginLen = EEPROM.read(addr);
  addr++;
  
  // Проверка длины логина
  if (loginLen < 0 || loginLen > MAX_LOGIN_LENGTH) {
    return false;
  }
  
  // Формируем логин
  login = "";
  for (int i = 0; i < loginLen; i++) {
    char c = char(EEPROM.read(addr + i));
    if (c == 0) break;
    login += c;
  }
  addr += MAX_LOGIN_LENGTH;
  
  // Читаем длину пароля
  int passLen = EEPROM.read(addr);
  addr++;
  
  // Проверка длины пароля
  if (passLen < 0 || passLen > MAX_PASSWORD_LENGTH) {
    return false;
  }
  
  // Формируем пароль
  password = "";
  for (int i = 0; i < passLen; i++) {
    char c = char(EEPROM.read(addr + i));
    if (c == 0) break;
    password += c;
  }
  
  return true;
}

// ============= ПРОВЕРКА СУЩЕСТВОВАНИЯ АККАУНТА =============
bool isAccountExists(int index) {
  if (index < 0 || index >= MAX_ACCOUNTS) return false;
  
  int addr = EEPROM_START_ADDR + (index * RECORD_SIZE);
  int loginLen = EEPROM.read(addr);
  
  return (loginLen > 0 && loginLen <= MAX_LOGIN_LENGTH);
}

// ============= УДАЛЕНИЕ ПО ИНДЕКСУ =============
void deleteAccountByIndex(int index) {
  if (index < 0 || index >= MAX_ACCOUNTS) return;
  
  int addr = EEPROM_START_ADDR + (index * RECORD_SIZE);
  
  // Очищаем всю запись
  for (int i = 0; i < RECORD_SIZE; i++) {
    EEPROM.update(addr + i, 0);
  }
}

// ============= ПОИСК ПО ЛОГИНУ =============
int findAccountByLogin(String login) {
  String tempLogin, tempPass;
  
  for (int i = 0; i < MAX_ACCOUNTS; i++) {
    if (loadAccountByIndex(i, tempLogin, tempPass)) {
      if (tempLogin == login) {
        return i;
      }
    }
  }
  return -1; // Не найден
}

// ============= ПОЛУЧИТЬ КОЛИЧЕСТВО АККАУНТОВ =============
int getAccountsCount() {
  int count = 0;
  for (int i = 0; i < MAX_ACCOUNTS; i++) {
    if (isAccountExists(i)) {
      count++;
    }
  }
  return count;
}

// ============= ПОИСК СВОБОДНОГО ИНДЕКСА =============
int findFreeIndex() {
  for (int i = 0; i < MAX_ACCOUNTS; i++) {
    if (!isAccountExists(i)) {
      return i;
    }
  }
  return -1;
}

// ============= ЗАГРУЗКА ВСЕХ АККАУНТОВ =============
int loadAllAccounts(String logins[], String passwords[], int maxCount) {
  int count = 0;
  String tempLogin, tempPass;
  
  for (int i = 0; i < MAX_ACCOUNTS && count < maxCount; i++) {
    if (loadAccountByIndex(i, tempLogin, tempPass)) {
      logins[count] = tempLogin;
      passwords[count] = tempPass;
      count++;
    }
  }
  return count;
}

// ============= ЗАГРУЗКА С ИНДЕКСАМИ =============
int loadAllAccountsWithIndices(String logins[], String passwords[], int indices[], int maxCount) {
  int count = 0;
  String tempLogin, tempPass;
  
  for (int i = 0; i < MAX_ACCOUNTS && count < maxCount; i++) {
    if (loadAccountByIndex(i, tempLogin, tempPass)) {
      logins[count] = tempLogin;
      passwords[count] = tempPass;
      indices[count] = i;
      count++;
    }
  }
  return count;
}

// ============= ОЧИСТКА ВСЕЙ EEPROM =============
void clearAllAccounts() {
  for (int i = 0; i < MAX_ACCOUNTS; i++) {
    deleteAccountByIndex(i);
  }
}

// ============= ОБНОВЛЕНИЕ ПАРОЛЯ ПО ЛОГИНУ =============
bool updatePasswordByLogin(String login, String newPassword) {
  int index = findAccountByLogin(login);
  if (index == -1) return false;
  
  String existingLogin, existingPass;
  loadAccountByIndex(index, existingLogin, existingPass);
  
  return saveAccountByIndex(index, existingLogin, newPassword);
}

// ============= ДОБАВЛЕНИЕ НОВОГО АККАУНТА =============
bool addNewAccount(String login, String password) {
  // Проверка, существует ли уже такой логин
  if (findAccountByLogin(login) != -1) {
    return false; // Логин уже существует
  }
  
  int freeIndex = findFreeIndex();
  if (freeIndex == -1) {
    return false; // Нет свободного места
  }
  
  return saveAccountByIndex(freeIndex, login, password);
}
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ (конец экспериментальных функций) +++++++++++++++++++++++++++++++++++++++++++++++++++++



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

// ============= ВВОД ТЕКСТА =============
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

// ============= СВЕТОВАЯ ИНДИКАЦИЯ =============
void diode_blink(int count, int delay_between, int delay_after){
  for (int i = 0; i < count; i++) {
    digitalWrite(led_pin, HIGH);
    delay(delay_between);
    digitalWrite(led_pin, LOW);
    delay(delay_after);
  }
}

// ============= ИЗМЕНЕНИЕ РАСКЛАДКИ =============
void change_keyboard_layout(String lang){
  String key;
  if (lang == "en") {key = "0";}
  else {key = "1";}
  // Смена раскладки:
  Keyboard.press(KEY_LEFT_CTRL);  // Нажатие клавиши CTRL
  delay (800);
  Keyboard.press(KEY_LEFT_SHIFT);    // Нажатие клавиши SHIFT
  Keyboard.print(key);               // Нажатие клавиши номера языка
  Keyboard.release(KEY_LEFT_CTRL);   // Отжатие клавиши SHIFT
  Keyboard.release(KEY_LEFT_SHIFT);  // Отжатие клавиши SHIFT
}

// ============= ИНИЦИАЛИЗАЦИЯ =============
void setup() {
  pinMode(led_pin, OUTPUT);
  Serial.begin(9600);   // USB Serial
  Serial1.begin(9600);  // Bluetooth HM-10
//  Serial1.print("AT+NAMEAirPods"); // Нужен только для настройки имени Bluetooth модуля

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

// ============= ОСНОВНОЙ ЦИКЛ =============
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

    //! Команда установки имени устройства (Работает при отключении устройства)
    if (command.startsWith("set_ble_name ")) {
      String newName = command.substring(13);
      newName.trim();
      Serial1.println("AT");
      delay(5000);
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
