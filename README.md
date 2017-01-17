# bc-bridge
=======
# Clown.Bridge

Clown.Bridge is a proxy between BigClown's Bridge Module
(connected via USB HID class) and Clown.Talk protocol on stdin/stdout.

##### Install on Raspbian, Debian, Ubuntu
> Add BigClown repository

    sudo sh -c 'echo "deb https://repo.bigclown.com/debian jessie main" >/etc/apt/sources.list.d/bigclown.list'
    wget https://repo.bigclown.com/debian/pubkey.gpg -O - | sudo apt-key add -
    sudo apt-get update

> Install bc-bridge, udev rules and systemd service

    sudo apt-get install bc-bridge

> **Important** Service bc-bridge.service is enabled after install

##### Building

> The following snippet has been tested on Ubuntu Desktop 16.04 LTS.

    sudo apt-get install build-essential cmake git libudev-dev
    git clone --recursive https://github.com/bigclownlabs/bc-bridge.git
    mkdir -p bc-bridge/build
    cd bc-bridge/build
    cmake .
    cmake --build .

##### Run without sudo

> create file /etc/udev/rules.d/99-bc-bridge.rules with this content

    SUBSYSTEM !="usb_device", ACTION !="add", GOTO="device_rules_end"
    SYSFS{idVendor} =="0403", SYSFS{idProduct} =="6030", SYMLINK+="bc_bridge_device" MODE="0666", TAG+="uaccess"
    LABEL="device_rules_end"


##### How to run
> with udev rules

    ./bc-bridge -f

> without udev rules

    sudo ./bc-bridge -f

##### Print command line help

    ./bc-bridge --help


##### Initialize Clown.Talk

    ["clown.talk/-/config/set", {}]

> **Note** Waiting for the start string can skip using the -f option

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

##### Get actuator's state

    ["relay/i2c0-3b/get", {}]


##### On Segmentation fault

> Find file `core` in folder with bc-bridge program and send on our support email with version info or git info,
to obtain info use

    ./bc-bridge -v
    git rev-parse --verify HEAD

> If folder doesn't contains file `core`, it may be that the core size is set to 0. Try this command

    ulimit -c

> if 0, set size

    ulimit -c unlimited

> try again run bc-bridge
