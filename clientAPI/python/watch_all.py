#!/usr/bin/env python3

import os
import time
from oblast import OblastConnection


def reactor(var_name, var_value):
    print("reactor_all: %s: %s" %(var_name, var_value))

def read_sensor_data():
    return "foo=%d" % os.getpid()

def main():
    with OblastConnection() as connection:
        try:
            connection.subscribe_all(reactor)
            while True:
                connection.handle_updates()
                time.sleep(1)
        except KeyboardInterrupt as e:
            print("Terminating...")


if __name__ == "__main__":
    main()
