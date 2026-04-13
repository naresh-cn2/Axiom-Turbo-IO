#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // High-precision timer
    
    FILE *file = fopen("server.log", "r");
    if (!file) {
        printf("Error: Could not open server.log\n");
        return 1;
    }

    char line[512];
    int count = 0;
    
    // Fast line-by-line reading
    while (fgets(line, sizeof(line), file)) {
        // Raw memory scan instead of slow Regex
        if (strstr(line, "\" 404 ")) {
            // Find the first space to isolate the IP address
            char *space_ptr = strchr(line, ' ');
            if (space_ptr) {
                *space_ptr = '\0'; // Terminate string right after the IP
                count++;
            }
        }
    }
    
    fclose(file);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    
    printf("Found %d 404 errors.\n", count);
    printf("Axiom C-Engine Time: %.4f seconds\n", time_taken);
    
    return 0;
}
