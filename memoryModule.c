#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/cJSON.h"

// Function for pretty printing >~<
void print_frames(int *memory, int num_frames) {
    for (int i = 0; i < num_frames; i++)  printf("+----");
    printf("+\n");
    // Content of each frame
    for (int i = 0; i < num_frames; i++) {
        if (memory[i] == 0)    printf("|   ");
        else                   printf("| %2d ", memory[i]);
    }
    printf("|\n");
    for (int i = 0; i < num_frames; i++)  printf("+----");
    printf("+\n\n");
}

int main() {
    // Step 1 : Key Initializations of memory setup
    int mem_size   = 1024;
    int frame_size = 64;
    int num_frames = mem_size / frame_size;
    int page_size  = frame_size;

    // Initialize the memory module 
    int *memory = malloc(num_frames * sizeof(int));
    if (!memory) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    // Initialize memory to 0 (empty frames)
    memset(memory, 0, num_frames * sizeof(int));

    // Helping counters
    int frames_used = 0;
    int frames_free = num_frames;
    int evict_ptr   = 0;   // next frame index to consider for eviction

    // Step 2 : Read & parse JSON 
    FILE *fp = fopen("processes.json", "r");
    if (!fp) {
        perror("opening processes.json");
        free(memory);
        return EXIT_FAILURE;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate a buffer large enough to hold the file plus a terminator
    char *json_text = malloc(file_size + 1);
    if (!json_text) {
        perror("malloc for JSON text");
        free(memory);
        fclose(fp);
        return EXIT_FAILURE;
    }

    // Read the file into the buffer
    fread(json_text, 1, file_size, fp);
    json_text[file_size] = '\0';
    fclose(fp);

    // Parse teh json
    cJSON *json = cJSON_Parse(json_text);
    free(json_text);
    if (!json) {
        fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
        free(memory);
        return EXIT_FAILURE;
    }

    int num_procs = cJSON_GetArraySize(json);
    printf("Found %d processes in JSON.\n\n", num_procs);

    // Step 3 : Loop over each process 
    for (int i = 0; i < num_procs; i++) {
        cJSON *proc = cJSON_GetArrayItem(json, i);

        // Get its initials
        int    pid  = cJSON_GetObjectItem(proc, "pid")->valueint;
        int    size = cJSON_GetObjectItem(proc, "size")->valueint;
        int    pages= (size + page_size - 1) / page_size; // ceiling division

        // Evict if needed
        if (pages > frames_free) {
            int need = pages - frames_free;
            printf("Not enough free frames for P%d (%d pages). Evicting %d pages...\n",
                   pid, pages, need);

            // evict one frame at a time
            while (need > 0) {
                if (memory[evict_ptr] != 0) {
                    printf("  Evicting frame %d (was P%d)\n",
                           evict_ptr, memory[evict_ptr]);
                    memory[evict_ptr] = 0;

                    // update the counters 
                    frames_used--;
                    frames_free++;
                    need--;
                }
                // Update the pointer to the next frame
                evict_ptr = (evict_ptr + 1) % num_frames;
            }

            // print details
            printf("After eviction -> used: %d, free: %d\n\n",
                   frames_used, frames_free);
            print_frames(memory, num_frames);
        }

        // Allocation phase
        int allocated = 0;

        // Loop over the memory frames to find free ones, stop when process is allocated completely
        for (int f = 0; f < num_frames && allocated < pages; f++) {
            if (memory[f] == 0) {
                // free frame found
                memory[f] = pid; // assign the process ID to the frame
                // update counters 
                allocated++;
                frames_used++;
                frames_free--;
                printf("  Allocated frame %d to P%d\n", f, pid);
            }
        }

        // Print details
        printf("Loaded P%d (%d pages) -> used: %d, free: %d\n\n",
               pid, pages, frames_used, frames_free);
        print_frames(memory, num_frames);
    }

    // Cleanup 
    cJSON_Delete(json);
    free(memory);
    return EXIT_SUCCESS;
}
