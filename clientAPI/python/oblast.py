#!/usr/bin/env python3

import socket
import struct

class OblastVar:

    def __init__(self, oblast_connection, name, callback=None):
        self.oblast_connection = oblast_connection
        self.name = name
        self.value = None
        self.callback = callback

    def _set(self, data):
        # Setting the variable value *without* notifying the server:
        # (This is used when new data comes in - we don't want an update loop)
        self.value = data

    def set(self, data):
        self._set(data)
        self.oblast_connection._send("update", self.name, data)

    def get(self, data):
        return self.value

    def fire_reactor(self):
        self.callback(self.name, self.value)


class OblastConnection:
    MCAST_GRP_PORT = ("226.1.1.38", 1291)
    LOCAL_UDP_PORT = 1292

    def __init__(self):
        self.socket = None
        self.subscribed_vars = {}
        self.all_callback = None

    def connect(self):
        # 1. Server discover phase (using UDP multicast)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp_socket:
            udp_socket.bind(("", OblastConnection.LOCAL_UDP_PORT))

            byte_message = b"oblast_discover"

            attempts = 5
            server_address = None
            timeout = 0.15
            while attempts>0:
                udp_socket.sendto(byte_message, OblastConnection.MCAST_GRP_PORT)
                try:
                    udp_socket.settimeout(timeout)
                    timeout *= 1.5
                    expected_answer = "oblast_server"
                    data, address = udp_socket.recvfrom(len(expected_answer))
                    response = data.decode("utf-8")
                    if response == expected_answer:
                        server_address = address
                        attempts = 0
                except OSError as e:
                    pass
                attempts -= 1


        if server_address is None:
            raise ConnectionRefusedError("Unable to locate the localhost server")

        # 2. Server TCP connection:
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect(server_address)

    def close(self):
        self.socket.close()

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, type, value, traceback):
        self.close()


    def _send(self, action, key=None, value=None):
        payload = action
        if key is not None:
            payload += ":%s" % key
        if value is not None:
            payload += "=%s" % value
        payload = payload.encode("utf-8")

        # converting to network endianness (Big Endian) unsigned short
        packed_payload_len = struct.pack("!H", len(payload))
        sent_bytes = self.socket.send(packed_payload_len)
        sent_bytes += self.socket.send(payload)
        if sent_bytes == 0:
            raise RuntimeError("The socket connection is brocken.")

    def subscribe(self, var_name, callback=None):
        self._send("subscribe", var_name)
        new_var = OblastVar(self, var_name, callback)
        self.subscribed_vars[var_name] = new_var
        return new_var

    def subscribe_all(self, callback=None):
        if self.all_callback is not None:
            raise ValueError("A previous registration on all variable " \
                "has already been made.")
        self.all_callback = callback
        self._send("subscribe_all")

    def handle_updates(self):
        self.socket.setblocking(0)
        try:
            while True:
                length = struct.unpack("!H", self.socket.recv(2))[0]
                message = self.socket.recv(length).decode("utf-8")
                try:
                    action, payload = message.split(":")
                    key, *values = payload.split("=")
                    value = "=".join(values)

                    if key in self.subscribed_vars:
                        self.subscribed_vars[key]._set(value)
                        self.subscribed_vars[key].fire_reactor()

                        # Also calling the all_callback if available:
                        if self.all_callback is not None:
                            self.all_callback(key, value)
                    else:
                        if self.all_callback is not None:
                                self.all_callback(key, value)
                except ValueError as error:
                    print("Ill-formed message :", message)
        except BlockingIOError as error:
            pass
