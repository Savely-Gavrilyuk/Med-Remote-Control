#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//Ввод данных сети
#define ssid "Remote Control"
#define password "123456789"
#define PARAM_INPUT_1 "direction"

//Строки для управления выбором режима
String direction;
String modestate;
String FRONT;
String BACK;
String STEP;

AsyncWebServer server(80);  //Создаем сервер на порту 80

//Меню выбора
const char main_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    <!-- Задаем размеры страницы в соответствии с размером экрана -->
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Пульт управления</title>
    <link rel="icon" href="data:," />
    <style>
      /* Устанавливаем положение всех элементов */
      body {
        font-family: Arial;
        text-align: center;
        margin: 0px auto;
        padding-top: 8px;
      }
      /* Отменяем выделения при нажатии */
      .noselect {
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(255, 255, 255, 0);
        -webkit-tap-highlight-color: transparent;
      }
    </style>
  </head>
  <body>
    <div class="noselect">
      <!-- Форма для выбора режима -->
      <form action="/" method="POST">
        <p>Выбранный режим: <strong>%STATE%</strong></p>
        <style>
          /* Положение радио кнопок (текст) */
          .container {
            display: block;
            position: relative;
            padding-left: 35px;
            margin-bottom: 20px;
            cursor: pointer;
            font-size: 22px;
          }
          /* Скрываем переключатели по умолчанию */
          .container input {
            position: center;
            opacity: 0;
            cursor: pointer;
          }
          /* Создание пользовательского переключателя */
          .checkmark {
            position: absolute;
            top: 0;
            left: 70px;
            height: 25px;
            width: 25px;
            background-color: #eee;
            border-radius: 50%;
          }
          /* Когда переключатель установлен, добавляем синий фон */
          .container input:checked ~ .checkmark {
            background-color: #2196f3;
          }
        </style>
        <!-- Радио кнопки (выбор кнопки отправляется запросом на сервер, по умолчанию ничего не выбрано) -->
        <label class="container"
          >Диафрагма
          <input type="radio" name="direction" value="mode1" id="mode1" />
          <span class="checkmark"></span>
        </label>
        <label class="container"
          >ZOOM
          <input type="radio" name="direction" value="mode2" />
          <span class="checkmark"></span>
        </label>
        <label class="container"
          >Фокус
          <input type="radio" name="direction" value="mode3" />
          <span class="checkmark"></span>
        </label>
        <label class="container"
          >Зеркало
          <input type="radio" name="direction" value="mode4" />
          <span class="checkmark"></span>
        </label>
        <label class="container"
          >Передний отрезок
          <input type="radio" name="direction" value="frontSeg" />
          <span class="checkmark"></span>
        </label>
        <label class="container"
          >Задний отрезок
          <input type="radio" name="direction" value="backSeg" />
          <span class="checkmark"></span>
        </label>
        <!-- Отправка формы на сервер -->
        <input type="submit" value="Выбрать" style="height: 50px" />
      </form>
      <br />
      <!-- Форма для настройки положений (передний и задний отрезки) -->
      <form action="/" method="POST">
        <p>Статус переднего отрезка: <strong>%FRONT%</strong></p>
        <span>Передний отрезок: </span>
        <input type="submit" value="Сохранить" formaction="/frontSegment" style="height: 50px" />
        <input type="submit" value="Удалить" formaction="/DfrontSegment" style="height: 50px" />
        <br /><br />
        <p>Статус заднего отрезка: <strong>%BACK%</strong></p>
        <span>Задний отрезок: </span>
        <input type="submit" value="Сохранить" formaction="/backSegment" style="height: 50px" />
        <input type="submit" value="Удалить" formaction="/DbackSegment" style="height: 50px" />
        <br /><br />
      </form>
    </div>
  </body>
</html>)rawliteral";

