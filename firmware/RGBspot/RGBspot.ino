/*
  Скетч к проекту "RGBspot"
  - Страница проекта (схемы, описания): https://alexgyver.ru/rgbspot/
  - Исходники на GitHub: https://github.com/AlexGyver/RGBspot
  Проблемы с загрузкой? Читай гайд для новичков: https://alexgyver.ru/arduino-first/
  AlexGyver, AlexGyver Technologies, 2021
*/

/*
  Необходимые библиотеки можно установить через менеджер библиотеки
  - EncButton
  - GyverNTC
  - GRGB
  - EEManager
*/

/*
  Настройки компиляции и фьюзов:
  MiniCore https://github.com/MCUdude/MiniCore
  - ATmega328
  - No bootloader
  - Clock: Internal 8 MHz
  
  При 8 МГц частота ШИМ на presc /1 составляет ~16 кГц
*/

// пины заданы здесь
#include "pinMap.h"

// энкодер
#include <EncButton.h>
EncButton<EB_TICK, P_ENC_A, P_ENC_B, P_BTN> enc(INPUT);

// термистор
#include <GyverNTC.h>
GyverNTC therm(P_NTC, 10000, 3950);

// дисплей
#include "SevSeg.h"
const uint8_t digs[] = {P_G4, P_G3, P_G2, P_G1};
const uint8_t segs[] = {P_SSA, P_SSB, P_SSC, P_SSD, P_SSE, P_SSF, P_SSG};
SevSeg<4, SS_CATHODE> disp(digs, segs);

// светодиод
#include <GRGB.h>
GRGB led(COMMON_CATHODE, P_LED_R, P_LED_G, P_LED_B);

// настройки для eeprom 
struct Settings {
  uint8_t mode = 0;   // 0 hue, 1 kelvin
  uint8_t hue = 0;    // цвет
  uint8_t hueB = 0;   // яркость при цвете
  uint8_t temp = 0;   // температура
  uint8_t tempB = 0;  // яркость при температуре
};
Settings settings;

// менеджер eeprom
#include <EEManager.h>
EEManager memory(settings, 10000);  // 10 сек

void setup() {
  /*
    // Пины D3 и D11 - 8 кГц
    TCCR2B = 0b00000010;  // x8
    TCCR2A = 0b00000011;  // fast pwm

    // Пины D9 и D10 - 7.8 кГц
    TCCR1A = 0b00000001;  // 8bit
    TCCR1B = 0b00001010;  // x8 fast pwm
  */

  // Пины D3 и D11 - 31.4 кГц
  TCCR2B = 0b00000001;  // x1
  TCCR2A = 0b00000001;  // phase correct

  // Пины D9 и D10 - 31.4 кГц
  TCCR1A = 0b00000001;  // 8bit
  TCCR1B = 0b00000001;  // x1 phase correct

  pinMode(P_FAN, OUTPUT);

  // запуск епром с адреса 0, ключ 2 (любой 0-255)
  // https://github.com/GyverLibs/EEManager
  memory.begin(0, 2);

  // принудительно обновляем
  updateLED();
  updateDisp();
}

// режим настройки
byte dispMode = 0;  // 0 color, 1 bright

void loop() {
  memory.tick();    // менеджер епром
  disp.tick();      // динамо дисплея
  coolingTick();    // регулирование вентилятора

  if (enc.tick()) {   // опрос, если было событие
    // по клику меняем режим вывода
    if (enc.click()) dispMode = !dispMode;

    // по удержанию меняем режим настройки величина/яркость
    if (enc.held()) {
      settings.mode = !settings.mode;
      dispMode = 0;
    }

    // поворот - меняем величину
    if (enc.turn()) {
      int val = enc.getDir();     // направление поворота
      if (enc.fast()) val *= 5;   // быстрый поворот - в 5 раз быстрее

      if (dispMode) {
        if (!settings.mode) settings.hueB += val;
        else settings.tempB += val;
      } else {
        if (!settings.mode) settings.hue += val;
        else settings.temp += val;
      }
    }

    // обновляем
    updateLED();
    updateDisp();
    enc.resetState();   // сбрасываем флаги (очищаем остальные события)
    memory.update();    // откладываем обновление епром
  }
}

void coolingTick() {
  // таймер на 1 сек
  static uint32_t tmr;
  if (millis() - tmr >= 1000) {
    tmr = millis();
    static float temp = 25;   // фильтрованная температура
    temp += (therm.getTemp() - temp) * 0.2;   // фильтр
    // линейное регулирование
    int duty = map(int(temp), 30, 45, 10, 255);
    duty = constrain(duty, 10, 255);
    analogWrite(P_FAN, duty);
  }
}

void updateDisp() {
  // буквы http://www.uize.com/examples/seven-segment-display.html
  disp.clear();
  if (!settings.mode) {   // hue
    if (dispMode) {
      disp.setOneByte(3, 0x1f);
      disp.setInt(settings.hueB);
    } else {
      disp.setOneByte(3, 0x17);
      disp.setInt(settings.hue);
    }
  } else {                // temp
    if (dispMode) {
      disp.setOneByte(3, 0x1f);
      disp.setInt(settings.tempB);
    } else {
      disp.setOneByte(3, 0x0f);
      disp.setInt(settings.temp);
    }
  }
}

void updateLED() {
  // setWheel принимает 0-1530, у нас 8 бит
  if (!settings.mode) led.setWheel(settings.hue * 6, settings.hueB);
  else led.setKelvin(settings.temp * 100, settings.tempB);
}
