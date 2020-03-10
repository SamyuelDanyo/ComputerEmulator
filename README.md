# ComputerEmulator
Von Neumann computer emulator with Debug mode.
| INTRODUCTION: |
-----------------
The computer emulator implements the functionality of a Von Neumann machine, fetching and executing from memory,
extended with a cache memory. The emulator is able to process simple programs through its instruction set, displaying
the exact cause, place and data in the machine if an error occurs. Moreover, it allows the use of a debug mode,
which captures the value of all internal signals, register, validations, functions and errors, presenting the opportunity
of full analysis of the computer’s functions. 

The von Neumann architecture uses a single memory for data and program instructions, which lets the program to alter its own code.
The system’s flow is facilitated by the control unit- kind of a state machine, issuing signals to control the
communication between registers, memory, ALU.

| Execution: |
--------------
# Omit< -Ddebug=1 > if you do not wish to see the debug mode execution.
Linux/Unix: < g++ -o ComputerEmulator ComputerEmulator.cpp -Ddebug=1 >
Windows: < cl /EHsc ComputerEmulator.cpp /Ddebug >

| ISA Design: |
---------------
The word length of the CPU is 16 bits. If it is an instruction – the first byte is the op-code, the second is the data/address.
The focus was on simple functions:
  LOAD_A – loading from an address,
  LOAD_N0 – loading a number in the more significant byte of the accumulator,
  LOAD_N1 – loading a number in the less significant byte of the accumulator,
  STORE – storing the value of the ACC in an address,
  ADD_A – adding the content of an address to the value of the ACC,
  ADD_N – adding a number,
  SUB_A – subtract from an address,
  SUB_N – subtract a number,
  JUMP – jump to an address,
  PROCESS – turns off the signal (processing) - shutting down,
  DN – doing nothing for a cycle,
  RESET – resets the registers (Not the RAM, CACHE) – starts the program from the beginning,
  RELOAD – reloads the program(initialising, RAM & CACHE).
  
| Register Topology & Internal Signals Design: |
-------------------------------------------------
I stuck to simple architecture: Instruction Register (IR), Program Counter (PC), Accumulator (ACC),
Random Access Memory (RAM), one level CACHE with data (D_CACHE) and instruction (I_CACHE) sub-caches &
a number of signals/registers facilitating the CPU function.
  
The Instruction register (IR) holds the program instruction being executed in the cycle
(in the second byte is the op-code, in the first byte is the number or address that needs to be accessed during execution).
The Program Counter (PC) holds the address of the program instruction, being executed.
The Accumulator (ACC) holds data from previous instruction or a number/data from an address being loaded in the cycle.
The RAM is the main memory of the computer, storing the program instructions and the data.
The CACHE is a secondary memory (much faster), which holds the most used instructions and data – speeding up the CPU’s processing.
I use associative mapping with FIFO replacement procedure.
The cache_counters are holding the available line for storing new information.
The temporary register (temp) holds a temporary value that is used in a mathematical function or loading.
The processing signal determines if the control unit will function, if it is true – the CPU is working,
if – false, the CPU shuts down. Overflow is a signal going true if in addition function the data is too big to
hold in the RAM – the data topology and signals gets corrupted and the CPU stops working. Underflow is analogical to 
overflow but for subtracting. The RAM has 256 address of two-byte size. This shows the meaning of the max_data_cap of a
single entry in the memory, the max_addresses are initialised to values, which assume full used of the memory and
equal program memory and data size. Those are being changed when loading a particular program, according to its size and
use of memory. The IR and ACC are both 2-byte sized, since they hold entries of the RAM (instructions or data).
The PC is 1-byte sized, since the max address in the RAM is 255. Both sub-caches have ten lines, holding tag
(the address of the memory block) and ten memory blocks, each comprised of four instructions/data entries),
building up the CACHE to being able to store forty instructions and forty data entries. The cache_counters
are 1-byte sized, since they hold the cache line address.

| Control Unit Design: |
------------------------
The control unit’s sequencer has three distinct stages. First – fetching the program instruction,
second – decoding the instruction (matching the op-code and so, understanding what needs to be done in execution),
and last – execution of the operation, which flow depends on the operation and incrementing the PC
(setting the address for the next instruction). The last stage can me omitted or changed for different op-codes.
After execution, the sequencer goes back to checking if processing is one, if yes, the cycle starts again. 