//Управление диафрагмой
const char step_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    <!-- Задаем размеры страницы в соответствии с размером экрана -->
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Пульт управления</title>
    <link rel="icon" href="data:," />
    <style>
      /* Устанавливаем положение всех элементов */
      body {
        font-family: Arial;
        text-align: center;
        margin: 0px auto;
        padding-top: 8px;
      }
      /* Кнопки для упраления скоростью ШД */
      .buttonMove {
        padding: 25px 25px;
        font-size: 30px;
        text-align: absolute;
        outline: none;
        color: black;
        background-color: #dcdcdc;
        border: none;
        border-radius: 30px;
        box-shadow: 0 6px #999;
        margin-left: 5px;
        margin-right: 5px;
      }
      /* Стиль при нажатии */
      .buttonMove:active {
        background-color: #a9a9a9;
        box-shadow: 0 4px #666;
        transform: translateY(2px);
      }
      /* Отменяем выделения при нажатии */
      .noselect {
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
      }
    </style>
  </head>
  <body>
    <div class="noselect">
      <!-- Форма для выбора режима -->
      <form action="/" method="POST">
        <p>Выбранный режим: <strong>%STATE%</strong></p>
        <br /><br /><br /><br /><br /><br /><br /><br /><br /><br />
      </form>
      <!-- Кнопки управления -->
      <button class="buttonMove" ontouchstart="toggleCheckbox('leftOn');" ontouchend="toggleCheckbox('leftOff');">&lt;</button>
      <button class="buttonMove" ontouchstart="toggleCheckbox('rightOn');" ontouchend="toggleCheckbox('rightOff');">></button>
      <!-- Форма для выбора шага  -->
      <form action="/" method="POST">
        <p>Выбранный режим шага: <strong>%STEP%</strong></p>
        <input type="submit" value="    Шаг    " formaction="/step" style="height: 50px" />
        <input type="submit" value="Движение" formaction="/move" style="height: 50px" />
      </form>
      <!-- Форма для возврата в главное меню  -->
      <form action="/" method="GET">
        <br />
        <button style="font-size: 20px; padding: 15px 15px">Меню выбора</button>
        <br />
      </form>
      <script>
        //Отправляем значение нажатой кнопки на сервер (esp8266)
        function toggleCheckbox(x) {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/" + x, true);
          xhr.send();
        }
      </script>
    </div>
  </body>
</html>)rawliteral";

//Управление (лево-право)
const char keyboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    <!-- Задаем размеры страницы в соответствии с размером экрана -->
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Пульт управления</title>
    <link rel="icon" href="data:," />
    <style>
      /* Устанавливаем положение всех элементов */
      body {
        font-family: Arial;
        text-align: center;
        margin: 0px auto;
        padding-top: 8px;
      }
      /* Кнопки для упраления скоростью ШД */
      .buttonMove {
        padding: 25px 25px;
        font-size: 30px;
        text-align: absolute;
        outline: none;
        color: black;
        background-color: #dcdcdc;
        border: none;
        border-radius: 30px;
        box-shadow: 0 6px #999;
        margin-left: 5px;
        margin-right: 5px;
      }
      /* Стиль при нажатии */
      .buttonMove:active {
        background-color: #a9a9a9;
        box-shadow: 0 4px #666;
        transform: translateY(2px);
      }
      /* Отменяем выделения при нажатии */
      .noselect {
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
      }
    </style>
  </head>
  <body>
    <div class="noselect">
      <!-- Форма для выбора режима -->
      <form action="/" method="POST">
        <p>Выбранный режим: <strong>%STATE%</strong></p>
        <br /><br /><br /><br /><br /><br /><br /><br /><br /><br />
      </form>
      <!-- Кнопки управления -->
      <button class="buttonMove" ontouchstart="toggleCheckbox('leftFastOn');" ontouchend="toggleCheckbox('leftFastOff');">&lt;&lt;</button>
      <button class="buttonMove" ontouchstart="toggleCheckbox('leftOn');" ontouchend="toggleCheckbox('leftOff');">&lt;</button>
      <button class="buttonMove" ontouchstart="toggleCheckbox('rightOn');" ontouchend="toggleCheckbox('rightOff');">></button>
      <button class="buttonMove" ontouchstart="toggleCheckbox('rightFastOn');" ontouchend="toggleCheckbox('rightFastOff');">>></button>
      <!-- Форма для выбора шага  -->
      <form action="/" method="POST">
        <p>Выбранный режим шага: <strong>%STEP%</strong></p>
        <input type="submit" value="    Шаг    " formaction="/step" style="height: 50px" />
        <input type="submit" value="Движение" formaction="/move" style="height: 50px" />
      </form>
      <!-- Форма для возврата в главное меню  -->
      <form action="/" method="GET">
        <br />
        <button style="font-size: 20px; padding: 15px 15px">Меню выбора</button>
        <br />
      </form>
      <script>
        //Отправляем значение нажатой кнопки на сервер (esp8266)
        function toggleCheckbox(x) {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/" + x, true);
          xhr.send();
        }
      </script>
    </div>
  </body>
</html>)rawliteral";

