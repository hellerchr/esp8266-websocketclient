#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <WiFiClient.h>

class WebSocketClient {
public:

	WebSocketClient(bool secure = false);

	~WebSocketClient();

	bool connect(String host, String path, int port);

	bool isConnected();

	void disconnect();

	void send(const String& str);

	bool getMessage(String& message);

	void setAuthorizationHeader(String header);

private:
	int timedRead();

	String generateKey();

	WiFiClient *client;

	String authorizationHeader = "";

};

#endif //WEBSOCKETCLIENT_H
