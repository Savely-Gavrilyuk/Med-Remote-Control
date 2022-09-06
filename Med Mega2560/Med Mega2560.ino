#include <EEPROM.h>

//Пины направлений
#define left 10
#define right 11
#define leftFast 5
#define rightFast 6

//Коды режимов
const byte mode1 = B001;
const byte mode2 = B010;
const byte mode3 = B011;
const byte mode4 = B100;
const byte frontSeg = B101;
const byte backSeg = B110;

//Переменная для хранения режима
byte mode_sel;
//Переменная для хранения пинов с драйверов
byte pinEnable;
byte pinDir;
byte pinStep;
//Переменная для хранения пинов концевиков
byte Ltrailer;
byte Rtrailer;

//Пины управляющих контактов с драйверов
const byte masEnable[] = { 31, 49, 50, 46, 33 };
const byte masDir[] = { 30, 48, 52, 44, 32 };
const byte masStep[] = { 16, 22, 23, 21, 17 };
//Пины концевиков
const byte masLtrailer[] = {38, 25, 51, 29, 47};
const byte masRtrailer[] = {39, 28, 53, 24, 45};

//Массивы для хранения шагов
int masFront[5] = {};
int masBack[5] = {};

//Переменные для хранения шагов
int steps;
int stepF;
int stepB;
int stepzF;
int stepzB;
int stepmF;
int stepmB;
int revSteps;

bool flagPos = 1;      //Флаг для сброса функции перехода в позицию
bool flagMoveFoc = 0;  //Флаг для передвижения фокуса

