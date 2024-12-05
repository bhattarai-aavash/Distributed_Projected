#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

#define MAX_NODES 1000
#define MAX_COLOR 1000

// Function to get the total number of nodes
int get_total_nodes(redisContext *context) {
    redisReply *reply;
    reply = redisCommand(context, "KEYS node_*_neighbours"); 

    int total_nodes = 0;
    if (reply->type == REDIS_REPLY_ARRAY) {
        total_nodes = reply->elements; 
    }
    freeReplyObject(reply);
    return total_nodes;
}

// Function to get adjacent nodes for a given node
int *get_adjacent_nodes(redisContext *context, int node, int *adj_size) {
    redisReply *reply;
    char command[256];
    snprintf(command, sizeof(command), "SMEMBERS node_%d_neighbours", node);
    reply = redisCommand(context, command);

    if (reply->type == REDIS_REPLY_ARRAY) {
        *adj_size = reply->elements;
        int *adj_nodes = malloc((*adj_size) * sizeof(int));
        for (size_t i = 0; i < reply->elements; i++) {
            adj_nodes[i] = atoi(reply->element[i]->str);
        }
        freeReplyObject(reply);
        return adj_nodes;
    } else {
        *adj_size = 0;
        freeReplyObject(reply);
        return NULL;
    }
}

// Function to store the color of a node
void store_node_color(redisContext *context, int node, int color) {
    redisReply *reply;
    char command[256];
    snprintf(command, sizeof(command), "SET node_%d_color %d", node, color);
    reply = redisCommand(context, command);
    freeReplyObject(reply);
}

// Function to check if a node can be colored with a specific color
int can_color(int node, int color, int *colors, int *adj_nodes, int adj_size) {
    for (int i = 0; i < adj_size; i++) {
        int adj_node = adj_nodes[i];
        if (colors[adj_node] == color) {
            return 0;  
        }
    }
    return 1; 
}

// Greedy coloring algorithm
void greedy_coloring(redisContext *context, int total_nodes) {
    int colors[MAX_NODES];
    memset(colors, -1, sizeof(colors));  

    for (int node = 0; node < total_nodes; node++) {
        int adj_size = 0;
        int *adj_nodes = get_adjacent_nodes(context, node, &adj_size);  

        // Try to assign the lowest possible color
        for (int color = 0; color < MAX_COLOR; color++) {
            if (can_color(node, color, colors, adj_nodes, adj_size)) {
                colors[node] = color;  
                store_node_color(context, node, color);  
                printf("Node %d colored with color %d\n", node, color); // Debug print
                break;
            }
        }

        free(adj_nodes);  
    }
}

// Main function
int main() {
    // Connect to Redis
    redisContext *context = redisConnect("localhost", 6379);
    if (context == NULL || context->err) {
        if (context) {
            printf("Error: %s\n", context->errstr);
            redisFree(context);
        } else {
            printf("Can't allocate redis context\n");
        }
        return 1;
    }

    // Get total number of nodes
    int total_nodes = get_total_nodes(context);
    printf("Total nodes in the graph: %d\n", total_nodes);

    // Run the greedy coloring algorithm
    greedy_coloring(context, total_nodes);

    // Free Redis context
    redisFree(context);

    printf("Graph coloring complete and stored in Redis.\n");
    return 0;
}
