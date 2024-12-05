#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
typedef struct {
    char lock_key_1[128];  // Change from char** to char[] (array of chars)
    char lock_key_2[128];
    char turn_key[128];
} ClientGraphPetersonLock;


int getMinimalValueNotInArray(int *arr, int arrLength, int lowerBound, int upperBound) {
    int range = upperBound - lowerBound + 1;
    
    // Dynamically allocate memory for the used values array
    int *usedValues = (int *)calloc(range, sizeof(int));
    if (usedValues == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE); // Exit with an error
    }

    // Mark used values within the range [lowerBound, upperBound]
    for (int i = 0; i < arrLength; i++) {
        if (arr[i] >= lowerBound && arr[i] <= upperBound) {
            usedValues[arr[i] - lowerBound] = 1;
        }
    }

    // Find the first available unused value
    for (int i = lowerBound; i <= upperBound; i++) {
        if (usedValues[i - lowerBound] == 0) {
            free(usedValues); // Free the allocated memory
            return i;
        }
    }

    // Debug message: range too small
    printf("getMinimalValueNotInArray: error: the range is too small\n");

    // Free the allocated memory and recursively call with an extended range
    free(usedValues);
    return getMinimalValueNotInArray(arr, arrLength, lowerBound, upperBound + arrLength + 1);
}

void print_locks_array(ClientGraphPetersonLock *locks_array, int num_neighbors) {
    for (int i = 0; i < num_neighbors; i++) {
        printf("Lock %d:\n", i + 1);
        printf("  lock_key_1: %s\n", locks_array[i].lock_key_1);
        printf("  lock_key_2: %s\n", locks_array[i].lock_key_2);
        printf("  turn_key: %s\n", locks_array[i].turn_key);
    }
}

int acquire_lock(redisContext *context, ClientGraphPetersonLock *lock) {
    redisReply *reply;
    
    // Try to acquire the lock using SETNX for both flags
    reply = redisCommand(context, "SETNX %s 1", lock->lock_key_1);
    if (reply->integer == 1) {
        
        freeReplyObject(reply);
        
        reply = redisCommand(context, "SETNX %s 1", lock->lock_key_2);
        if (reply->integer == 1) {
            
            freeReplyObject(reply);
            
            
            reply = redisCommand(context, "SET %s 1", lock->turn_key);
            freeReplyObject(reply);
            
            return 1; 
        } else {
           
            redisCommand(context, "DEL %s", lock->lock_key_1);
            freeReplyObject(reply);
        }
    } else {
        freeReplyObject(reply);
    }
    
    return 0;  // Failed to acquire lock
}
void release_lock(redisContext *context, ClientGraphPetersonLock *lock) {
    redisReply *reply;

    // Delete the first lock key
    reply = redisCommand(context, "DEL %s", lock->lock_key_1);
    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
        fprintf(stderr, "Error: Failed to delete lock_key_1: %s\n", lock->lock_key_1);
    }
    if (reply) {
        freeReplyObject(reply);
    }

    // Delete the second lock key
    reply = redisCommand(context, "DEL %s", lock->lock_key_2);
    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
        fprintf(stderr, "Error: Failed to delete lock_key_2: %s\n", lock->lock_key_2);
    }
    if (reply) {
        freeReplyObject(reply);
    }

    // Delete the turn key
    reply = redisCommand(context, "DEL %s", lock->turn_key);
    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
        fprintf(stderr, "Error: Failed to delete turn_key: %s\n", lock->turn_key);
    }
    if (reply) {
        freeReplyObject(reply);
    }
}

