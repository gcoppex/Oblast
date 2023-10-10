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
            upper_ring_top_setting = connection.subscribe("diningroom_upper_ring_top_light_setting", reactor)
            upper_ring_top_status = connection.subscribe("diningroom_upper_ring_top_light_status", reactor)
            upper_ring_bottom_setting = connection.subscribe("diningroom_upper_ring_bottom_light_setting", reactor)
            upper_ring_bottom_status = connection.subscribe("diningroom_upper_ring_bottom_light_status", reactor)
            
            lower_ring_top_setting = connection.subscribe("diningroom_lower_ring_top_light_setting", reactor)
            lower_ring_top_status = connection.subscribe("diningroom_lower_ring_top_light_status", reactor)
            lower_ring_bottom_setting = connection.subscribe("diningroom_lower_ring_bottom_light_setting", reactor)
            lower_ring_bottom_status = connection.subscribe("diningroom_lower_ring_bottom_light_status", reactor)
            
            while True:
                connection.handle_updates()
                lower_ring_top_setting.set("1")
                lower_ring_bottom_setting.set("0")
                upper_ring_top_setting.set("0")
                upper_ring_bottom_setting.set("0")
                time.sleep(2)
                
                lower_ring_top_setting.set("0")
                lower_ring_bottom_setting.set("1")
                upper_ring_top_setting.set("0")
                upper_ring_bottom_setting.set("0")
                time.sleep(2)
                
                lower_ring_top_setting.set("0")
                lower_ring_bottom_setting.set("0")
                upper_ring_top_setting.set("1")
                upper_ring_bottom_setting.set("0")
                time.sleep(2)
                
                lower_ring_top_setting.set("0")
                lower_ring_bottom_setting.set("0")
                upper_ring_top_setting.set("0")
                upper_ring_bottom_setting.set("1")
                time.sleep(2)
        except KeyboardInterrupt as e:
            print("Terminating...")

if __name__ == "__main__":
    main()
