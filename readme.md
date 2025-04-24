Why Paging is used for memory Management?
Paging is a memory management technique that addresses common challenges in allocating and managing memory efficiently. Here we can understand why paging is needed as a Memory Management technique:

Memory isn’t always available in a single block: Programs often need more memory than what is available in a single continuous block. Paging breaks memory into smaller, fixed-size pieces, making it easier to allocate scattered free spaces.
Processes size can increase or decrease: programs don’t need to occupy continuous memory, so they can grow dynamically without the need to be moved.



# Process Memory Simulation

This project simulates a basic memory management system using paging. It consists of two main C programs:

1. `generate_data.c` – Generates a JSON file of processes.
2. `memoryModule.c` – Simulates memory allocation using paging based on the process data.

## Structure

```
.
├── generate_data.c
├── memoryModule.c
├── utils/
│   ├── cJSON.c
│   └── cJSON.h
└── processes.json (generated)
```

## Step-by-Step Usage

### Step 1: Generate Process Data

Compile and run `generate_data.c` to create a `processes.json` file:

```bash
gcc -std=c11 -Wall -I ./utils generate_data.c utils/cJSON.c -o GenerateData
./GenerateData
```

This creates a `processes.json` file containing an array of 14 simulated processes with the following attributes:
- `pid`: Process ID
- `arrival`: Arrival time (may include fractional values)
- `size`: Size of the process (e.g., memory requirement)

### Step 2: Run the Memory Management Simulation

Now compile and run `memoryModule.c` to simulate paging-based memory allocation:

```bash
gcc -std=c11 -Wall -I ./utils memoryModule.c utils/cJSON.c -o MemoryModule
./MemoryModule
```

This program:
- Initializes a memory block of 1024 units, divided into 64-unit frames.
- Loads each process into memory (if enough frames are available).
- If memory is full, it evicts pages using a simple round-robin replacement strategy.
- Prints the memory frame allocation at each step.

### Dependencies

- `cJSON`: A lightweight JSON parser in C. Make sure both `cJSON.c` and `cJSON.h` are available in the `utils/` directory.

## Output

The `MemoryModule` program prints a frame-by-frame visualization of memory usage, showing how each process is allocated (or evicted) over time.

## License

This project is intended for educational purposes.
