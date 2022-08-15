#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//Ввод данных сети
const char *ssid = "ESP";
const char *password = "123456789";
const char *PARAM_INPUT_1 = "direction";

String direction;
String modestate;

//HTML страница
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <!-- Задаем размеры страницы в соответствии с размером экрана -->
    <meta name="viewport" content="width=device-width, initial-scale=1"> 
    <title>ПДУ</title>
    <style>
    /* Устанавливаем положение всех элементов */
    body {font-family: Arial; text-align: center; margin:0px auto; padding-top: 8px;} 
    /* Кнопки для упраления скоростью ШД */
    .buttonMove
      {
        padding: 25px 25px;
        font-size: 30px;
        text-align: absolute;
        outline: none;
        color: black;
        background-color: #DCDCDC;
        border: none;
        border-radius: 30px;
        box-shadow: 0 6px #999;
        margin-left: 5px;
        margin-right: 5px;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(255,255,255,0);
        -webkit-tap-highlight-color: transparent;
      } 
      .button
      {
        padding: 17px 17px;
        font-size: 11px;
        text-align: absolute;
        outline: none;
        color: black;
        background-color: #DCDCDC;
        border: none;
        border-radius: 30px;
        box-shadow: 0 6px #999;
        margin-left: 5px;
        margin-right: 5px;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(255,255,255,0);
        -webkit-tap-highlight-color: transparent;
      } 
    .buttonMirrorLR 
      {
        padding: 10px 40px;
        font-size: 30px;
        text-align: absolute;
        outline: none;
        color: black;
        background-color: #DCDCDC;
        border: none;
        border-radius: 30px;
        box-shadow: 0 6px #999;
        cursor: pointer;
        margin-left: 5px;
        margin-right: 5px;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(255,255,255,0);
        -webkit-tap-highlight-color: transparent;
      } 
    .buttonMirrorUD 
      {
        padding: 30px 20px;
        font-size: 30px;
        text-align: absolute;
        outline: none;
        color: black;
        background-color: #DCDCDC;
        border: none;
        border-radius: 30px;
        box-shadow: 0 6px #999;
        cursor: pointer;
        margin-left: 5px;
        margin-right: 5px;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(255,255,255,0);
        -webkit-tap-highlight-color: transparent;
      } 
    .buttonMove:active, .buttonMirrorUD:active, .buttonMirrorLR:active 
      {
        background-color: #A9A9A9;
        box-shadow: 0 4px #666;
        transform: translateY(2px);
      }
    .button:active 
      {
        background-color: #A9A9A9;
        box-shadow: 0 2px #666;
        transform: translateY(1px);
      }

      .button:disabled
      {
        background-color:  #999;
      }

    .noselect
    {
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(255,255,255,0);
        -webkit-tap-highlight-color: transparent;
    }
    </style>
  </head>
  <body>
  <div class="noselect">
    <form action="/" method="POST">
    <p> Выбранный режим: <strong>%STATE%</strong></p>
  </div>
    <style>
        /* Положение радио кнопок (текст) */
    .container 
      {
        display: block;
        position: relative;
        padding-left: 35px;
        margin-bottom: 20px;
        cursor: pointer;
        font-size: 22px;
        -webkit-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
      }
    /* Скрываем переключатели по умолчанию */
    .container input 
      {
        position: center;
        opacity: 0;
        cursor: pointer;
      }
    /* Создание пользовательского переключателя */
    .checkmark 
      {
        position: absolute;
        top: 0;
        left: 70px;
        height: 25px;
        width: 25px;
        background-color: #eee;
        border-radius: 50%;
      }
    /* Когда переключатель установлен, добавляем синий фон */
    .container input:checked ~ .checkmark 
      {
        background-color: #2196F3;
      }
    </style>





    
    <!-- Радио кнопки (выбор кнопки отправляется запросом на сервер, по умолчанию ничего не выбрано) -->
      <label class="container">Диафрагма
        <input type="radio" name="direction" value="mode1" id="mode1">
        <span class="checkmark"></span>
      </label>
      <label class="container">ZOOM
        <input type="radio" name="direction" value="mode2">
        <span class="checkmark"></span>
      </label>
      <label class="container">Фокус
        <input type="radio" name="direction" value="mode3">
        <span class="checkmark"></span> 
      </label>
      <label class="container">Зеркало 
        <input type="radio" name="direction" value="mode4">
        <span class="checkmark"></span>
      </label>
      <label class="container">Передний отрезок 
        <input type="radio" name="direction" value="frontSeg">
        <span class="checkmark"></span>
      </label>
      <label class="container">Задний отрезок 
        <input type="radio" name="direction" value="backSeg">
        <span class="checkmark"></span>
      </label>

    <div class="noselect" >
    <input type="submit" value="Выбрать" style="height: 50px"><br><br>
    </form>

    <button  class="button" ontouchstart="pressedButton('step');">Шаг</button>
    <button  class="button" ontouchstart="pressedButton('move');" ontouchend="pressedButton('moveEnd');">Движение</button>
    <script>
      //Отправляем значение нажатой кнопки на сервер (esp8266)
      function pressedButton(x) 
      {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/" + x, true);
        xhr.send();
      }
    </script>
    <br><br>

    <!-- Kнопки управления ШД (left, lefrFast, right, rightFast) -->
    <button class="buttonMove" ontouchstart="toggleCheckbox('leftFastOn');" ontouchend="toggleCheckbox('leftFastOff');">&lt;&lt;</button>
    <button class="buttonMove" ontouchstart="toggleCheckbox('leftOn');" ontouchend="toggleCheckbox('leftOff');">&lt;</button>
    <button class="buttonMove" ontouchstart="toggleCheckbox('rightFastOn');" ontouchend="toggleCheckbox('rightFastOff');">></button>
    <button class="buttonMove" ontouchstart="toggleCheckbox('rightOn');"  ontouchend="toggleCheckbox('rightOff');">>></button>
    </div>
    <script>
      //Отправляем значение нажатой кнопки на сервер (esp8266)
      function toggleCheckbox(x) 
      {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/" + x, true);
        xhr.send();
      }
    </script>
