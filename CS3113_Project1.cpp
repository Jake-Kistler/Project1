#include <iostream>
#include <vector>
#include <tuple>
#include <fstream>
#include <algorithm>
#include <string>

using namespace std;

struct PCB
{
    int process_id;
    int state = 1; // 1 for NEW
    int max_memory_needed;
    int num_of_instructions;
    vector<tuple<int, vector<int>>> instructions;
};

// ----------------------------------------------------------------------
// Write processes to main memory
// ----------------------------------------------------------------------
void writeToMemory(vector<int> &main_memory, vector<PCB> &processes, int main_memory_size)
{
    fill(main_memory.begin(), main_memory.end(), -1); // Initialize memory to -1

    int memory_index = 0;
    for (auto &process : processes)
    {
        int required_memory = 10 + process.num_of_instructions + process.instructions.size() * 2;

        if (memory_index + required_memory > main_memory_size)
        {
            cerr << "Error: Not enough memory to allocate process " << process.process_id << endl;
            continue;
        }

        // (1) PCB region in memory
        main_memory[memory_index + 0] = process.process_id;
        main_memory[memory_index + 1] = process.state; // NEW
        main_memory[memory_index + 2] = 0;             // programCounter
        main_memory[memory_index + 3] = process.num_of_instructions;
        main_memory[memory_index + 4] = 13; // Placeholder
        main_memory[memory_index + 5] = process.max_memory_needed;
        main_memory[memory_index + 6] = 0; // cpuCyclesUsed
        main_memory[memory_index + 7] = 0; // registerValue
        main_memory[memory_index + 8] = process.max_memory_needed;
        main_memory[memory_index + 9] = memory_index; // mainMemoryBase

        // (2) Store instructions (opcodes)
        int instr_base = memory_index + 10;
        int data_base = instr_base + process.num_of_instructions;

        for (auto &instr : process.instructions)
        {
            int opcode = get<0>(instr);
            main_memory[instr_base++] = opcode;
        }

        // (3) Store instruction parameters
        for (auto &instr : process.instructions)
        {
            auto &params = get<1>(instr);
            for (auto param : params)
            {
                main_memory[data_base++] = param;
            }
        }

        memory_index += required_memory;
    }
}

// ----------------------------------------------------------------------
// Write the contents of memory to a file
// ----------------------------------------------------------------------
void writeMemoryToFile(const vector<int> &main_memory, ofstream &fout)
{
    for (size_t i = 0; i < main_memory.size(); ++i)
    {
        cout << i << ": " << main_memory[i] << endl;
        fout << i << ": " << main_memory[i] << endl;
    }
}

// ----------------------------------------------------------------------
// Execute instructions from main memory
// ----------------------------------------------------------------------
void executeCPU(int start_address, vector<int> &main_memory, ofstream &fout)
{
    int mem_size = main_memory.size();
    int memory_index = start_address; // Start execution from given address

    while (memory_index < mem_size)
    {
        int pid = main_memory[memory_index];

        // Stop if we reach uninitialized memory (-1 means empty space)
        if (pid == -1)
        {
            break;
        }

        // Read PCB fields from memory
        int &state = main_memory[memory_index + 1];
        int &programCounter = main_memory[memory_index + 2];
        int numInstructions = main_memory[memory_index + 3];
        int memoryLimit = main_memory[memory_index + 5];
        int &cpuCyclesUsed = main_memory[memory_index + 6];
        int &registerValue = main_memory[memory_index + 7];
        int instruction_base = memory_index + 10;
        int data_base = instruction_base + numInstructions;

        state = 3; // Set process state to RUNNING

        int param_index = data_base;
        for (int i = 0; i < numInstructions; i++)
        {
            int opcode = main_memory[instruction_base + i];

            if (opcode == 1)
            { // Compute
                int iterations = main_memory[param_index++];
                int cycles = main_memory[param_index++];
                cout << "compute" << endl;
                fout << "compute" << endl;
                cpuCyclesUsed += cycles;
                programCounter++;
            }
            else if (opcode == 2)
            { // Print
                int cycles = main_memory[param_index++];
                cout << "print" << endl;
                fout << "print" << endl;
                cpuCyclesUsed += cycles;
                programCounter++;
            }
            else if (opcode == 3)
            { // Store
                int value = main_memory[param_index++];
                int address = main_memory[param_index++];
                if (address < mem_size)
                {
                    main_memory[address] = value;
                    cout << "stored" << endl;
                    fout << "stored" << endl;
                }
                else
                {
                    cout << "store error!" << endl;
                    fout << "store error!" << endl;
                }
                cpuCyclesUsed += 1;
                programCounter++;
            }
            else if (opcode == 4)
            { // Load
                int address = main_memory[param_index++];
                if (address < mem_size)
                {
                    registerValue = main_memory[address];
                    cout << "loaded" << endl;
                    fout << "loaded" << endl;
                }
                else
                {
                    cout << "load error!" << endl;
                    fout << "load error!" << endl;
                }
                cpuCyclesUsed += 1;
                programCounter++;
            }
        }

        state = 4; // TERMINATED

        // Print PCB details to console and file
        string output =
            "\nPCB Contents (Stored in Main Memory):\n" +
            ("Process ID: " + to_string(pid) + "\n") +
            "State: TERMINATED\n" +
            "Program Counter: " + to_string(programCounter) + "\n" +
            "Instruction Base: " + to_string(instruction_base) + "\n" +
            "Data Base: " + to_string(data_base) + "\n" +
            "Memory Limit: " + to_string(memoryLimit) + "\n" +
            "CPU Cycles Used: " + to_string(cpuCyclesUsed) + "\n" +
            "Register Value: " + to_string(registerValue) + "\n" +
            "Max Memory Needed: " + to_string(memoryLimit) + "\n" +
            "Main Memory Base: " + to_string(memory_index) + "\n" +
            "Total CPU Cycles Consumed: " + to_string(cpuCyclesUsed) + "\n" +
            "--------------------------------------\n";

        cout << output;
        fout << output;

        // Move to the next PCB in memory
        memory_index += 10 + numInstructions + numInstructions * 2;

        // Stop if we go past valid memory range
        if (memory_index >= mem_size)
        {
            break;
        }
    }
}

// ----------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------
int main()
{
    int main_memory_size, num_of_processes;
    cin >> main_memory_size >> num_of_processes;

    vector<PCB> processes;
    vector<int> main_memory(main_memory_size, -1);

    ofstream fout("output.txt"); // Open output file once

    for (int i = 0; i < num_of_processes; ++i)
    {
        PCB process;
        cin >> process.process_id >> process.max_memory_needed >> process.num_of_instructions;
        processes.push_back(process);
    }

    writeToMemory(main_memory, processes, main_memory_size);
    writeMemoryToFile(main_memory, fout);
    executeCPU(0, main_memory, fout);

    fout.close();
    return 0;
}
