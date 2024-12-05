#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>  // Include time.h for measuring time

typedef struct {
    int node1;
    int node2;
} Edge;

int edgeExists(Edge *edges, int edgeCount, int node1, int node2) {
    for (int i = 0; i < edgeCount; i++) {
        if ((edges[i].node1 == node1 && edges[i].node2 == node2) ||
            (edges[i].node1 == node2 && edges[i].node2 == node1)) {
            return 1;
        }
    }
    return 0;
}

int main() {
    // Start measuring time
    clock_t startTime = clock();

    FILE *inputFile = fopen("/home/abhattar/Desktop/redis/redis/color/graphs/graph_dblp_coauthorship_n317080.txt", "r");  // Replace with your input file path
    FILE *outputFile = fopen("/home/abhattar/Desktop/redis/redis/color/graphs/edge_graph_dblp_coauthorship_n317080.txt", "w");  // Replace with your output file path

    if (inputFile == NULL || outputFile == NULL) {
        fprintf(stderr, "Error: Could not open input or output file.\n");
        return 1;
    }

    int initialCapacity = 1000;  // Starting capacity for edges
    int edgeCount = 0;
    Edge *edges = (Edge *)malloc(initialCapacity * sizeof(Edge));
    if (edges == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(inputFile);
        fclose(outputFile);
        return 1;
    }

    char line[10000];  // To read each line of the input
    while (fgets(line, sizeof(line), inputFile) != NULL) {
        // Skip header and empty lines
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        int node;
        char *token = strtok(line, " ");
        
        // Read the first node
        node = atoi(token);

        // Read the neighbors
        while ((token = strtok(NULL, " ")) != NULL) {
            int neighbor = atoi(token);
            if (node < neighbor && !edgeExists(edges, edgeCount, node, neighbor)) {
                if (edgeCount >= initialCapacity) {
                    initialCapacity *= 2;
                    Edge *newEdges = (Edge *)realloc(edges, initialCapacity * sizeof(Edge));
                    if (newEdges == NULL) {
                        fprintf(stderr, "Memory reallocation failed.\n");
                        free(edges);
                        fclose(inputFile);
                        fclose(outputFile);
                        return 1;
                    }
                    edges = newEdges;
                }
                edges[edgeCount].node1 = node;
                edges[edgeCount].node2 = neighbor;
                edgeCount++;
            }
        }
    }

    // Write the unique edges to the output file
    for (int i = 0; i < edgeCount; i++) {
        fprintf(outputFile, "%d,%d\n", edges[i].node1, edges[i].node2);
    }

    // Clean up
    free(edges);
    fclose(inputFile);
    fclose(outputFile);

    // End measuring time and calculate the elapsed time
    clock_t endTime = clock();
    double timeTaken = (double)(endTime - startTime) / CLOCKS_PER_SEC;

    // Print the elapsed time
    printf("Edge list has been saved to edge_list.txt\n");
    printf("Processing time: %.2f seconds\n", timeTaken);

    return 0;
}
