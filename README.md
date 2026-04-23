# RFC 9213 Compliant Modular API Gateway Priority Request Scheduler

This project implements a high-performance **Priority Request Scheduler** for an API Gateway, strictly adhering to the **RFC 9213 (Extensible HTTP Priorities)** standard. The system is designed to parse dynamic HTTP priority headers and organize them using a **Min-Heap** based Priority Queue to ensure optimal resource allocation and low-latency processing.

## Overview

In modern networking architectures, managing high-volume traffic at the API Gateway level is critical. This scheduler prioritizes incoming requests based on their `urgency` and `incremental` parameters defined in the RFC 9213 standard. By utilizing a Min-Heap data structure, the system ensures that critical requests (e.g., payments, authentication) are prioritized over non-essential background tasks with logarithmic time complexity.

## Technical Key Features

* **RFC 9213 Parser:** A robust parsing engine that extracts `u` (urgency) and `i` (incremental) parameters from HTTP headers and maps them to a mathematical priority key.
* **Min-Heap Implementation:** Efficient priority management utilizing **Heapify-Up (Bubble-Up)** and **Heapify-Down** algorithms to maintain O(log N) performance for both insertion and extraction.
* **Modular C Architecture:** Strict separation of concerns using a header/source (`.h` / `.c`) architectural pattern, promoting encapsulation and maintainability.
* **Dynamic Memory Management:** Manual heap allocation using `malloc`, `realloc`, and `free` to handle fluctuating traffic loads without memory leaks.
* **Technical Standardization:** Adherence to professional coding standards, including zero-leak memory policies and Big-O performance optimization.

## Directory Structure

```text
.
├── include/
│   ├── priority_queue.h  # Definitions for the Heap-based Priority Queue
│   └── rfc9213_parser.h  # Definitions for the RFC 9213 header parser
├── src/
│   ├── main.c             # Orchestration and demo simulation scenarios
│   ├── priority_queue.c   # Logic for heap operations and memory scaling
│   └── rfc9213_parser.c   # Logic for string tokenization and priority mapping
└── scheduler.exe          # Compiled executable
