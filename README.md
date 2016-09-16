How to build

    sudo apt-get install cmake libudev-dev git
    git clone git@bitbucket.org:BigClown/bc-bridge.git
    cd bc-bridge
    cmake .
    cmake --build .

How to run

    ./bc-bridge -n --debug info

Print command line help

    ./bc-bridge --help

Initialize Clown.Talk

    ["$config/clown.talk/-/create", {}]

Change sensor measurement interval

    ["$config/sensors/thermometer/i2c0-48/update", {"publish-interval": 500}]
    
Set actuator state
    
    ["relay/i2c0-3b/set", {"state": true}]
    ["relay/i2c0-3b/set", {"state": false}]

