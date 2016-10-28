# Clown.Bridge

Clown.Bridge is a proxy between BigClown's Bridge Module
(connected via USB HID class) and Clown.Talk protocol on stdin/stdout.

##### Building

> The following snippet has been tested on Ubuntu Desktop 16.04 LTS.

    sudo apt-get install build-essential cmake git libudev-dev 
    git clone --recursive https://github.com/bigclownlabs/bc-bridge.git
    cd bc-bridge
    cmake .
    cmake --build .

##### How to run

    ./bc-bridge

##### Print command line help

    ./bc-bridge --help


##### Initialize Clown.Talk

    ["clown.talk/-/config/set", {}]

##### List of sensors and actuators 

    ["-/-/config/list", {}]
    
##### Set led state

    ["led/-/set", {"state":"on"}]
    ["led/-/set", {"state":"off"}]
    ["led/-/set", {"state":"1-dot"}]
    ["led/-/set", {"state":"2-dot"}]
    ["led/-/set", {"state":"3-dot"}]

##### Set sensor's measurement interval set 5 seconds

    ["thermometer/i2c0-48/config/set", {"publish-interval": 5}]
    
##### Set actuator's state
    
    ["relay/i2c0-3b/set", {"state": true}]
    ["relay/i2c0-3b/set", {"state": false}]
 