// Function to get all neighbors of a node using SMEMBERS
char** get_all_neighbours(redisContext *context, const char *node_name, int *neighbour_count) {
    // Generate the Redis key for the neighbors
    char key[256];
    snprintf(key, sizeof(key), "%s_neighbours", node_name);

    // Send the Redis SMEMBERS command
    redisReply *reply = redisCommand(context, "SMEMBERS %s", key);

    // Check if the key exists and if the reply is valid
    if (reply == NULL || reply->type == REDIS_REPLY_NIL) {
        fprintf(stderr, "Error: Node %s does not exist or has no neighbors.\n", node_name);
        freeReplyObject(reply);
        return NULL;
    }

    // Check if the reply contains an array
    if (reply->type != REDIS_REPLY_ARRAY) {
        fprintf(stderr, "Error: Unexpected reply type for node %s neighbors.\n", node_name);
        freeReplyObject(reply);
        return NULL;
    }

    // Allocate memory for the neighbor list
    int count = reply->elements;
    char **neighbor_list = malloc(sizeof(char*) * count);
    if (neighbor_list == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        freeReplyObject(reply);
        return NULL;
    }

    // Copy neighbors from the reply into the list
    for (int i = 0; i < count; i++) {
        neighbor_list[i] = strdup(reply->element[i]->str);
    }

    *neighbour_count = count; // Return the number of neighbors
    freeReplyObject(reply);
    return neighbor_list;
}

// Comparator function for numerical sorting
int compare_neighbours(const void *a, const void *b) {
    // Convert the strings to integers and compare them
    int num_a = atoi(*(const char **)a);
    int num_b = atoi(*(const char **)b);
    return num_a - num_b;
}

// Function to sort the neighbor list numerically
void sort_neighbours(char **neighbour_list, int neighbour_count) {
    qsort(neighbour_list, neighbour_count, sizeof(char *), compare_neighbours);
}


int get_node_color(redisContext *context, const char *node_name) {
    char key[256];
    snprintf(key, sizeof(key), "node_%s_color", node_name);
    redisReply *reply = redisCommand(context, "GET %s", key);
    if (reply == NULL || reply->type == REDIS_REPLY_NIL) {
        fprintf(stderr, "Error: Node %s color not found.\n", node_name);
        return -1; // Return -1 if not found
    }

    
    return atoi(reply->str);
}
void set_node_color(redisContext *context, const char *node_name, int color) {
    char key[256];
    snprintf(key, sizeof(key), "%s_color", node_name);
    
    // Convert the integer color to string
    char color_str[10];
    snprintf(color_str, sizeof(color_str), "%d", color);
    
    // Set the color in Redis
    redisReply *reply = redisCommand(context, "SET %s %s", key, color_str);
    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
        fprintf(stderr, "Error: Failed to set color for node %s.\n", node_name);
        if (reply != NULL) {
            freeReplyObject(reply);
        }
        return; // Exit the function
    }

    freeReplyObject(reply);
}
int get_node_id(const char *node_name) {
    const char *prefix = "node_";
    size_t prefix_len = strlen(prefix);

    // Check if the input starts with the prefix "node_"
    if (strncmp(node_name, prefix, prefix_len) != 0) {
        fprintf(stderr, "Error: Invalid node name format: %s\n", node_name);
        return -1; // Return an error value
    }

    // Extract the numeric part of the node name and convert to integer
    int node_id = atoi(node_name + prefix_len);
    return node_id;
}
char* extract_number_from_string(const char *str) {
    // Find the position of the underscore
    const char *underscore_pos = strchr(str, '_');
    if (underscore_pos == NULL) {
        return NULL;  // Return NULL if there's no underscore in the string
    }
    
    // Allocate memory to hold the number string (after the underscore)
    char *result = (char*)malloc(strlen(underscore_pos) * sizeof(char));
    if (result == NULL) {
        return NULL;  // Return NULL if memory allocation fails
    }
    
    // Copy the part of the string after the underscore
    strcpy(result, underscore_pos + 1);  // Skip the underscore itself

    return result;
}
void read_color_of_neighbors(redisContext *context,
                              const char *node_id,
                              int first_node_id_of_first_task,
                              int last_node_id_of_last_task,
                              char **node_nbr_list_sorted,
                              int *nbr_color_list,
                              int num_neighbors,
                              ClientGraphPetersonLock *locks_array) { // New parameter for locks array
    for (int nbr = 0; nbr < num_neighbors; nbr++) {
        const char *nbr_id = node_nbr_list_sorted[nbr];
        int nbr_id_int = atoi(nbr_id);
        int node_id_int = get_node_id(node_id);
        char *char_node = extract_number_from_string(node_id);

        // Generate bothId
        char bothId[64];
        if (nbr_id_int < node_id_int) {
            snprintf(bothId, sizeof(bothId), "%s_%s", nbr_id, char_node);
        } else {
            snprintf(bothId, sizeof(bothId), "%s_%s", char_node, nbr_id);
        }

        // Create and store the lock in the locks array
        ClientGraphPetersonLock *lock = &locks_array[nbr]; // Reference the specific lock in the array
        snprintf(lock->lock_key_1, sizeof(lock->lock_key_1), "flag_%s_%s", bothId, char_node);
        snprintf(lock->lock_key_2, sizeof(lock->lock_key_2), "flag_%s_%s", bothId, nbr_id);
        snprintf(lock->turn_key, sizeof(lock->turn_key), "turn_%s", bothId);

        // printf("%s\n", lock->lock_key_1);
        // printf("%s\n", lock->lock_key_2);
        // printf("%s\n", lock->turn_key);

        // Acquire the lock
        while (acquire_lock(context, lock) == 0) {
            printf("Waiting for lock for node %s and neighbor %s\n", node_id, nbr_id);
            // usleep(1000); // Optional: Avoid busy-waiting
        }

        // Retrieve the color of the neighbor
        int nbr_color = get_node_color(context, nbr_id);
        nbr_color_list[nbr] = nbr_color;
    }
}

