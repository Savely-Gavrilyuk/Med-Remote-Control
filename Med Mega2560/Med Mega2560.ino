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

byte mode_sel;
byte pinEnable;
byte pinDir;
byte pinStep;
//Концевики
byte Ltrailer;
byte Rtrailer;

//Пины управляющих контактов с драйверов
const byte masEnable[] = { 31, 49, 50, 46, 33 };
const byte masDir[] = { 30, 48, 52, 44, 32 };
const byte masStep[] = { 16, 22, 23, 21, 17 };

//Массивы для хранения шагов
int masFront[5] = {};
int masBack[5] = {};

//Переменные для хранения шагов
int step;
int stepF;
int stepB;
int stepzF;
int stepzB;
int stepmF;
int stepmB;

bool flag = 1;
bool flagMode;

void setup() {
  Serial.begin(9600);
  //Направления и выбор режимов
  for (byte i = 2; i <= 13; i++) {
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
  //Выключаем драйвера (dir)
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
}

void loop() {
  //Сохранение шагов
  switch (mode_sel) {
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
  mode_sel = modeVar();
  //Выбор режима
  switch (mode_sel) {
    case mode1:
      stepF = masFront[0];
      stepB = masBack[0];
      pinEnable = 31;
      pinDir = 30;
      pinStep = 16;
      Ltrailer = 38;
      Rtrailer = 39;
      mode();
      break;
    case mode2:
      stepF = masFront[2];
      stepB = masBack[2];
      pinEnable = 49;
      pinDir = 48;
      pinStep = 22;
      Ltrailer = 25;
      Rtrailer = 28;
      mode();
      break;
    case mode3:
      stepF = masFront[2];
      stepB = masBack[2];
      pinEnable = 50;
      pinDir = 52;
      pinStep = 23;
      Ltrailer = 51;
      Rtrailer = 53;
      mode();
      break;
    case mode4:
      stepF = masFront[3];
      stepB = masBack[3];
      if (digitalRead(left) == 1 || digitalRead(right) == 1) {
        pinEnable = 46;
        pinDir = 44;
        pinStep = 21;
        Ltrailer = 29;
        Rtrailer = 24;
        mode();
      }
      if (digitalRead(leftFast) == 1 || digitalRead(rightFast) == 1) {
        pinEnable = 33;
        pinDir = 32;
        pinStep = 17;
        Ltrailer = 47;
        Rtrailer = 45;
        mode();
      }
      break;
    case frontSeg:
      if (flag) {
        position(masFront);
        for (byte i = 0; i < 5; i++) {
          masFront[i] = 0;
        }
        stepzF = 0;
        stepmF = 0;
        flag = false;
      }
      break;
    case backSeg:
      if (flag) {
        position(masBack);
        for (byte i = 0; i < 5; i++) {
          masBack[i] = 0;
        }
        stepzB = 0;
        stepmB = 0;
        flag = false;
      }
      break;
  }
  //Удаление сохраненных положений
  if (digitalRead(12) == 0) delPos(masFront);
  if (digitalRead(13) == 0) delPos(masBack);
}

void delPos(int masDel[]) {
  for (byte i = 0; i < 5; i++) {
    masDel[i] = 0;
  }
  stepB = 0;
  stepzB = 0;
  stepmB = 0;
}

byte modeVar() {
  byte x = 0;
  for (byte i = 2; i <= 4; i++) {
    x <<= 1;
    x |= digitalRead(i);
  }
  if (x != mode_sel) {
    flag = true;
    stepF = 0;
    stepB = 0;
  }
  return x;
}

void move(byte pin, byte speed) {
  byte t = 5;
  if (mode_sel == mode1) t = 25;
  digitalWrite(pin, 1);
  delay(t / speed);
  digitalWrite(pin, 0);
  delay(t / speed);
  if (digitalRead(12) == 1) {
    if (digitalRead(pinDir)) {
      if ((mode_sel == mode4) && (digitalRead(leftFast) == 1)) stepmF++;
      else stepF++;
    } else {
      if ((mode_sel == mode4) && (digitalRead(rightFast) == 1)) stepmF--;
      else stepF--;
    }
  }
  if (digitalRead(13) == 1) {
    if (digitalRead(pinDir)) {
      if ((mode_sel == mode4) && (digitalRead(leftFast) == 1)) stepmB++;
      else stepB++;
    } else {
      if ((mode_sel == mode4) && (digitalRead(rightFast) == 1)) stepmB--;
      else stepB--;
    }
  }
}

void mode() {
  flagMode = 1;
  if (digitalRead(left) == 1 || digitalRead(leftFast) == 1) {
    digitalWrite(pinEnable, 0);
    digitalWrite(pinDir, 1);
    if (mode_sel == mode2) {
      digitalWrite(50, 0);
      digitalWrite(52, 1);
    }
    while (digitalRead(left) == 1 && digitalRead(Ltrailer) == 1) {
      move_sel(1);
    }
    while (digitalRead(leftFast) == 1 && digitalRead(Ltrailer) == 1) {
      move_sel(5);
    }
    digitalWrite(pinEnable, 1);
    digitalWrite(50, 1);
  } else if (digitalRead(right) == 1 || digitalRead(rightFast) == 1) {
    digitalWrite(pinEnable, 0);
    digitalWrite(pinDir, 0);
    if (mode_sel == mode2) {
      digitalWrite(50, 0);
      digitalWrite(52, 0);
    }
    while (digitalRead(right) == 1 && digitalRead(Rtrailer) == 1) {
      move_sel(1);
    }
    while (digitalRead(rightFast) == 1 && digitalRead(Rtrailer) == 1) {
      move_sel(5);
    }
    digitalWrite(pinEnable, 1);
    digitalWrite(50, 1);
  }
}

void moveZoom(byte speed) {
  digitalWrite(22, 1);
  digitalWrite(23, 1);
  delay(5 / speed);
  digitalWrite(22, 0);
  digitalWrite(23, 0);
  delay(5 / speed);
  if (digitalRead(12) == 1) {
    if (digitalRead(pinDir)) {
      stepzF++;
      stepF++;
    } else {
      stepzF--;
      stepF--;
    }
  }
  if (digitalRead(13) == 1) {
    if (digitalRead(pinDir)) {
      stepzB++;
      stepB++;
    } else {
      stepzB--;
      stepB--;
    }
  }
}

void move_sel(byte speed)  //функция выбора функции скорости для выбранного режима
{
  if (mode_sel == mode1 || mode_sel == mode3) {
    if (digitalRead(8) == 1 && flagMode == 1) {
      if (mode_sel == mode1) modeStep(pinStep, 5, 5);
      else if (mode_sel == mode3) modeStep(pinStep, 5, 30);
    } else if (digitalRead(8) == 0) move(pinStep, speed);
  } else if (mode_sel == mode2) {
    byte n = 0;
    pinDir = 48;
    pinStep = 22;
    do {
      n++;
      if (digitalRead(8) == 1 && flagMode == 1) modeStep(pinStep, 5, 20);
      else if (digitalRead(8) == 0) move(pinStep, speed);
      pinDir = 52;
      pinStep = 23;
    } while (n <= 1);
  } else if (mode_sel == mode4) {
    if (digitalRead(8) == 1 && flagMode == 1) modeStep(pinStep, 1, 5);
    else if (digitalRead(8) == 0) move(pinStep, 1);
  }
}

void modeStep(byte pinStep, byte speed, byte n) {
  byte x = 1;
  for (byte i = 0; i < n; i++) {
    move(pinStep, speed);
  }
  if (mode_sel == mode2 && x == 1) {
    n++;
    //доделать функцию одновременного шага 2ух шд
  }
  flagMode = 0;
  digitalWrite(pinEnable, 1);
}

void position(int masWay[])  //Функция позиционирования в заданное положение (передний и задний отрезок)
{
  for (byte i = 0; i < 5; i++) {
    int reSteps;
    pinEnable = masEnable[i];
    pinDir = masDir[i];
    pinStep = masStep[i];
    step = masWay[i];
    digitalWrite(pinEnable, 0);
    if (mode_sel == frontSeg) {
      reSteps = masBack[i];
      masBack[i] = reSteps - step;
    } else if (mode_sel == backSeg) {
      reSteps = masFront[i];
      masFront[i] = reSteps - step;
    }
    if (step < 0) {
      step = step * (-1);
      digitalWrite(pinDir, 1);
    } else digitalWrite(pinDir, 0);
    for (int i = 0; i < step; i++) {
      move(pinStep, 5);
    }
    digitalWrite(pinEnable, 1);
  }
}