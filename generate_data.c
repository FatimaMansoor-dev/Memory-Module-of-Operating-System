#include <stdio.h>
#include <stdlib.h>
#include "utils/cJSON.h"

typedef struct {
    int   pid;        // process ID
    double arrival;   // arrival time (can be integer or fractional)
    int   size;       // process size
} Process;

int main(void) {
    // 1. Create an array of processes
    Process procs[14] = {
        {  1,  0.0, 100 },
        {  2,  1.2, 150 },
        {  3,  2.5,  80 },
        {  4,  3.0, 200 },
        {  5,  4.7, 120 },
        {  6,  5.1, 110 },
        {  7,  6.3,  90 },
        {  8,  7.8, 140 },
        {  9,  8.0, 130 },
        { 10,  9.4, 160 },
        { 11, 10.0, 170 },
        { 12, 11.6,  95 },
        { 13, 12.3, 105 },
        { 14, 13.9, 115 }
    };

    // 2. Build a cJSON array
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < 14; i++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "pid",     procs[i].pid);
        cJSON_AddNumberToObject(obj, "arrival", procs[i].arrival);
        cJSON_AddNumberToObject(obj, "size",    procs[i].size);
        cJSON_AddItemToArray(arr, obj);
    }

    // 3. Serialize to a string
    char *json_str = cJSON_Print(arr);
    if (!json_str) {
        fprintf(stderr, "Failed to print JSON.\n");
        cJSON_Delete(arr);
        return 1;
    }

    // 4. Write to file
    FILE *f = fopen("processes.json", "w");
    if (f) {
        fprintf(f, "%s\n", json_str);
        fclose(f);
        printf("Written to processes.json\n");
    } else {
        perror("fopen");
    }

    // 5. Clean up
    free(json_str);
    cJSON_Delete(arr);
    return 0;
}
