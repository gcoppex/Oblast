#ifndef _OBLAST_CLIENT_H
#define _OBLAST_CLIENT_H


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <map>

#ifdef WiFi_h
    #define _SEND_FUNCTION_ print
#else
    #define _SEND_FUNCTION_ write
#endif


typedef void (*OBLAST_CALLBACK)(String const&, String const&);

class OblastVar;

class OblastConnection {
public:
    bool connect(bool incrementalRetryDelay=0);
    static OblastConnection& getInstance();
    bool isConnected();
    bool subscribe(OblastVar& var);
    bool handleUpdates();
    bool sendMessage(String const& action, String const& key, String const& value);
private:
    OblastConnection(){}
    static OblastConnection instance;
    WiFiClient client;
    std::map<String, OblastVar*> keyValueStore;
    short int connectionRetryDelay = 500;
};

class OblastVar {
public:
    OblastVar(String const& name, OBLAST_CALLBACK callback):
        connection(), name(name), value(""), callback(callback) { };
    OblastVar(String const& name):
        connection(), name(name), value(""), callback() {};

    bool setValue(String const& value, bool updateServer=true);
    String const& getValue() const;
    String const& getName() const;
    void setConnection(OblastConnection* connection);
    void fireCallback(String const& key, String const& value);
private:
    OblastConnection* connection;
    String name;
    String value;
    OBLAST_CALLBACK callback;
};

#endif
