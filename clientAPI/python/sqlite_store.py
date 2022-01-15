#!/usr/bin/env python3

import datetime
import os
import time
import sqlite3

from oblast import OblastConnection

db_connection = sqlite3.connect('database.db')
registered_vars = set(())
#resolutions = {"day": 86400, "hour": 3600, "minute": 60, "second": 1}
resolutions = {"second": 1}
def database_save(var_name, var_value):
    cur = db_connection.cursor()
    record_time = datetime.datetime.now()

    # If the table do not exist, let's create them:
    if not var_name in registered_vars:
        for resolution_name, resolution_seconds_qty in resolutions.items():
            table_name = "`%s_%s`" % (var_name, resolution_name)
            cur.execute("CREATE TABLE IF NOT EXISTS" + table_name \
                + "(id INTEGER PRIMARY KEY AUTOINCREMENT," \
                + " time DATETIME," \
                + " value TEXT)" \
            )
        registered_vars.add(var_name)

    # Storing the data at the correct resolution:
    for resolution_name, resolution_seconds_qty in resolutions.items():
        table_name = "`%s_%s`" % (var_name, resolution_name)
        timestamp = int(datetime.datetime.timestamp(record_time))
        if timestamp%resolution_seconds_qty == 0 :
            sql = "INSERT INTO " + table_name + " VALUES (NULL, ?, ?)"
            cur.execute(sql, (record_time, var_value))
            print("storing a value for", resolution_name)
    db_connection.commit()

    print("reactor_all: %s: %s" %(var_name, var_value))

def main():
    with OblastConnection() as connection:
        try:
            connection.subscribe_all(database_save)
            while True:
                connection.handle_updates()
                time.sleep(1)
        except KeyboardInterrupt as e:
            print("Terminating...")
    db_connection.close()

if __name__ == "__main__":
    main()
