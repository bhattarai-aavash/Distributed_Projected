#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include <unistd.h>

#define MAX_NODES 1000
#define MAX_COLOR 1000

// Function to get the total number of nodes in the graph
int get_total_nodes(redisContext *context) {
    redisReply *reply = redisCommand(context, "KEYS node_*_neighbours");
    int total_nodes = 0;

    if (reply->type == REDIS_REPLY_ARRAY) {
        total_nodes = reply->elements;
    }

    freeReplyObject(reply);
    return total_nodes;
}

int acquire_lock(redisContext *context, int node1, int node2, int process_id) {
    char lock_key[256];
    snprintf(lock_key, sizeof(lock_key), "lock_edge_%d_%d_%d", node1, node2, process_id);

    while (1) {
        redisReply *reply = redisCommand(context, "SETNX %s %d", lock_key, process_id);

        if (reply && reply->integer == 1) {
            // Lock acquired
            printf("Lock acquired for edge (%d, %d) by process %d\n", node1, node2, process_id);
            freeReplyObject(reply);
            return 1; // Success
        }

        // If lock is not acquired, wait and retry
        printf("Waiting for lock on edge (%d, %d)...\n", node1, node2);
        freeReplyObject(reply);
    }
}

// Release lock for an edge
void release_lock(redisContext *context, int node1, int node2) {
    char lock_key[256];
    snprintf(lock_key, sizeof(lock_key), "lock_edge_%d_%d", node1, node2);

    redisReply *reply = redisCommand(context, "DEL %s", lock_key);

    if (reply) {
        printf("Lock released for edge (%d, %d)\n", node1, node2);
        freeReplyObject(reply);
    }
}

// Get adjacent nodes for a given node
int *get_adjacent_nodes(redisContext *context, int node, int *adj_size) {
    char command[256];
    snprintf(command, sizeof(command), "SMEMBERS node_%d_neighbours", node);

    redisReply *reply = redisCommand(context, command);

    if (reply->type == REDIS_REPLY_ARRAY) {
        *adj_size = reply->elements;
        int *adj_nodes = malloc((*adj_size) * sizeof(int));

        for (size_t i = 0; i < reply->elements; i++) {
            adj_nodes[i] = atoi(reply->element[i]->str);
        }

        freeReplyObject(reply);
        return adj_nodes;
    }

    *adj_size = 0;
    freeReplyObject(reply);
    return NULL;
}

// Store the color of a node in Redis
void store_node_color(redisContext *context, int node, int color) {
    char command[256];
    snprintf(command, sizeof(command), "SET node_%d_color %d", node, color);

    redisReply *reply = redisCommand(context, command);
    if (reply) {
        freeReplyObject(reply);
    }
}

// Check if a node can be colored with a specific color
int can_color(int node, int color, int *colors, int *adj_nodes, int adj_size) {
    for (int i = 0; i < adj_size; i++) {
        if (colors[adj_nodes[i]] == color) {
            return 0; // Adjacent node has the same color
        }
    }
    return 1;
}

// Perform greedy graph coloring
void greedy_coloring(redisContext *context, int start_node, int end_node) {
    int colors[MAX_NODES];
    memset(colors, -1, sizeof(colors)); // Initialize all node colors to -1

    for (int node = start_node; node <= end_node; node++) {
        int adj_size = 0;
        int *adj_nodes = get_adjacent_nodes(context, node, &adj_size);

        // Acquire locks for all edges connected to this node
        for (int i = 0; i < adj_size; i++) {
            acquire_lock(context, node, adj_nodes[i], 1);
        }

        // Assign the lowest possible color
        for (int color = 0; color < MAX_COLOR; color++) {
            if (can_color(node, color, colors, adj_nodes, adj_size)) {
                colors[node] = color;
                store_node_color(context, node, color);
                printf("Node %d colored with color %d\n", node, color);
                break;
            }
        }

        // Release locks for all edges connected to this node
        for (int i = 0; i < adj_size; i++) {
            release_lock(context, node, adj_nodes[i]);
        }

        free(adj_nodes);
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <start_node> <end_node>\n", argv[0]);
        return 1;
    }

    int start_node = atoi(argv[1]);
    int end_node = atoi(argv[2]);

    // Connect to Redis
    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == NULL || context->err) {
        if (context) {
            printf("Error: %s\n", context->errstr);
            redisFree(context);
        } else {
            printf("Can't allocate Redis context\n");
        }
        return 1;
    }

    printf("Connected to Redis server.\n");

    // Perform greedy coloring for the specified range of nodes
    greedy_coloring(context, start_node, end_node);

    // Disconnect from Redis
    redisFree(context);

    return 0;
}
