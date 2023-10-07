#!/usr/bin/env python3

import os
import time
from oblast import OblastConnection


def reactor(var_name, var_value):
    print("Reactor: %s: %s" %(var_name, var_value))

def read_sensor_data():
    return "foo=%d" % os.getpid()

def main():
    with OblastConnection() as connection:
        try:
            top_setting = connection.subscribe("livingroom_top_light_setting", reactor)
            top_status = connection.subscribe("livingroom_top_light_status", reactor)
            bottom_setting = connection.subscribe("livingroom_bottom_light_setting", reactor)
            bottom_status = connection.subscribe("livingroom_bottom_light_status", reactor)
            while True:
                connection.handle_updates()
                top_setting.set("1")
                time.sleep(2)
                top_setting.set("1")
                time.sleep(2)

                bottom_setting.set("1")
                time.sleep(2)
                bottom_setting.set("0")
                time.sleep(2)
        except KeyboardInterrupt as e:
            print("Terminating...")

if __name__ == "__main__":
    main()