void setup() 
{
  //Направления и выбор режимов
  for (byte i = 2; i <= 13; i++) 
  {
    pinMode(i, INPUT_PULLUP);
  }
  //Концевики
  pinMode(25, INPUT_PULLUP);
  pinMode(28, INPUT_PULLUP);
  pinMode(38, INPUT_PULLUP);
  pinMode(39, INPUT_PULLUP);
  pinMode(51, INPUT_PULLUP);
  pinMode(53, INPUT_PULLUP);
  pinMode(47, INPUT_PULLUP);
  pinMode(45, INPUT_PULLUP);
  pinMode(29, INPUT_PULLUP);
  pinMode(24, INPUT_PULLUP);
  //Выключаем драйвера (Dir)
  digitalWrite(31, 1);
  digitalWrite(33, 1);
  digitalWrite(46, 1);
  digitalWrite(49, 1);
  digitalWrite(50, 1);
  //Step, Enable
  pinMode(52, OUTPUT);
  pinMode(23, OUTPUT);
  pinMode(48, OUTPUT);
  pinMode(44, OUTPUT);
  pinMode(30, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(17, OUTPUT);
  //Считываем сохраненные положения из памяти
  EEPROM.get(0, masFront);
  EEPROM.get(10, masBack);
  //Установка в заданное положение (передний отрезок)
  position(masFront);
  clearMas(masFront);
}

void loop() 
{
  //Сохранение шагов
  switch (mode_sel) 
  {
    case mode1:
      masFront[0] = stepF;
      masBack[0] = stepB;
      break;
    case mode2:
      masFront[1] = stepzF;
      masBack[1] = stepzB;
      masFront[2] = stepF;
      masBack[2] = stepB;
      break;
    case mode3:
      masFront[2] = stepF;
      masBack[2] = stepB;
      break;
    case mode4:
      masFront[3] = stepF;
      masBack[3] = stepB;
      masFront[4] = stepmF;
      masBack[4] = stepmB;
      break;
  }
  //Записываем сохраненные шаги в память
  if (digitalRead(9) == 1) 
  {
    EEPROM.put(0, masFront);
    EEPROM.put(10, masBack);
    while (1) {}  //Уходим в бесконечный цикл после сохранения            
  }
  mode_sel = modeVar();  //Считывание режима 
  //Выбор режима
  switch (mode_sel) 
  {
    case mode1:
      stepF = masFront[0];
      stepB = masBack[0];
      configPin(0);
      mode();
      break;
    case mode2:
      stepF = masFront[2];
      stepB = masBack[2];
      stepzF = masFront[1];
      stepzB = masBack[1];
      configPin(1);
      mode();
      break;
    case mode3:
      stepF = masFront[2];
      stepB = masBack[2];
      configPin(2);
      mode();
      break;
    case mode4:
      stepF = masFront[3];
      stepB = masBack[3];
      if (digitalRead(left) == 1 || digitalRead(right) == 1) 
      {
        configPin(3);
        mode();
      }
      if (digitalRead(leftFast) == 1 || digitalRead(rightFast) == 1) 
      {
        configPin(4);
        mode();
      }
      break;
    case frontSeg:
      if (flagPos) 
      {
        position(masFront);
        clearMas(masFront);
        stepzF = 0;
        stepmF = 0;
        flagPos = 0;
      }
      break;
    case backSeg:
      if (flagPos) 
      {
        position(masBack);
        clearMas(masBack);
        stepzB = 0;
        stepmB = 0;
        flagPos = 0;
      }
      break;
  }
  //Удаление сохраненных положений
  if (digitalRead(12) == 0) 
  {
    clearMas(masFront);
    stepF = 0;
    stepzF = 0;
    stepmF = 0;
  }
  if (digitalRead(13) == 0) 
  {
    clearMas(masBack);
    stepB = 0;
    stepzB = 0;
    stepmB = 0;
  }
}

//Функция конфигурации пинов c драйвера 
void configPin(byte pin)
{
  pinEnable = masEnable[pin];
  pinDir = masDir[pin];
  pinStep = masStep[pin];
  Ltrailer = masLtrailer[pin];
  Rtrailer = masRtrailer[pin];
}

//Функция очищения массива
void clearMas(int masClear[]) 
{
  for (byte i = 0; i < 5; i++) 
  {
    masClear[i] = 0;
  }
}

//Функция определения режима
byte modeVar() 
{
  byte x = 0;
  for (byte i = 2; i <= 4; i++) 
  {
    x <<= 1;
    x |= digitalRead(i);
  }
  if (x != mode_sel)  //Если режим переключен, выставляем флаг и зануляем текущие шаги
  {
    flagPos = 1;
    stepF = 0;
    stepB = 0;
  }
  return x;
}

//Функция движения
void move(byte pinStep, byte speed) 
{
  if (mode_sel == mode1) speed = speed * 2;
  digitalWrite(pinStep, 1);
  delayMicroseconds(500 * speed);
  digitalWrite(pinStep, 0);
  delayMicroseconds(500 * speed);
  stepCount();  //Считаем шаги
}

//Функция подсчета шагов
void stepCount() 
{
  if (digitalRead(12)) 
  {
    if (digitalRead(pinDir)) 
    {
      if ((mode_sel == mode4) && (digitalRead(leftFast) == 1)) stepmF++;
      else if ((mode_sel == mode2) && pinStep == 22) stepzF++;
      else stepF++;
    } 
    else 
    {
      if ((mode_sel == mode4) && (digitalRead(rightFast) == 1)) stepmF--;
      else if ((mode_sel == mode2) && pinStep == 22) stepzF--;
      else stepF--;
    }
  }
  if (digitalRead(13)) 
  {
    if (digitalRead(pinDir)) 
    {
      if ((mode_sel == mode4) && (digitalRead(leftFast) == 1)) stepmB++;
      else if ((mode_sel == mode2) && pinStep == 22) stepzB++;
      else stepB++;
    } 
    else 
    {
      if ((mode_sel == mode4) && (digitalRead(rightFast) == 1)) stepmB--;
      else if ((mode_sel == mode2) && pinStep == 22) stepzB--;
      else stepB--;
    }
  }
}

//Функция выбора направления
void mode() 
{
  if (digitalRead(left) == 1 || digitalRead(leftFast) == 1) 
  {
    digitalWrite(pinEnable, 0);
    digitalWrite(pinDir, 1);
    if (mode_sel == mode2) 
    {
      digitalWrite(50, 0);
      digitalWrite(52, 1);
    }
    while (digitalRead(left) == 1 && digitalRead(Ltrailer) == 1) 
    {
      move_sel(4);
    }
    while (digitalRead(leftFast) == 1 && digitalRead(Ltrailer) == 1) 
    {
      move_sel(1);
    }
    digitalWrite(pinEnable, 1);
    digitalWrite(50, 1);
  } 
  else if (digitalRead(right) == 1 || digitalRead(rightFast) == 1) 
  {
    digitalWrite(pinEnable, 0);
    digitalWrite(pinDir, 0);
    if (mode_sel == mode2) 
    {
      digitalWrite(50, 0);
      digitalWrite(52, 0);
    }
    while (digitalRead(right) == 1 && digitalRead(Rtrailer) == 1) 
    {
      move_sel(4);
    }
    while (digitalRead(rightFast) == 1 && digitalRead(Rtrailer) == 1) 
    {
      if (mode_sel == mode2 && digitalRead(51) == 1) move_sel(1);
      else if (!(mode_sel == mode2)) move_sel(1);
    }
    digitalWrite(pinEnable, 1);
    digitalWrite(50, 1);
  }
}

//Функция выбора функции скорости для заданного режима
void move_sel(byte speed) 
{
  if (mode_sel == mode1 || mode_sel == mode3) 
  {
    if (digitalRead(8)) 
    {
      if (mode_sel == mode1) modeStep(pinStep, 5);
      else if (mode_sel == mode3) modeStep(pinStep, 30);
    } 
    else move(pinStep, speed * 2);
  } 
  else if (mode_sel == mode2)   //Меняем 2 двигателя по очереди за 2 итерации 
  {
    byte n = 0;
    pinStep = 22;
    do 
    {
      n++;
      if (digitalRead(8)) modeStep(pinStep, 30);
      else move(pinStep, speed);
      pinStep = 23;
    } 
    while (n <= 1);
  } 
  else if (mode_sel == mode4) 
  {
    if (digitalRead(8)) modeStep(pinStep, 5);
    else move(pinStep, 10);
  }
}

//Функция режима шага
void modeStep(byte pinStep, byte turn) 
{
  for (byte i = 0; i < turn; i++)  //Крутим на определенное кол-во шагов
  {
    move(pinStep, 3);
  }
  digitalWrite(pinEnable, 1);
  if (mode_sel == mode2 && pinStep == 23) digitalWrite(50, 1);  //Выключаем Enable для второго ШД
}

//Функция позиционирования в заданное положение (передний и задний отрезок)
void position(int masWay[]) 
{
  int reSteps;           //Переменная для хранения шагов при пересчете расстояния 
  bool flagStopFoc = 0;  //Флаг для определения остановки фокуса
  for (byte i = 0; i < 5; i++)  //Перебираем ШД по очереди и перемещаем в нужное положение 
  {
    steps = masWay[i];
    revSteps = masWay[1];
    if ((i == 1) && (revSteps < 0))  //Отодвигаем Фокус в случае если он мешает движению ZOOM
    {
      flagMoveFoc = 1;
      movePos(2, revSteps);
      masWay[1] = revSteps;  //Сохраняем значение пройденных шагов
      flagMoveFoc = 0;
      flagStopFoc = 1;
    }
    //Пересчитываем расстояние для второго положения
    if (mode_sel == frontSeg) 
    {
      reSteps = masBack[i];
      masBack[i] = reSteps - steps;
    } 
    else if (mode_sel == backSeg) 
    {
      reSteps = masFront[i];
      masFront[i] = reSteps - steps;
    }
    if (i == 2 && flagStopFoc == 1) 
    {
      steps = steps - revSteps;
      flagStopFoc = 0;
    }
    movePos(i, steps);  //Перемещаем ЩД
  }
  steps = 0;
}

//Функия движения до сохраненной позиции
void movePos(byte i, int steps) 
{
  pinEnable = masEnable[i];
  pinDir = masDir[i];
  pinStep = masStep[i];
  digitalWrite(pinEnable, 0);
  if (steps < 0) 
  {
    steps = steps * (-1);
    digitalWrite(pinDir, 1);
  } 
  else digitalWrite(pinDir, 0);
  for (int j = 0; j < steps; j++) 
  {    
    if (flagMoveFoc) 
    {
      if (digitalRead(51)) move(pinStep, 2);
      else  //Если Фокус уперся в концевик, то сохраняем пройденные шаги 
      {
        revSteps = j * (-1);
        steps = 0;
      }
    }
    else move(pinStep, 2);
  }
  digitalWrite(pinEnable, 1);
}
