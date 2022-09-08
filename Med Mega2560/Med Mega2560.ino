#include <EEPROM.h>  //Библиотека для работы с энергонезависимой памятью

//Пины направлений для управления 
#define left 10
#define right 11
#define leftFast 5
#define rightFast 6

//Коды режимов работы
const byte mode1 = B001;     //Диафрагма
const byte mode2 = B010;     //ZOOM
const byte mode3 = B011;     //Фокус
const byte mode4 = B100;     //Зеркало
const byte frontSeg = B101;  //Передний отрезок
const byte backSeg = B110;   //Задний отрезок

//Переменная для хранения выбранного режима
byte mode_sel;
//Переменные для выбора пинов для драйвера
byte pinEnable;
byte pinDir;
byte pinStep;
//Переменные для выбора пинов для концевиков
byte Ltrailer;
byte Rtrailer;

//Пины управляющих контактов с драйверов
const byte masEnable[] = { 31, 49, 50, 46, 33 };  //Пины вкл/выкл драйверов
const byte masDir[] = { 30, 48, 52, 44, 32 };     //Пины выбора направления
const byte masStep[] = { 16, 22, 23, 21, 17 };    //Пины шага
//Пины концевиков
const byte masLtrailer[] = {38, 25, 51, 29, 47};  //Левые концевики
const byte masRtrailer[] = {39, 28, 53, 24, 45};  //Правые концевики

//Массивы для хранения шагов
int masFront[5] = {};  //Передний отрезок
int masBack[5] = {};   //Задний отрезок

//Переменные для хранения шагов ШД
int steps;     //Переменная хранения шагов для положений
int stepF;     //Переменная хранения шагов для основных двигателей (передний отрезок)
int stepB;     //Переменная хранения шагов для основных двигателей (задний отрезок)
int stepzF;    //Переменная хранения шагов для второго двигателя "ZOOM" (передний отрезок)
int stepzB;    //Переменная хранения шагов для второго двигателя "ZOOM" (задний отрезок)
int stepmF;    //Переменная хранения шагов для "Зеркало" по Y (передний отрезок)
int stepmB;    //Переменная хранения шагов для "Зеркало" по Y (задний отрезок)
int revSteps;  //Переменная хранения шагов для отката "Фокус" при конфликте с "ZOOM"

bool flagPos = 1;      //Флаг для сброса функции перехода в позицию
bool flagMoveFoc = 0;  //Флаг для передвижения "Фокус"

void setup() 
{
  //Конфигурирование выходов направления и выбора режимов
  for (byte i = 2; i <= 13; i++) 
  {
    pinMode(i, INPUT_PULLUP);
  }
  //Конфигурирование выходов концевиков
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
  //Сохранение шагов в массивы в соответствии с режимом работы
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
  //Записываем сохраненные шаги в энергонезависимую память
  if (digitalRead(9) == 1) 
  {
    EEPROM.put(0, masFront);
    EEPROM.put(10, masBack);
    while (1) {}  //Уходим в бесконечный цикл после сохранения            
  }
  mode_sel = modeVar();  //Считывание режима 
  //Конфигурация пинов в соответствии с выбранным режимом и вызов соответствующих функций
  //Перед конфигурацией записываем в переменные для счета шагов сохраненные значения из массива
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
    //Если выбран режим установки в положение, то устанавливаем положение, зануляем массив и сбрасываем флаг     
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
  //Удаление сохраненных положений. Зануляем массивы и обнуляем переменные хранения шагов
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

//Функция конфигурации пинов для драйвера
//Записываем соответствующие значения из массивов
void configPin(byte pin)
{
  pinEnable = masEnable[pin];
  pinDir = masDir[pin];
  pinStep = masStep[pin];
  Ltrailer = masLtrailer[pin];
  Rtrailer = masRtrailer[pin];
}

//Функция зануления массива
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
  //Пакуем в переменную считанные значения с пинов
  byte x = 0;
  for (byte i = 2; i <= 4; i++) 
  {
    x <<= 1;              //Сдвигаем бит
    x |= digitalRead(i);  //Записываем бит
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
  if (mode_sel == mode1) speed = speed * 2;  //Если выбран режим "Диафрагма", то уменьшаем скорость
  //Крутим двигатель 
  digitalWrite(pinStep, 1);
  delayMicroseconds(500 * speed);
  digitalWrite(pinStep, 0);
  delayMicroseconds(500 * speed);
  stepCount();  //Считаем шаги
}

//Функция подсчета шагов
//Cчет в зависимости от направления (лево -, право +), начальная точка 0
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

//Функция выбора направления и скорости двжения
//Включаем и выключаем соответствующие драйвера в зависимости от выбранного режима
//Включаем и выключаем  соответствующее направление на драйвере в зависимости от выбранного 
//Включаем и выключаем соответствующие пины драйвера для второго двигателя в режиме "ZOOM"
void mode() 
{
  if (digitalRead(left) == 1 || digitalRead(leftFast) == 1) //Диапозон скоростей для левого направления
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
  else if (digitalRead(right) == 1 || digitalRead(rightFast) == 1)  //Диапозон скоростей для правого направления
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

//Функция выбора функции движения для заданного режима (по умолчанию "движение")
//Если пин 8 == 1, то "шаг"
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
  else if (mode_sel == mode2)   //Меняем два двигателя по очереди за две итерации 
  {
    byte n = 0;
    pinStep = 22;  //Пин шага для первого двигателя
    do 
    {
      n++;
      if (digitalRead(8)) modeStep(pinStep, 30);
      else move(pinStep, speed);
      pinStep = 23;   //Пин шага для второго двигателя
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
  int reSteps;           //Переменная хранения шагов при пересчете расстояния 
  bool flagStopFoc = 0;  //Флаг для определения остановки "Фокус"
  for (byte i = 0; i < 5; i++)  //Перебираем ШД по очереди и перемещаем в нужное положение 
  {
    steps = masWay[i];
    revSteps = masWay[1];
    if ((i == 1) && (revSteps < 0))  //Отодвигаем "Фокус" в случае если он мешает движению "ZOOM"
    {
      flagMoveFoc = 1;       //Выставляем флаг для движения
      movePos(2, revSteps);
      masWay[1] = revSteps;  //Сохраняем значение пройденных шагов
      flagMoveFoc = 0;       //Сбрасываем флаг для движения "Фокус"
      flagStopFoc = 1;       //Выставляем флаг для пересчета шагов "Фокус"
    }
    //Пересчитываем расстояние для второго положения
    //Вычитаем из кол-ва шагов второго положения кол-во шагов первого и записываем обратно 
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
  //Конфигурируем соответствующие пины
  pinEnable = masEnable[i];
  pinDir = masDir[i];
  pinStep = masStep[i];
  digitalWrite(pinEnable, 0);  //Включаем драйвер
  //Устанавливаем соответствующиее направление в зависимости от знака переменной хранения шагов
  if (steps < 0) 
  {
    steps = steps * (-1);
    digitalWrite(pinDir, 1);
  } 
  else digitalWrite(pinDir, 0);
  //Движение до заданного положения
  for (int j = 0; j < steps; j++) 
  {    
    if (flagMoveFoc)  //Если был конфликт "Фокус" и "ZOOM", то откатываем "Фокус"
    {
      if (digitalRead(51)) move(pinStep, 2);
      else  //Если "Фокус" уперся в концевик, то сохраняем пройденные шаги 
      {
        revSteps = j * (-1);
        steps = 0;
      }
    }
    else move(pinStep, 2);
  }
  digitalWrite(pinEnable, 1);  //Выключаем драйвер
}
