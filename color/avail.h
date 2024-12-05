#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

int check_if_available(char **ip_list, int ip_count, int node1, int node2);
int acquire_lock(redisContext *context, int node1, int node2);
void release_lock(redisContext *context, int node1, int node2);
int set_turn_variable(redisContext *context, int node1, int node2) ;
