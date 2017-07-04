/*
 * qmqtt_client.cpp - qmqtt client
 *
 * Copyright (c) 2013  Ery Lee <ery.lee at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of mqttc nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "qmqtt_client.h"
#include "qmqtt_client_p.h"

#include "stdio.h"
#include "stdlib.h"

namespace QMQTT {

Client::Client(const QString & host, quint32 port, QObject * parent /* =0 */)
    :internalState(STATE_DISCONNECTED), pPrivateClient(new ClientPrivate(this))
{
    pPrivateClient->init(host, port, parent);
}

Client::~Client()
{
    //Since we're not using std::nothrow we'll get an exception if new fails.
    delete pPrivateClient;
}

State Client::state() const {
    return internalState;
}

/*----------------------------------------------------------------
 * Get/Set Property
 ----------------------------------------------------------------*/
QString Client::host() const
{
    return pPrivateClient->host;
}

void Client::setHost(const QString & host)
{
    pPrivateClient->host = host;
}

quint32 Client::port() const
{
    return pPrivateClient->port;
}

void Client::setPort(quint32 port)
{
    pPrivateClient->port = port;
}

QString Client::clientId() const
{
    return pPrivateClient->clientId;
}

void Client::setClientId(const QString &clientId)
{
    pPrivateClient->clientId = clientId;
}

QString Client::username() const
{
    return pPrivateClient->username;
}

void Client::setUsername(const QString & username)
{
    pPrivateClient->username = username;
}

QString Client::password() const
{
    return pPrivateClient->password;
}

void Client::setPassword(const QString & password)
{
    pPrivateClient->password = password;
}

int Client::keepalive()
{
    return pPrivateClient->keepalive;
}

void Client::setKeepAlive(int keepalive)
{
    pPrivateClient->keepalive = keepalive;
}

bool Client::cleansess()
{
    return pPrivateClient->cleansess;
}

void Client::setCleansess(bool cleansess)
{
    pPrivateClient->cleansess = cleansess;
}

bool Client::autoReconnect() const
{
    return pPrivateClient->network->autoReconnect();
}

void Client::setAutoReconnect(bool value)
{
    pPrivateClient->network->setAutoReconnect(value);
}

Will *Client::will()
{
    return pPrivateClient->will;
}

void Client::setWill(Will *will)
{
    pPrivateClient->will = will;
}

bool Client::isConnected()
{
    return pPrivateClient->network->isConnected();
}


/*----------------------------------------------------------------
 * MQTT Command
 ----------------------------------------------------------------*/
void Client::connect()
{
    pPrivateClient->sockConnect();
    internalState = STATE_CONNECTING;
}

void Client::onConnected()
{
    pPrivateClient->sendConnect();
    pPrivateClient->startKeepalive();
    internalState = STATE_CONNECTED;
    emit connected();
}

quint16 Client::publish(quint16 id, const QString& topic, const QByteArray& payload, quint8 qos, bool retain, bool dup)
{
    Message message(id, topic, payload, qos, retain, dup);
    return this->publish(message);
}

quint16 Client::publish(QMQTT::Message& message)
{
    quint16 msgid = pPrivateClient->sendPublish(message);
    emit published(message);
    return msgid;
}

void Client::puback(quint8 type, quint16 msgid)
{
    pPrivateClient->sendPuback(type, msgid);
    emit pubacked(type, msgid);
}

quint16 Client::subscribe(const QString &topic, quint8 qos)
{
    quint16 msgid = pPrivateClient->sendSubscribe(topic, qos);
    emit subscribed(topic);
    return msgid;
}

void Client::unsubscribe(const QString &topic)
{
    pPrivateClient->sendUnsubscribe(topic);
    emit unsubscribed(topic);
}

void Client::ping()
{
    pPrivateClient->sendPing();
}

void Client::disconnect()
{
    pPrivateClient->disconnect();
}

void Client::onDisconnected()
{
    pPrivateClient->stopKeepalive();
    internalState = STATE_DISCONNECTED;
    emit disconnected();
}

//---------------------------------------------
//---------------------------------------------
void Client::onReceived(Frame &frame)
{
    quint8 qos = 0;
    bool retain, dup;
    QString topic;
    quint16 mid = 0;
    quint8 header = frame.header();
    quint8 type = GETTYPE(header);
    Message message;

    switch(type) {
    case CONNACK:
        //skip reserved
        try {
            frame.readChar();
            handleConnack(frame.readChar());
        } catch (std::out_of_range& e) {
            // partial frame received, just drop it for now
            fprintf(stderr, "connack dropped frame: %s\n", e.what());
        }
        break;
    case PUBLISH:
        try {
            qos = GETQOS(header);
            retain = GETRETAIN(header);
            dup = GETDUP(header);
            topic = frame.readString();
            if(qos > MQTT_QOS0) {
                mid = frame.readInt();
            }
            message.setId(mid);
            message.setTopic(topic);
            message.setPayload(frame.data());
            message.setQos(qos);
            message.setRetain(retain);
            message.setDup(dup);
            handlePublish(message);
        } catch (std::out_of_range& e) {
            // partial frame received, just drop it for now
            fprintf(stderr, "publish dropped frame: %s\n", e.what());
        }
        break;
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP:
        try {
            mid = frame.readInt();
            handlePuback(type, mid);
        } catch(std::out_of_range& e) {
            // partial frame received, just drop it for now
            fprintf(stderr, "puback dropped frame: %s\n", e.what());
        }
        break;
    case SUBACK:
        try {
            mid = frame.readInt();
            qos = frame.readChar();
            emit subacked(mid, qos);
        } catch (std::out_of_range& e) {
            // partial frame received, just drop it for now
            fprintf(stderr, "suback dropped frame: %s\n", e.what());
        }
        break;
    case UNSUBACK:
        emit unsubacked(mid);
        break;
    case PINGRESP:
        emit pong();
        break;
    default:
        break;
    }
}

void Client::handleConnack(quint8 ack)
{
    emit connacked(ack);
}

void Client::handlePublish(Message & message)
{
    if(message.qos() == MQTT_QOS1) {
        pPrivateClient->sendPuback(PUBACK, message.id());
    } else if(message.qos() == MQTT_QOS2) {
        pPrivateClient->sendPuback(PUBREC, message.id());
    }
    emit received(message);
    emit received(message.id(), message.topic(), message.payload(), message.qos(), message.retain(), message.dup());
}

void Client::handlePuback(quint8 type, quint16 msgid)
{
    if(type == PUBREC) {
        pPrivateClient->sendPuback(PUBREL, msgid);
    } else if (type == PUBREL) {
        pPrivateClient->sendPuback(PUBCOMP, msgid);
    }
    emit pubacked(type, msgid);
}

} // namespace QMQTT