int main() {
    // Connect to Redis server
    redisContext *context = redisConnect("127.0.0.1", 6379);
    int first_node_id_of_first_task = 0; 
    int last_node_id_of_last_task = 0;
    
    if (context == NULL || context->err) {
        if (context) {
            printf("Redis connection error: %s\n", context->errstr);
        } else {
            printf("Connection error: can't allocate Redis context\n");
        }
        return 1;
    }
        
    // Define the node you want to check (node_0 in this case)
    const char *node_name = "node_3";
    int neighbour_count = 0;

    // Get all neighbors of node_0
    char **neighbour_list = get_all_neighbours(context, node_name, &neighbour_count);
    if (neighbour_list == NULL || neighbour_count == 0) {
        printf("No neighbors found for node %s\n", node_name);
        redisFree(context);
        return 1;
    }
   
    // Sort the neighbors numerically
    sort_neighbours(neighbour_list, neighbour_count);

    // Print the sorted neighbors
    printf("Sorted neighbors of %s:\n", node_name);
    for (int i = 0; i < neighbour_count; i++) {
        printf(" %s\n", neighbour_list[i]);
          // Free each neighbor string after use
    }
    ClientGraphPetersonLock *locks_array = malloc(neighbour_count * sizeof(ClientGraphPetersonLock));
    // Create the nbr_color_list of size neighbour_count (integer type)
    int *nbr_color_list = (int *)malloc(neighbour_count * sizeof(int ));
    if (nbr_color_list == NULL) {
        printf("Memory allocation error for nbr_color_list\n");
        redisFree(context);
        return 1;
    }

    
    read_color_of_neighbors(context, node_name, first_node_id_of_first_task, last_node_id_of_last_task,
                            neighbour_list, nbr_color_list, neighbour_count,locks_array);

    
   
    for (int i = 0; i < neighbour_count; i++) {
        printf("%d\n",nbr_color_list[i]);
    }
    int new_color = getMinimalValueNotInArray(nbr_color_list,neighbour_count,0,1000);
    
    set_node_color(context, node_name, new_color);
     for (int i = 0; i < neighbour_count; i++) {
        release_lock(context, &locks_array[i]);
    }
    // print_locks_array(locks_array, neighbour_count);
    return 0;
}