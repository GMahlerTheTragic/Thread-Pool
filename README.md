# Simple Thread Pool using Pthreads

This project is a simple thread pool implementation using pthreads library in C. It provides a way to execute tasks asynchronously and obtain their results using a Future API.

## Project Structure

The project consists of the following main components:

- `threadpool.c` and `threadpool.h`: These files contain the implementation and interface of the thread pool.

- `future.c` and `future.h`: These files contain the implementation and interface of the Future API.

- `main.c`: This file contains a sample program that demonstrates the usage of the thread pool and Future API, parallelizing the QuickSort algorithm

- `Makefile`: This file contains the build instructions using the `make` utility.

## Prerequisites

Before running the code, ensure that you have the following prerequisites installed:

- GCC: The GNU Compiler Collection (GCC) is required to compile the C code.

- GNU Make: The `make` utility is used to build and run the program.

## Building and Running the Program

To build and run the program, follow these steps:

1. Clone or download the project repository.

2. Open a terminal or command prompt and navigate to the project's root directory.

3. Run the following command to build the program using `make`:

   ```shell
   make all
   ```
