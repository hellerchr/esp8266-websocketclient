//#define DEBUG

#include "WebSocketClient.h"
#include <WiFiClientSecure.h>

#define WS_FIN            0x80
#define WS_OPCODE_TEXT    0x01
#define WS_OPCODE_BINARY  0x02

#define WS_MASK           0x80
#define WS_SIZE16         126

#ifdef DEBUG
#define DEBUG_WS Serial.println
#else
#define DEBUG_WS(MSG)
#endif

WebSocketClient::WebSocketClient(bool secure) {
	if (secure)
		this->client = new WiFiClientSecure;
	else
		this->client = new WiFiClient;
}

WebSocketClient::~WebSocketClient() {
	delete this->client;
}

void WebSocketClient::setAuthorizationHeader(String header) {
	this->authorizationHeader = header;
}

String WebSocketClient::generateKey() {
	String key = "";
	for (int i = 0; i < 22; ++i) {
		int r = random(0, 3);
		if (r == 0)
			key += (char) random(48, 57);
		else if (r == 1)
			key += (char) random(65, 90);
		else if (r == 2)
			key += (char) random(97, 122);
	}
	return key;
}
void WebSocketClient::write(uint8_t data) {
    if (client->connected())
        client->write(data);
}

void WebSocketClient::write(const char *data) {
    if (client->connected())
        client->write(data);
}

bool WebSocketClient::connect(String host, String path, int port) {
    if (!client->connect(host.c_str(), port))
        return false;

	// send handshake
	String handshake = "GET " + path + " HTTP/1.1\r\n"
			"Host: " + host + "\r\n"
			"Connection: Upgrade\r\n"
			"Upgrade: websocket\r\n"
			"Sec-WebSocket-Version: 13\r\n"
			"Sec-WebSocket-Key: " + generateKey() + "==\r\n";

	if (authorizationHeader != "")
		handshake += "Authorization: " + authorizationHeader + "\r\n";

	handshake += "\r\n";

	DEBUG_WS("[WS] sending handshake");
	DEBUG_WS(handshake);

    write(handshake.c_str());

	// success criteria
	bool hasCorrectStatus = false;
	bool isUpgrade = false;
	bool isWebsocket = false;
	bool hasAcceptedKey = false;

	bool endOfResponse = false;

	// handle response headers
	String s;
	while (!endOfResponse && (s = client->readStringUntil('\n')).length() > 0) {
		DEBUG_WS("[WS][RX] " + s);
		// HTTP Status
		if (s.indexOf("HTTP/") != -1) {
			auto status = s.substring(9, 12);
			if (status == "101")
				hasCorrectStatus = true;
			else {
				DEBUG_WS("[WS] wrong status: " + status);
				return false;
			}
		}
		// Headers
		else if (s.indexOf(":") != -1) {
			auto col = s.indexOf(":");
			auto key = s.substring(0, col);
			auto value = s.substring(col + 2, s.length() - 1);

			if (key == "Connection" && (value == "Upgrade" || value == "upgrade"))
				isUpgrade = true;

			else if (key == "Sec-WebSocket-Accept")
				hasAcceptedKey = true;

			else if (key == "Upgrade" && value == "websocket")
				isWebsocket = true;
		}

		else if (s == "\r")
			endOfResponse = true;
	}

	bool success = hasCorrectStatus && isUpgrade && isWebsocket && hasAcceptedKey;

	if (success) {
		DEBUG_WS("[WS] sucessfully connected");
        this->websocketEstablished = true;
    }
	else {
		DEBUG_WS("[WS] could not connect");
        this->disconnect();
	}

	return success;
}

bool WebSocketClient::isConnected() {
	return this->websocketEstablished && client->connected();
}

void WebSocketClient::disconnect() {
	client->stop();
    this->websocketEstablished = false;
}

void WebSocketClient::send(const String& str) {
	DEBUG_WS("[WS] sending: " + str);
	if (!client->connected()) {
		DEBUG_WS("[WS] not connected...");
		return;
	}

	// 1. send fin and type text
	write(WS_FIN | WS_OPCODE_TEXT);

	// 2. send length
	int size = str.length();
	if (size > 125) {
		write(WS_MASK | WS_SIZE16);
		write((uint8_t) (size >> 8));
		write((uint8_t) (size & 0xFF));
	} else {
		write(WS_MASK | (uint8_t) size);
	}

	// 3. send mask
	uint8_t mask[4];
	mask[0] = random(0, 256);
	mask[1] = random(0, 256);
	mask[2] = random(0, 256);
	mask[3] = random(0, 256);

	write(mask[0]);
	write(mask[1]);
	write(mask[2]);
	write(mask[3]);

	//4. send masked data
	for (int i = 0; i < size; ++i) {
		write(str[i] ^ mask[i % 4]);
	}
}

int WebSocketClient::timedRead() {
	while (!client->available()) {
		delay(20);
	}
	return client->read();
}

bool WebSocketClient::getMessage(String& message) {
	if (!client->connected()) {	return false; }

	// 1. read type and fin
	unsigned int msgtype = timedRead();
	if (!client->connected()) {
		DEBUG_WS("Step 1");
		return false;
	}

	// 2. read length and check if masked
	int length = timedRead();
	bool hasMask = false;
	if (length & WS_MASK) {
		hasMask = true;
		length = length & ~WS_MASK;
	}

	if (length == WS_SIZE16) {
		length = timedRead() << 8;
		length |= timedRead();
	}

	// 3. read mask
	if (hasMask) {
		uint8_t mask[4];
		mask[0] = timedRead();
		mask[1] = timedRead();
		mask[2] = timedRead();
		mask[3] = timedRead();

		// 4. read message (masked)
		message = "";
		for (int i = 0; i < length; ++i) {
			message += (char) (timedRead() ^ mask[i % 4]);
		}
	} else {
		// 4. read message (unmasked)
		message = "";
		for (int i = 0; i < length; ++i) {
			message += (char) timedRead();
		}
	}
    
    return true;
}
