#!/bin/bash

REDIS_HOST="localhost"
REDIS_PORT=6379
GRAPH_FILE="/home/abhattar/Desktop/redis/redis/color/graphs/CL-1000-1d7-trial1.txt"

store_in_redis() {
    node=$1
    shift
    adj_nodes=("$@")

    /home/abhattar/Desktop/redis/redis/src/redis-cli -h $REDIS_HOST -p $REDIS_PORT SADD "node_${node}_neighbours" "${adj_nodes[@]}"
}

store_node_color() {
    node=$1
    color=$2

    /home/abhattar/Desktop/redis/redis/src/redis-cli -h $REDIS_HOST -p $REDIS_PORT SET "node_${node}_color" "$color"
}

assign_color() {
    echo 0  
}

# Record the start time
start_time=$(date +%s)

# while read -r line; do
#     # Ignore empty lines
#     if [[ -z "$line" ]]; then
#         continue
#     fi

#     # Split the line into two nodes based on a comma
#     IFS=',' read -r node1 node2 <<< "$line"

#     # Trim whitespace if necessary
#     node1=$(echo "$node1" | xargs)
#     node2=$(echo "$node2" | xargs)

#     # Store node1's neighbor as node2
#     store_in_redis "$node1" "$node2"
#     # Store node2's neighbor as node1
#     store_in_redis "$node2" "$node1"

#     # Assign and store colors for both nodes
#     node_color=$(assign_color)
#     store_node_color "$node1" "$node_color"
#     store_node_color "$node2" "$node_color"
# done < "$GRAPH_FILE"

# Record the end time
/home/abhattar/Desktop/redis/redis/src/redis-cli -h $REDIS_HOST -p $REDIS_PORT add_graph /home/abhattar/Desktop/redis/redis/color/graphs/edge_graph_dblp_coauthorship_n317080.txt

end_time=$(date +%s)

# Calculate the elapsed time
elapsed_time=$((end_time - start_time))

echo "Graph and node colors (0 for all nodes) have been stored in Redis."
echo "Time taken to store the graph: ${elapsed_time} seconds."
