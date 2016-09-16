# Clown.Bridge

Clown.Bridge is a proxy between BigClown's Bridge Module
(connected via USB HID class) and Clown.Talk protocol on stdin/stdout.

##### Building

> The following snippet has been tested on Ubuntu Desktop 16.04 LTS.

    sudo apt-get install cmake libudev-dev git
    git clone git@bitbucket.org:BigClown/bc-bridge.git
    cd bc-bridge
    cmake .
    cmake --build .

##### How to run

    ./bc-bridge

##### Print command line help

    ./bc-bridge --help

##### Initialize Clown.Talk

    ["$config/clown.talk/-/create", {}]

##### Set sensor's measurement interval

    ["$config/sensors/thermometer/i2c0-48/update", {"publish-interval": 500}]
    
##### Set actuator's state
    
    ["relay/i2c0-3b/set", {"state": true}]
    ["relay/i2c0-3b/set", {"state": false}]

## More information

You can find detailed documentation for Clown.Talk protocol on this link:
`TODO: Add link`
