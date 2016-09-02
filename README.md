
install

sudo apt-get install libudev-dev

How build 

cd bc-bridge

cmake . 

cmake --build .


and run

./bc-bridge -n --debug info


for help

./bc-bridge --help



Initialize Clown.Talk

`["$config/clown-talk/create", {}]`

Change Sensor interval for measurement and message transfer

`["$config/sensors/thermometer/update", {"publish-interval": 500}]`