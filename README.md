cmake . && make all && ./Bridge


Initialize Clown.Talk

`["$config/clown-talk/create", {}]`

Change Sensor interval for measurement and message transfer

`["$config/sensors/thermometer/update", {"publish-interval": 500}]`