//Управление (джойстик)
const char mirror_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    <!-- Задаем размеры страницы в соответствии с размером экрана -->
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Пульт управления</title>
    <link rel="icon" href="data:," />
    <style>
      /* Устанавливаем положение всех элементов */
      body {
        font-family: Arial;
        text-align: center;
        margin: 0px auto;
        padding-top: 8px;
      }
      /* Кнопки лево-право  */
      .buttonMirrorLR {
        padding: 10px 40px;
        font-size: 30px;
        text-align: absolute;
        outline: none;
        color: black;
        background-color: #dcdcdc;
        border: none;
        border-radius: 30px;
        box-shadow: 0 6px #999;
        cursor: pointer;
        margin-left: 5px;
        margin-right: 5px;
      }
      /* Кнопки вверх-низ */
      .buttonMirrorUD {
        padding: 30px 20px;
        font-size: 30px;
        text-align: absolute;
        outline: none;
        color: black;
        background-color: #dcdcdc;
        border: none;
        border-radius: 30px;
        box-shadow: 0 6px #999;
        cursor: pointer;
        margin-left: 5px;
        margin-right: 5px;
      }
      /* Стиль при нажатии */
      .buttonMirrorUD:active,
      .buttonMirrorLR:active {
        background-color: #a9a9a9;
        box-shadow: 0 4px #666;
        transform: translateY(2px);
      }
      /* Отменяем выделения при нажатии */
      .noselect {
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
      }
    </style>
  </head>
  <body>
    <div class="noselect">
      <!-- Форма для выбора режима -->
      <form action="/" method="POST">
        <p>Выбранный режим: <strong>%STATE%</strong></p>
        <br /><br /><br /><br /><br />
      </form>
      <!-- Кнопки управления  -->
      <button class="buttonMirrorUD" ontouchstart="toggleCheckboxM('upOn');" ontouchend="toggleCheckboxM('upOff');"><div style="transform: rotate(90deg)">&lt;</div></button><br />
      <button style="position: relative; right: 17px" class="buttonMirrorLR" ontouchstart="toggleCheckboxM('leftOn');" ontouchend="toggleCheckboxM('leftOff');">&lt;</button>
      <button style="position: relative; left: 17px" class="buttonMirrorLR" ontouchstart="toggleCheckboxM('rightOn');" ontouchend="toggleCheckboxM('rightOff');">></button><br />
      <button style="position: relative" class="buttonMirrorUD" ontouchstart="toggleCheckboxM('downOn');" ontouchend="toggleCheckboxM('downOff');"><div style="transform: rotate(90deg)">></div></button><br />
      <!-- Форма для выбора шага  -->
      <form action="/" method="POST">
        <p>Выбранный режим шага: <strong>%STEP%</strong></p>
        <input type="submit" value="    Шаг    " formaction="/step" style="height: 50px" />
        <input type="submit" value="Движение" formaction="/move" style="height: 50px" />
      </form>
      <!-- Форма для возврата в главное меню  -->
      <form action="/" method="GET">
        <br />
        <button style="font-size: 20px; padding: 15px 15px">Меню выбора</button>
        <br />
      </form>
      <script>
        //Отправляем значение нажатой кнопки на сервер (esp8266)
        function toggleCheckboxM(x) {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/M" + x, true);
          xhr.send();
        }
      </script>
    </div>
  </body>
</html>)rawliteral";

//Сообщение об ошибке
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

//Определение текста выбранного режима
void modeState() {
  if (direction == "mode1") modestate = "Диафрагма";
  else if (direction == "mode2") modestate = "ZOOM";
  else if (direction == "mode3") modestate = "Фокус";
  else if (direction == "mode4") modestate = "Зеркало";
  else if (direction == "frontSeg") modestate = "Передний отрезок";
  else if (direction == "backSeg") modestate = "Задний отрезок";
}

//Заменяет заполнители %STATE%, %STEP%, %FRONT% и %BACK% на нужный текст
String processor(const String &var) {
  modeState();
  if (var == "STATE") return String(modestate);
  if (var == "FRONT") {
    if (digitalRead(RX)) FRONT = "Сохранено";
    else FRONT = "Удалено";
    return String(FRONT);
  }
  if (var == "BACK") {
    if (digitalRead(TX)) BACK = "Сохранено";
    else BACK = "Удалено";
    return String(BACK);
  }
  if (var == "STEP") {
    if (digitalRead(D0)) STEP = "Шаг";
    else STEP = "Движение";
    return String(STEP);
  }
}

