#!/bin/bash

# Redis server information (adjust as needed)
REDIS_HOST="localhost"
REDIS_PORT=6379

# Path to your redis-cli binary (adjust as needed)
REDIS_CLI_PATH="/home/abhattar/Desktop/redis/redis/src/redis-cli "

# Function to delete all nodes and their neighbors from Redis
delete_all_nodes() {
    echo "Deleting node neighbors..."
    $REDIS_CLI_PATH -h $REDIS_HOST -p $REDIS_PORT --scan --pattern "node_*_neighbours" | \
    xargs -r $REDIS_CLI_PATH -h $REDIS_HOST -p $REDIS_PORT DEL
}

# Function to delete all node colors from Redis
delete_all_colors() {
    echo "Deleting node colors..."
    $REDIS_CLI_PATH -h $REDIS_HOST -p $REDIS_PORT --scan --pattern "node_*_color" | \
    xargs -r $REDIS_CLI_PATH -h $REDIS_HOST -p $REDIS_PORT DEL
}

# Call the functions to delete nodes and colors
delete_all_nodes
delete_all_colors

echo "All graph nodes and their colors have been deleted from Redis."