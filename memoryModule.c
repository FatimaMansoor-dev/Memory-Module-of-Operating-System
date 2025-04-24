#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>  // Add this for INT_MAX
#include "utils/cJSON.h"

typedef struct {
    int pid;         // Process ID
    double arrival;  // Arrival time
    int size;        // Size in bytes
    int pages;       // Number of pages needed
    int allocated;   // Flag to mark if process has been allocated
} Process;

void print_frames(int *memory, int num_frames) {
    // Print top border
    for (int i = 0; i < num_frames; i++) {
        printf("+----");
    }
    printf("+\n");
    
    // Print frame contents
    for (int i = 0; i < num_frames; i++) {
        if (memory[i] == 0) {
            printf("|    ");  // Empty frame
        } else {
            printf("| %2d ", memory[i]);  // Frame with process
        }
    }
    printf("|\n");
    
    // Print bottom border
    for (int i = 0; i < num_frames; i++) {
        printf("+----");
    }
    printf("+\n\n");
}

int compare_processes(const void *a, const void *b) {
    Process *p1 = (Process*)a;
    Process *p2 = (Process*)b;
    
    if (p1->arrival < p2->arrival) return -1;
    if (p1->arrival > p2->arrival) return 1;
    return 0;
}

int allocate_process(int *memory, int num_frames, int pid, int pages, 
                    int *frames_used, int *frames_free) {
    int allocated = 0;

    // Loop over the memory frames to find free ones
    for (int f = 0; f < num_frames && allocated < pages; f++) {
        if (memory[f] == 0) {
            // Free frame found
            memory[f] = pid;
            allocated++;
            (*frames_used)++;
            (*frames_free)--;
            printf("  Allocated frame %d to P%d\n", f, pid);
        }
    }
    
    return allocated;
}

void evict_frames(int *memory, int num_frames, int need, int *evict_ptr,
                int *frames_used, int *frames_free) {
    // Evict one frame at a time
    while (need > 0) {
        if (memory[*evict_ptr] != 0) {
            printf("  Evicting frame %d (was P%d)\n",
                  *evict_ptr, memory[*evict_ptr]);
            memory[*evict_ptr] = 0;

            // Update the counters 
            (*frames_used)--;
            (*frames_free)++;
            need--;
        }
        // Update the pointer to the next frame
        *evict_ptr = (*evict_ptr + 1) % num_frames;
    }
}

int main() {
    // STEP 1: Memory setup initialization
    int mem_size   = 1024;
    int frame_size = 64;
    int num_frames = mem_size / frame_size;
    int page_size  = frame_size;

    printf("Memory Module Configuration:\n");
    printf("---------------------------\n");
    printf("Total Memory: %d bytes\n", mem_size);
    printf("Frame Size: %d bytes\n", frame_size);
    printf("Total Frames: %d\n", num_frames);
    printf("Memory Management: Paging\n\n");

    // Initialize the memory module 
    int *memory = malloc(num_frames * sizeof(int));
    if (!memory) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    // Initialize memory to 0 (empty frames)
    memset(memory, 0, num_frames * sizeof(int));

    // Tracking counters
    int frames_used = 0;
    int frames_free = num_frames;
    int evict_ptr   = 0;   // Next frame index to consider for eviction

    // STEP 2: Read & parse JSON 
    FILE *fp = fopen("processes.json", "r");
    if (!fp) {
        perror("opening processes.json");
        free(memory);
        return EXIT_FAILURE;
    }
    
    // Determine file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate buffer for JSON data
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

    // Parse the JSON data
    cJSON *json = cJSON_Parse(json_text);
    free(json_text);
    if (!json) {
        fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
        free(memory);
        return EXIT_FAILURE;
    }

    int num_procs = cJSON_GetArraySize(json);
    printf("Found %d processes in JSON.\n\n", num_procs);

    // Create array to store all processes
    Process *processes = malloc(num_procs * sizeof(Process));
    if (!processes) {
        perror("malloc for processes");
        cJSON_Delete(json);
        free(memory);
        return EXIT_FAILURE;
    }

    // STEP 3: Load process data and calculate statistics
    int min_size = INT_MAX; // searching min_size
    int max_size = 0; // searching max_size
    int total_size = 0;
    
    for (int i = 0; i < num_procs; i++) {
        cJSON *proc = cJSON_GetArrayItem(json, i);
        
        processes[i].pid = cJSON_GetObjectItem(proc, "pid")->valueint;
        processes[i].arrival = cJSON_GetObjectItem(proc, "arrival")->valuedouble;
        processes[i].size = cJSON_GetObjectItem(proc, "size")->valueint;
        processes[i].pages = (processes[i].size + page_size - 1) / page_size;
        processes[i].allocated = 0;
        
        // Update statistics
        if (processes[i].size < min_size) min_size = processes[i].size;
        if (processes[i].size > max_size) max_size = processes[i].size;
        total_size += processes[i].size;
    }
    
    // Sort processes by arrival time
    qsort(processes, num_procs, sizeof(Process), compare_processes);
    
    // Display process statistics
    printf("Process Statistics:\n");
    printf("------------------\n");
    printf("Total Processes: %d\n", num_procs);
    printf("Minimum Process Size: %d bytes\n", min_size);
    printf("Maximum Process Size: %d bytes\n", max_size);
    printf("Average Process Size: %.2f bytes\n\n", (float)total_size / num_procs);
    
    // Show initial memory state
    printf("Initial Memory State:\n");
    print_frames(memory, num_frames);
    
    // STEP 4: Process arrivals in time order
    double current_time = 0.0;
    int all_processed = 0;
    
    while (!all_processed) {
        all_processed = 1;  // Assume all processes are done
        
        for (int i = 0; i < num_procs; i++) {
            // Skip if already allocated
            if (processes[i].allocated) continue;
            
            // Check if this process has arrived, basically a sanity check even though we have sorted array
            if (processes[i].arrival <= current_time) {
                int pid = processes[i].pid;
                int pages = processes[i].pages;
                
                printf("Time %.1f: Process P%d arrived (size: %d, pages: %d)\n", 
                       current_time, pid, processes[i].size, pages);
                
                // Check if eviction is needed
                if (pages > frames_free) {
                    int need = pages - frames_free;
                    printf("Not enough free frames for P%d (%d pages). Evicting %d pages...\n",
                          pid, pages, need);

                    // Perform eviction
                    evict_frames(memory, num_frames, need, &evict_ptr, 
                                &frames_used, &frames_free);

                    // Print post-eviction state
                    printf("After eviction -> used: %d, free: %d\n\n",
                          frames_used, frames_free);
                    print_frames(memory, num_frames);
                }

                // Allocate frames to this process
                allocate_process(memory, num_frames, pid, pages, 
                                &frames_used, &frames_free);

                // Print allocation details
                printf("Loaded P%d (%d pages) -> used: %d, free: %d\n\n",
                      pid, pages, frames_used, frames_free);
                print_frames(memory, num_frames);
                
                // Mark as allocated
                processes[i].allocated = 1;
            } else {
                // Found a process that hasn't arrived yet
                all_processed = 0;
            }
        }
        
        // Advance time if not all processes are allocated
        if (!all_processed) {
            current_time += 0.5;  // Increment time by 0.5 units
        }
    }

    printf("All processes allocated successfully.\n");
    printf("Final Memory State: %d frames used, %d frames free\n", frames_used, frames_free);
    print_frames(memory, num_frames);

    // Cleanup resources
    free(processes);
    cJSON_Delete(json);
    free(memory);
    return EXIT_SUCCESS;
}