void setup() {
  WiFi.mode(WIFI_AP);           //Устанавливаем режим точки доступа
  WiFi.softAP(ssid, password);  //Конфигурируем точку доступа
  pinMode(RX, OUTPUT);
  digitalWrite(RX, 0);
  pinMode(TX, OUTPUT);
  digitalWrite(TX, 0);
  pinMode(D0, OUTPUT);
  digitalWrite(D0, 0);
  pinMode(D1, OUTPUT);
  digitalWrite(D1, 0);
  pinMode(D2, OUTPUT);
  digitalWrite(D2, 0);
  pinMode(D3, OUTPUT);
  digitalWrite(D3, 0);
  pinMode(D4, OUTPUT);
  digitalWrite(D4, 0);
  pinMode(D5, OUTPUT);
  digitalWrite(D5, 0);
  pinMode(D6, OUTPUT);
  digitalWrite(D6, 0);
  pinMode(D7, OUTPUT);
  digitalWrite(D7, 0);
  pinMode(D8, OUTPUT);
  digitalWrite(D8, 0);

  //Главная страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", main_html, processor);
  });

  //Получаем запросы от клиента и конфигурируем пины
  //Кнопки управления ШД (лево-право)
  server.on("/leftFastOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D1, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/leftFastOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D1, 0);
    request->send(200, "text/plain", "ok");
  });
  server.on("/leftOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D2, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/leftOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D2, 0);
    request->send(200, "text/plain", "ok");
  });
  server.on("/rightOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D3, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/rightOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D3, 0);
    request->send(200, "text/plain", "ok");
  });
  server.on("/rightFastOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D4, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/rightFastOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D4, 0);
    request->send(200, "text/plain", "ok");
  });

  //Кнопки управления ШД (джойстик)
  server.on("/MupOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D1, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/MupOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D1, 0);
    request->send(200, "text/plain", "ok");
  });
  server.on("/MrightOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D2, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/MrightOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D2, 0);
    request->send(200, "text/plain", "ok");
  });
  server.on("/MdownOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D4, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/MdownOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D4, 0);
    request->send(200, "text/plain", "ok");
  });
  server.on("/MleftOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D3, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/MleftOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D3, 0);
    request->send(200, "text/plain", "ok");
  });

  //Кнопки выбора шага/движения
  server.on("/step", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(D0, 1);
    if (direction == "mode4") request->send_P(200, "text/html", mirror_html, processor);
    else request->send_P(200, "text/html", step_html, processor);
  });
  server.on("/move", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(D0, 0);
    if (direction == "mode4") request->send_P(200, "text/html", mirror_html, processor);
    else if (direction == "mode1") request->send_P(200, "text/html", step_html, processor);
    else request->send_P(200, "text/html", keyboard_html, processor);
  });

  //Кнопки для сохранения положений
  server.on("/frontSegment", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(RX, 1);
    request->send_P(200, "text/html", main_html, processor);
  });
  server.on("/backSegment", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(TX, 1);
    request->send_P(200, "text/html", main_html, processor);
  });

  //Кнопки для удаления сохраненных положений
  server.on("/DfrontSegment", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(RX, 0);
    request->send_P(200, "text/html", main_html, processor);
  });
  server.on("/DbackSegment", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(TX, 0);
    request->send_P(200, "text/html", main_html, processor);
  });

  //Устанавливаем нужную страницу в зависимости от выбранного режима
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isPost()) {
        if (p->name() == PARAM_INPUT_1) {
          direction = p->value().c_str();
        }
      }
    }
    if (direction == "mode1") request->send_P(200, "text/html", step_html, processor);
    else if ((direction == "mode2" || direction == "mode3") && (digitalRead(D0) == 1)) request->send_P(200, "text/html", step_html, processor);
    else if (direction == "mode2" || direction == "mode3") request->send_P(200, "text/html", keyboard_html, processor);
    else if (direction == "mode4") request->send_P(200, "text/html", mirror_html, processor);
    else request->send_P(200, "text/html", main_html, processor);
  });

  server.onNotFound(notFound);  //Если адрес не найден, выводим сообщение об ошибке
  server.begin();               //Запускаем сервер
}

void loop() {
  //Конфигурируем пины в зависимости от выбранного режима
  if (direction == "mode1") {
    digitalWrite(D5, 0);
    digitalWrite(D6, 0);
    digitalWrite(D7, 1);
  } else if (direction == "mode2") {
    digitalWrite(D5, 0);
    digitalWrite(D6, 1);
    digitalWrite(D7, 0);
  } else if (direction == "mode3") {
    digitalWrite(D5, 0);
    digitalWrite(D6, 1);
    digitalWrite(D7, 1);
  } else if (direction == "mode4") {
    digitalWrite(D5, 1);
    digitalWrite(D6, 0);
    digitalWrite(D7, 0);
  } else if (direction == "frontSeg") {
    digitalWrite(D5, 1);
    digitalWrite(D6, 0);
    digitalWrite(D7, 1);
  } else if (direction == "backSeg") {
    digitalWrite(D5, 1);
    digitalWrite(D6, 1);
    digitalWrite(D7, 0);
  }
}