#!/bin/bash

# Fetch all nodes that have colors stored
nodes=$(./src/redis-cli KEYS "node_*_color")

# Check if any nodes were found
if [ -z "$nodes" ]; then
    echo "No nodes found."
    exit 0
fi

# Loop through each node and retrieve its color
for node in $nodes; do
    color=$(./src/redis-cli GET "$node")
    if [ -n "$color" ]; then
        echo "$node: Color $color"
    else
        echo "$node: No color assigned"
    fi
done