# esp8266-websocketclient
ESP8266 WebsocketClient

A simple Websocket Client for the ESP8266. Supporting ws:// & wss://.

Example:

```
#include <Arduino.h>
#include "WebSocketClient.h"
#include "ESP8266WiFi.h"

WebSocketClient ws(true);

void setup() {
	Serial.begin(115200);
	WiFi.begin("MyWifi", "secret");

	Serial.print("Connecting");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
}

void loop() {
	if (!ws.isConnected()) {
		ws.connect("echo.websocket.org", "/", 443);
	} else {
		ws.send("hello");

		String msg;
		if (ws.getMessage(msg)) {
			Serial.println(msg);
		}
	}
	delay(500);
}
```
