#!/bin/bash

# Start Mosquitto
sudo systemctl start mosquitto

# Start the web interface
cd /home/pi/spoofing_project/web_interface
python3 app.py &

# Start the coordinator
cd /home/pi/spoofing_project/scripts
python3 coordinate_devices.py &

# Start the MITM proxy
cd /home/pi/spoofing_project/mitm_proxy
mitmproxy -s mitm_script.py &

echo "All services started."
