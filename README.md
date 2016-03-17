QMQTT
=====

mqtt client for Qt

QML Usage
=====
	//in main.cpp
	#include "qmqtt.h"

	int main(int argc, char* argv[]) {
		qmlRegisterType<QMQTT::Client>("os.interaktionsbyran.mqtt", 1, 0, "MqttClient");
		qmlRegisterType<QMQTT::Will>("os.interaktionsbyran.mqtt", 1, 0, "MqttWill");

		//...
	}


	//in qml
	MqttClient {
		id: mqttclient
		host: "localhost"
		port: 1883

		will: MqttWill {
			topic: "will"
			message: "avenge me!"
			retain: true
		}

		onConnected: {
			mqttclient.subscribe("my/topic", 0);
			mqttclient.publish(0, "my/topic/greetings", "hello"); //Regular mesasge
			mqttclient.publish(0, "my/topic/derp", "herp", 0, true); //Retained message
		}

		onReceived: {
			console.log(topic, payload, qos, retain, dup);
		}

		Component.onCompleted {
			mqttclient.connect();
		}
	}


C++ Usage
=====

	#include "qmqtt.h"

	QMQTT::Client *client = new QMQTT::Client("localhost", 1883);

	client->setClientId("clientId");

	client->setUsername("user");

	client->setPassword("password");

	client->connect();


Slots
=====

	void connect();

	quint16 publish(Message &message);

	void puback(quint8 type, quint16 msgid);

	quint16 subscribe(const QString &topic, quint8 qos);

	void unsubscribe(const QString &topic);

	void ping();

	void disconnect();

Signals
=======

	void connected();

	void error(QAbstractSocket::SocketError);

	void connacked(quint8 ack);

	void published(Message &message);

	void pubacked(quint8 type, quint16 msgid);

	void received(const Message &message);

	void subscribed(const QString &topic);

	void subacked(quint16 mid, quint8 qos);

	void unsubscribed(const QString &topic);

	void unsubacked(quint16 mid);

	void pong();

	void disconnected();


