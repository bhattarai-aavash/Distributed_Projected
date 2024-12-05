#!/bin/bash

# Run first script in the background using nohup
nohup ./color_1.sh > color_1.log 2>&1 &

# Run second script in the background using nohup
nohup ./color_2.sh > color_2.log 2>&1 &

echo "Both scripts are running in the background."