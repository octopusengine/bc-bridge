
How build 


    apt-get install cmake
    apt-get install libudev-dev
    apt-get install git
    git clone git@bitbucket.org:BigClown/bc-bridge.git
    cd bc-bridge
    cmake .
    cmake --build .

and run

    ./bc-bridge -n --debug info


for help

    ./bc-bridge --help



Initialize Clown.Talk

    ["$config/clown-talk/create", {}]

Change Sensor interval for measurement and message transfer

    ["$config/sensors/thermometer/i2c0-48/update", {"publish-interval": 500}]
    
Set Actor state  
    
    ["relay/i2c0-3b/set", {"state": true}]
    ["relay/i2c0-3b/set", {"state": false}]