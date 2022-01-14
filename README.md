# Oblast IoT platform
Oblast is a lightweight IoT platform following the publish/subscribe design pattern. The architecture consists of
 - a central server - responsible of collecting and distributing the updates and
 - a list of clients. Each client can publish or subscribe (or both) to a named
 variable.

Two client API are provided:
 - a Python3 module to write your own custom publisher and subscriber (e.g on a Raspberry PI)
 - a C++ library to write (embedded) clients (e.g. on an Arduino development
board)

## Autoamtic architecture discovery:
Once the server is started, it listens to UDP port `1292` and TCP port `1291`. The UDP socket is used for the client to discover the server's IP address using a multicast query. Once the server is located, the clients are instructed to connect using a TCP channel.

## Global variable names and subscription:
Each publisher first subscribe to a given variable name. These names are unique for the whole platform. The value can then be manipulated at the programmer's convenience and updates are automatically sent to the server who propagates them to the other subscribers.

The client API provides also the ability for a client to register to all existing (and future) variables. This is typically useful for having an overview of the state of the system and possibly to store data in a database.
