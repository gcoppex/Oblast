#include "OblastConnection.h"

bool OblastConnection::connect(bool incrementalRetryDelay/* = 0 */) {
    if(incrementalRetryDelay) {
        delay(this->connectionRetryDelay);
        this->connectionRetryDelay *= 1.25;
        // Bounding the delay to 10 seconds:
        if(this->connectionRetryDelay>10000) {
            this->connectionRetryDelay = 10000;
        }
    }

    this->keyValueStore.clear();
    static const IPAddress multicastAddress(226,1,1,38);
    static const unsigned int multicastPort = 1291;
    static const unsigned int receivingUdpPort = 1292;

    WiFiUDP multicastUdp;
    WiFiUDP standardUdp;

    int res = standardUdp.begin(receivingUdpPort);

    String register_message("oblast_discover");

    int sendingRetryBudget = 5;
    int sendingRetryDelay = 50;
    IPAddress serverIpAddress(0, 0, 0, 0);
    // This loop can take up to 281×5+659 = ~2064 ms max delay time to execute.
    while(!serverIpAddress && sendingRetryBudget>0) {
        sendingRetryBudget--;

        multicastUdp.beginPacket(multicastAddress, multicastPort);
        multicastUdp._SEND_FUNCTION_(register_message.c_str());
        int ret = multicastUdp.endPacket();
        if(ret==0) {
            // try again to send by restarting the loop:
            continue;
        }

        // Incrementally waiting for the server's response
        // (max 10 retries, the cumulated max waiting time is ~281ms):
        int responseReady = 0;
        int retryDelay = 25;
        int retryElapsed = 0;
        while(retryElapsed<250 && !responseReady) {
            if(standardUdp.parsePacket()) {
                responseReady = 1;
            } else {
                delay(retryDelay);
                retryElapsed += retryDelay;
                retryDelay *= 1.25;
            }
        }

        // Reading the response to make sure it has a valid payload:
        if(responseReady) {
            char packet[20];
            int len = standardUdp.read(packet, 20);
            packet[len] = '\0';
            if(strncmp(packet, "oblast_server", 255)==0) {
                serverIpAddress = standardUdp.remoteIP();
            }
        } else {
            delay(sendingRetryDelay);
            sendingRetryDelay *= 1.25;
            // The cumulated maximal retry time is ~659 ms
        }
    }
    if(!serverIpAddress) {
        return false;
    }

    // Connecting to the server, now that his address is known;
    if(this->client.connect(serverIpAddress, 1291)) {
         // resetting the incremental retry delay:
        this->connectionRetryDelay = 500;
        return true;
    }
    return false;
}

OblastConnection& OblastConnection::getInstance() {
    return instance;
}
OblastConnection OblastConnection::instance;

bool OblastConnection::isConnected() {
    return this->client.connected();
}

bool OblastConnection::sendMessage(String const& action, String const& key, String const& value) {
    if(!this->isConnected()) {
         // Unable to send message - not connected?
        return false;
    }
    uint16_t length = action.length() + 1 + key.length();
    if(value.length()>0) {
        length += 1 + value.length();
    }

    char payload[2+length];
    payload[0] = length >> 8;
    payload[1] = length;
    if(value.length()>0) {
        sprintf(payload+2, "%s:%s=%s", action.c_str(), key.c_str(), value.c_str());
    } else {
        sprintf(payload+2, "%s:%s", action.c_str(), key.c_str());
    }
    int n0 = this->client.write(payload, 2+length);
    this->client.flush();
    return true;
}

bool OblastConnection::subscribe(OblastVar& var) {
    if(keyValueStore.count(var.getName())>0) {
        // The variable is already registered... Doing nothing.
        // Variable already registered
        return false;
    }
    if(this->client.connected()) {
        if(this->sendMessage("subscribe", var.getName(), "")) {
            keyValueStore[var.getName()] = &var;
        }
    }
    // Registering the connection to the variable:
    var.setConnection(this);

    return true;
}

String const& OblastVar::getValue() const {
    return this->value;
}
String const& OblastVar::getName() const {
    return this->name;
}

void OblastVar::setCallback(OBLAST_CALLBACK callback) {
    this->callback = callback;
}

void OblastVar::setConnection(OblastConnection* connection) {
    this->connection = connection;
}
bool OblastVar::setValue(String const& value, bool updateServer/* = true*/) {
  this->value = value;
    if(updateServer) {
        if(!this->connection->
            sendMessage("update", this->getName(), this->getValue())
        ) {
            return false;
        }
    }
    return true;
}

void OblastVar::fireCallback(String const& key, String const& value) {
    (*this->callback)(key, value);
}

bool OblastConnection::handleUpdates() {
    if(isConnected()) {
        while(client.available()>0) {
            char receivedPayload[2];
            int n0 = client.readBytes(receivedPayload, 2);
            if(n0==2) {
                unsigned int messageLength = receivedPayload[0] << 16 | receivedPayload[1];
                char messageBuffer[messageLength+1];
                int n1 = client.readBytes(messageBuffer, messageLength);
                messageBuffer[messageLength] = '\0';

                char* action = strtok(messageBuffer, ":");
                char* key = strtok(NULL, "=");
                char* value = strtok(NULL, "");

                if(strncmp(action, "update", 1024)==0
                    && key!=NULL && value!=NULL
                ) {
                    if(keyValueStore[key]) {
                        keyValueStore[key]->setValue(value);
                        keyValueStore[key]->fireCallback(key, value);
                    } else {
                        // We received an update for a var that is not
                        // registered? Let's do nothing...
                    }
                }
            }
        }
        return true;
    } else {
        return false;
    }
}
