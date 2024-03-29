# Oblast IoT platform
Oblast is a lightweight IoT platform following the publish/subscribe design pattern. The architecture consists of
 - a central server - responsible of collecting and distributing the updates and
 - a list of clients. Each client can publish or subscribe (or both) to a named
 variable.

Two client API are provided:
 - a Python3 module to write your own custom publisher and subscriber (e.g on a Raspberry PI)
 - a C++ library to write (embedded) clients (e.g. on an Arduino development
board)

## Automatic network discovery:
Once the server is started, it listens to UDP port `1292` and TCP port `1291` (don't forget to create a firewall rule for them). The UDP socket is used for the client to discover the server's IP address using a multicast query. Once the server is located, the clients are instructed to connect using a TCP channel.

## Global variable names:
Each publisher first subscribe to a given variable name. These names are unique for the whole platform. The value can then be manipulated at the programmer's convenience and updates are automatically sent to the server who propagates them to the other subscribers.

## subscribe to variables
A standard client would register to a set of specific variables using a `subscribe` call. Each variable is indentified by a system-wide name.

Oblast API provides also the ability for a client to subscribe to all existing (and future) variables. This is typically useful for having an overview of the state of the system and possibly to store data in a database.

## How to start the server:
To compile the server, go to the `server/` folder and use `make`:
```
cd server;
make;
```
Open the necessary ports using `ufw`:
```
sudo ufw allow 1291:1292/udp
```
Finally run the server:
```
./bin/oblast
```

## How to write a client:
To see the full example, open `clientAPI/python/producer.py`. Here is a step by
step walk-through:

First the client needs to create a connection using an `OblastConnection` object. Then it needs to use that connection to `subscribe` to a given variable with a suitable name (in the following examle, called `"foo_var"`).
Finally it periodically updates the variable value - maybe reading from a sensor - using the `set` function. The client should also yield its execution and let the connection handle the variable updates (`handle_updates()` below).
```
# simplified extract from clientAPI/python/producer.py:
with OblastConnection() as connection:
    foo_var = connection.subscribe("foo_var", reactor)
    while True:
        connection.handle_updates()
        foo_var.set("42")
        time.sleep(1)
```

### NodeMCU / ESP8266 demo:
An analogous implementation for Arduino (or NodeMCU) can be found in the `clientAPI/nodeMCU/` folder. In particular, `clientAPI/nodeMCU/dht22/dht22.ino` illustrates how to use an ESP8266 to measure temperature / humidity data and to publish them to Oblast infrastructure.

## Setting a service on Raspberrypi
Write the following file (as root) into `/etc/systemd/system/oblast.service`:
```
[Unit]
Description=Oblast service

[Service]
User=pi
WorkingDirectory=/home/pi/Documents/Oblast/server
ExecStart=/home/pi/Documents/Oblast/server/bin/oblast
Restart=Always
RestartSec=3

[Install]
WantedBy=multi-user.target                         
```
Restart the daemon (`systemctl daemon-reload`) and then start the service `systemctl start oblast.service`

### Logs of Oblast:
To see the logs, use 
```
sudo journalctl -u oblast
```
