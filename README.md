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

## Subscribe to all variables
A standard client would register to a set of specific variables. Oblast API provides also the ability for a client to subscribe to all existing (and future) variables. This is typically useful for having an overview of the state of the system and possibly to store data in a database.
