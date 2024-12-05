#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>


int check_if_available(char **ip_list, int ip_count, int node1, int node2) {
    for (int i = 0; i < ip_count; i++) {
        // Connect to Redis server for each IP
        redisContext *context = redisConnect(ip_list[i], 6379);
        if (context == NULL || context->err) {
            if (context) {
                printf("Error connecting to Redis server at %s: %s\n", ip_list[i], context->errstr);
                redisFree(context);
            } else {
                printf("Error allocating Redis context for server %s\n", ip_list[i]);
            }
            return 0; 
        }

      
        char lock_key[256];
        snprintf(lock_key, sizeof(lock_key), "lock_edge_%d_%d", node1, node2);
        redisReply *lock_reply = redisCommand(context, "EXISTS %s", lock_key);

        int is_lock_acquired = lock_reply->integer;
        freeReplyObject(lock_reply);

       
        char turn_key[256];
        snprintf(turn_key, sizeof(turn_key), "turn_%d_%d", node1, node2);
        redisReply *turn_reply = redisCommand(context, "EXISTS %s", turn_key);

        int is_turn_set = turn_reply->integer;
        freeReplyObject(turn_reply);

        
        redisFree(context);

        if (is_lock_acquired || is_turn_set) {
            printf("Edge (%d, %d) not available on server %s\n", node1, node2, ip_list[i]);
            return 0; 
        }
    }

    printf("Edge (%d, %d) is available on all servers\n", node1, node2);
    return 1; 
}


int acquire_lock(redisContext *context, int node1, int node2) {
    char lock_key[256];
    snprintf(lock_key, sizeof(lock_key), "lock_edge_%d_%d", node1, node2);

    redisReply *reply = redisCommand(context, "SETNX %s %d", lock_key, 1);

    if (reply && reply->integer == 1) {
        // Lock acquired
        printf("Lock acquired for edge (%d, %d) ", node1, node2);
        freeReplyObject(reply);
        return 1; // Success
    }

    freeReplyObject(reply);
    return 0; // Lock not acquired
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

int set_turn_variable(redisContext *context, int node1, int node2) {
    char turn_key[256];
    snprintf(turn_key, sizeof(turn_key), "turn_%d_%d", node1, node2);

    redisReply *reply = redisCommand(context, "SET %s %d", turn_key, 1);

    if (reply && reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0) {
        freeReplyObject(reply);
        return 1; // Successfully set the turn variable
    }

    freeReplyObject(reply);
    return 0; // Failed to set the turn variable
}
