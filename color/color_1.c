#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "avail.h"

#define MAX_NODES 1000
#define MAX_COLOR 1000


int get_total_nodes(redisContext *context) {
    redisReply *reply = redisCommand(context, "KEYS node_*_neighbours");
    int total_nodes = 0;

    if (reply->type == REDIS_REPLY_ARRAY) {
        total_nodes = reply->elements;
    }

    freeReplyObject(reply);
    return total_nodes;
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

// Perform greedy graph coloring
void greedy_coloring(redisContext *context, int start_node, int end_node, int process_id) {
    int colors[MAX_NODES];
    memset(colors, -1, sizeof(colors)); // Initialize all node colors to -1
    
    char *ip_list[] = {"10.22.12.242", "10.22.12.168"};
    char *ip_count = 2;
    for (int node = start_node; node <= end_node; node++) {
        int adj_size = 0;
        int *adj_nodes = get_adjacent_nodes(context, node, &adj_size);

        // Acquire locks for all edges connected to this node and set the turn
        for (int i = 0; i < adj_size; i++) {
            acquire_lock(context, node, adj_nodes[i]);
            set_turn_variable(context, node, adj_nodes[i]);
        }

        // Wait until the edge is available to be colored
        while (!check_if_available(ip_list, ip_count, node, adj_nodes[0])) {
            printf("Process %d waiting for lock and turn to be correct for edge (%d, %d)...\n", process_id, node, adj_nodes[0]);
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

        // Release locks and delete turn variables for all edges connected to this node
        for (int i = 0; i < adj_size; i++) {
            release_lock(context, node, adj_nodes[i]);
            delete_turn_variable(context, node, adj_nodes[i]);
        }

        free(adj_nodes);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <start_node> <end_node> <process_id>\n", argv[0]);
        return 1;
    }

    int start_node = atoi(argv[1]);
    int end_node = atoi(argv[2]);
    int process_id = atoi(argv[3]);  // Get the process ID as input

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

    // Call the greedy coloring function
    greedy_coloring(context, start_node, end_node, process_id);

    // Clean up and free the Redis context
    redisFree(context);
    return 0;
}