<div class="noselect">
    <button class="buttonMirrorUD" ontouchstart="toggleCheckboxM('upOn');" ontouchend="toggleCheckboxM('upOff');"><div style="transform: rotate(90deg)">&lt;<div\></button>
    <button style="position: absolute; right:50px; top:650px"class="buttonMirrorLR" ontouchstart="toggleCheckboxM('rightOn');" ontouchend="toggleCheckboxM('rightOff');">></button>
    <button style="position: absolute; left: 153px; top: 680px" class="buttonMirrorUD" ontouchstart="toggleCheckboxM('downOn');" ontouchend="toggleCheckboxM('downOff');"><div style="transform: rotate(90deg)">><div\></button>
    <button style="position: absolute; left:50px; top:650px"class="buttonMirrorLR" ontouchstart="toggleCheckboxM('leftOn');" ontouchend="toggleCheckboxM('leftOff');">&lt;</button>
    
<br><br><br><br><br><br><br><br>

      <button  class="button" ontouchstart="pressedButton('frontSegment');">Сохранить передний отрезок</button>
      <br><br>
      <button  class="button" ontouchstart="pressedButton('backSegment');">Сохранить задний отрезок</button>
      <br><br>
      <button  class="button" ontouchstart="pressedButton('DfrontSegment');">Удалить передний отрезок</button>
      <br><br>
      <button  class="button" ontouchstart="pressedButton('DbackSegment');">Удалить задний отрезок</button>
      <br><br>
</div>
    <script>
      //Отправляем значение нажатой кнопки на сервер (esp8266)
      function toggleCheckboxM(x) 
      {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/M" + x, true);
        xhr.send();
      }
    </script>
  </body>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void modeState() {
  if (direction == "mode1") modestate = "Диафрагма";
  else if (direction == "mode2") modestate = "ZOOM";
  else if (direction == "mode3") modestate = "Фокус";
  else if (direction == "mode4") modestate = "Зеркало";
  else if (direction == "frontSeg") modestate = "Передний отрезок";
  else if (direction == "backSeg") modestate = "Задний отрезок";
}

String processor(const String &var) {
  modeState();
  if (var == "STATE") return String(modestate);
  return String();
}

AsyncWebServer server(80);

void setup() {
  WiFi.softAP(ssid, password);
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
  //Стартовая страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  //Получаем запросы от клиента и конфигурируем пины
  //Кнопки управления ШД
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
    digitalWrite(D4, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/rightOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D4, 0);
    request->send(200, "text/plain", "ok");
  });
  server.on("/rightFastOn", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D3, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/rightFastOff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D3, 0);
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
  //Кнопки выбора шага
  server.on("/step", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D0, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/move", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(D0, 0);
    request->send(200, "text/plain", "ok");
  });
  //Кнопки для сохранения положений
  server.on("/frontSegment", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RX, 1);
    request->send(200, "text/plain", "ok");
  });
  server.on("/backSegment", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(TX, 1);
    request->send(200, "text/plain", "ok");
  });
  //Кнопки для удаления сохраненных положений
  server.on("/DfrontSegment", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RX, 0);
    request->send(200, "text/plain", "ok");
  });
  server.on("/DbackSegment", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(TX, 0);
    request->send(200, "text/plain", "ok");
  });
  //
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
  request->send_P(200, "text/html", index_html, processor);  //Тут переходы между массивыми страниц через иф
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
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