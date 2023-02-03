#!/usr/bin/env python3

import requests

url = "http://wd1.wspr.rocks/api/clickhouse/"

# data = {"parms": "true", "balloons": "hide", "txCall": "VU3CER", "band": "all", "hours": 840, "unique": "false", "count": 500}
data = {"parms": "true", "balloons": "hide", "txCall": "VU3CER", "band": "all", "hours": 840, "unique": "false", "count": 512}

r = requests.post(url=url, data=data)

data = r.json()["data"]

for row in data:
    print(row)
