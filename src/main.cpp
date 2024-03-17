#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define UD_PIN D3
#define CS_SERIAL_PIN D4
#define CS_CLOCK_PIN D2

#define NUMBER_OF_LIGHTS 2u

#define DOWN 0u
#define UP 1u

const char *ssid = "esp8266";
const char *password = "12345678";

uint8_t lightLevel[NUMBER_OF_LIGHTS] = {0u};

AsyncWebServer server(80);
AsyncWebSocket ws("/websocket");

/**
 * @brief Move wiper position for selected light
 * 
 * @param light  light ID, must be < NUMBER_OF_LIGHTS
 * @param upDown 0 for down 1 for up
 * @param step   number of steps
 */
void LightControl(uint8_t light, bool upDown, uint8_t step)
{

  for (uint8_t i = 0; i < NUMBER_OF_LIGHTS; i++)
  {
    /* set chip select pin for wanted light */
    digitalWrite(CS_SERIAL_PIN, HIGH);
    /* shift data out with clock cycle */
    delayMicroseconds(5u);
    digitalWrite(CS_CLOCK_PIN, 1u);
    delayMicroseconds(5u);
    digitalWrite(CS_CLOCK_PIN, 0u);
    delayMicroseconds(5u);
  }

  digitalWrite(UD_PIN, !upDown);
  delayMicroseconds(5u);

  for (uint8_t i = 0; i < NUMBER_OF_LIGHTS; i++)
  {
    /* set chip select pin for wanted light */
    digitalWrite(CS_SERIAL_PIN, !((NUMBER_OF_LIGHTS - 1) - light == i));
    /* shift data out with clock cycle */
    delayMicroseconds(5u);
    digitalWrite(CS_CLOCK_PIN, 1u);
    delayMicroseconds(5u);
    digitalWrite(CS_CLOCK_PIN, 0u);
    delayMicroseconds(5u);
  }

  for (uint8_t i = 0; i < step; i++)
  {
    digitalWrite(UD_PIN, upDown);
    delayMicroseconds(5u);
    digitalWrite(UD_PIN, !upDown);
    delayMicroseconds(5u);
  }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.println("WebSocket client connected");
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.println("WebSocket client disconnected");
  }
  else if (type == WS_EVT_DATA)
  {
    // Data received from a client
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_BINARY)
    {
      if (data[0] < NUMBER_OF_LIGHTS)
      {
        lightLevel[data[0]] = (uint8_t)data[1];
      }
    }
  }
}

extern const char *indexHtml;

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting...");

  pinMode(UD_PIN, OUTPUT);
  pinMode(CS_SERIAL_PIN, OUTPUT);
  pinMode(CS_CLOCK_PIN, OUTPUT);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", indexHtml);
    response->addHeader("Connection", "close");
    request->send(response); });

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop(void)
{
  static uint8_t lightLevelPrevious[NUMBER_OF_LIGHTS] = {0u};

  for (uint8_t i = 0u; i < NUMBER_OF_LIGHTS; i++)
  {
    if (lightLevel[i] != lightLevelPrevious[i])
    {
      int16_t diff = (int16_t)lightLevelPrevious[i] - (int16_t)lightLevel[i];
      if (diff > 0)
      {
        LightControl(i, UP, (uint8_t)diff);
      }
      else
      {
        LightControl(i, DOWN, (uint8_t)(diff * (-1)));
      }
      lightLevelPrevious[i] = lightLevel[i];
    }
  }
  delay(10);
}

const char *indexHtml =
"<!DOCTYPE html>\n"
"<html>\n"
"\n"
"<head>\n"
"    <title>Light Control</title>\n"
"</head>\n"
"\n"
"<style>\n"
"    body {\n"
"        background-color: #282828;\n"
"        display: flex;\n"
"        flex-direction: row;\n"
"        justify-content: space-evenly;\n"
"        width: 100vw;\n"
"        margin: auto;\n"
"        margin-top: 500px;\n"
"        overscroll-behavior-y: contain;\n"
"    }\n"
"\n"
"    .wrapper {\n"
"        position: relative;\n"
"        height: 40rem;\n"
"        width: 6rem;\n"
"\n"
"        &::before,\n"
"        &::after {\n"
"            display: block;\n"
"            position: absolute;\n"
"            z-index: 99;\n"
"            color: #533c3c;\n"
"            width: 100%;\n"
"            text-align: center;\n"
"            font-size: 3rem;\n"
"            line-height: 1;\n"
"            padding: 1.5rem 0;\n"
"            pointer-events: none;\n"
"        }\n"
"\n"
"        &::before {\n"
"            content: \"+\";\n"
"        }\n"
"\n"
"        &::after {\n"
"            content: \"-\";\n"
"            bottom: 0;\n"
"        }\n"
"    }\n"
"\n"
"    input[type=\"range\"] {\n"
"        overscroll-behavior-y: contain;\n"
"        -webkit-appearance: none;\n"
"        background-color: rgb(150, 150, 150);\n"
"        position: absolute;\n"
"        top: 50%;\n"
"        left: 50%;\n"
"        margin: 0;\n"
"        padding: 0;\n"
"        width: 40rem;\n"
"        height: 7rem;\n"
"        transform: translate(-50%, -50%) rotate(90deg);\n"
"        border-radius: 1rem;\n"
"        overflow: hidden;\n"
"        cursor: row-resize;\n"
"\n"
"        &::-webkit-slider-thumb {\n"
"            -webkit-appearance: none;\n"
"            width: 0;\n"
"            box-shadow: -20rem 0 0 20rem rgb(116, 116, 116);\n"
"        }\n"
"    }\n"
"</style>\n"
"\n"
"<body>\n"
"    <div class=\"wrapper\"><input type=\"range\" min=\"0\" max=\"63\" value=\"32\"\n"
"            oninput=\"UpdateLightIntensity(0, this.value)\" /></div>\n"
"    <div class=\"wrapper\"><input type=\"range\" min=\"0\" max=\"63\" value=\"32\"\n"
"            oninput=\"UpdateLightIntensity(1, this.value)\" /></div>\n"
"</body>\n"
"\n"
"<script>\n"
"\n"
"    var ws = new WebSocket(\"ws://\" + window.location.hostname + \"/websocket\");\n"
"\n"
"    function UpdateLightIntensity(lightId, value) {\n"
"        (ws.readyState === ws.OPEN)\n"
"        {\n"
"            console.log(value)\n"
"            var binaryData = new Uint8Array([lightId, parseInt(value)]);\n"
"            ws.send(binaryData);\n"
"        }\n"
"    }\n"
"\n"
"</script>\n"
"\n"
"</html>";