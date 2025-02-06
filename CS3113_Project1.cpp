#include <iostream>
#include <vector>
#include <tuple>
#include <fstream>
#include <algorithm>

using namespace std;

struct PCB
{
    int process_id;
    int state = 1; // 1 for NEW
    int max_memory_needed;
    int num_of_instructions;
    vector<tuple<int, vector<int>>> instructions;
    // this is a vector, the vector is made of a tuple which has a op code and a vector that will hold the data for the instruction
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
void writeMemoryToFile(const vector<int> &main_memory, const string &filename)
{
    ofstream fout(filename);
    if (!fout)
    {
        cerr << "Error: Unable to open file " << filename << endl;
        return;
    }

    fout << "Main Memory:\n";
    for (size_t i = 0; i < main_memory.size(); ++i)
    {
        fout << i << ": " << main_memory[i] << endl;
    }

    fout.close();
}

// ----------------------------------------------------------------------
// Execute instructions from main memory
// ----------------------------------------------------------------------
void executeProcesses(vector<int> &main_memory, const string &filename)
{
    ofstream fout(filename, ios::app); // Append execution logs to the file
    if (!fout)
    {
        cerr << "Error: Unable to open file " << filename << endl;
        return;
    }

    int memSize = (int)main_memory.size();
    int memory_index = 0;

    while (memory_index < memSize)
    {
        int pid = main_memory[memory_index];
        if (pid == -1)
            break;

        // Read PCB fields from memory
        int &state = main_memory[memory_index + 1];
        int &programCounter = main_memory[memory_index + 2];
        int numInstructions = main_memory[memory_index + 3];
        int memoryLimit = main_memory[memory_index + 5];
        int &cpuCyclesUsed = main_memory[memory_index + 6];
        int &registerValue = main_memory[memory_index + 7];
        int instruction_base = memory_index + 10;
        int data_base = instruction_base + numInstructions;

        fout << "Executing Process ID: " << pid << endl;

        state = 3; // Set process state to RUNNING

        int param_index = data_base;
        for (int i = 0; i < numInstructions; i++)
        {
            int opcode = main_memory[instruction_base + i];

            if (opcode == 1)
            { // Compute
                int iterations = main_memory[param_index++];
                int cycles = main_memory[param_index++];
                fout << "compute" << endl;
                cpuCyclesUsed += cycles;
                programCounter++;
            }
            else if (opcode == 2)
            { // Print
                int cycles = main_memory[param_index++];
                fout << "print" << endl;
                cpuCyclesUsed += cycles;
                programCounter++;
            }
            else if (opcode == 3)
            { // Store
                int value = main_memory[param_index++];
                int address = main_memory[param_index++];
                if (address < memSize)
                {
                    main_memory[address] = value;
                    fout << "stored" << endl;
                }
                else
                {
                    fout << "store error!" << endl;
                }
                cpuCyclesUsed += 1;
                programCounter++;
            }
            else if (opcode == 4)
            { // Load
                int address = main_memory[param_index++];
                if (address < memSize)
                {
                    registerValue = main_memory[address];
                    fout << "loaded" << endl;
                }
                else
                {
                    fout << "load error!" << endl;
                }
                cpuCyclesUsed += 1;
                programCounter++;
            }
        }

        state = 4; // TERMINATED

        fout << "\nPCB Contents (Stored in Main Memory):\n";
        fout << "Process ID: " << pid << endl;
        fout << "State: TERMINATED" << endl;
        fout << "Program Counter: " << programCounter << endl;
        fout << "Instruction Base: " << instruction_base << endl;
        fout << "Data Base: " << data_base << endl;
        fout << "Memory Limit: " << memoryLimit << endl;
        fout << "CPU Cycles Used: " << cpuCyclesUsed << endl;
        fout << "Register Value: " << registerValue << endl;
        fout << "Max Memory Needed: " << memoryLimit << endl;
        fout << "Main Memory Base: " << memory_index << endl;
        fout << "Total CPU Cycles Consumed: " << cpuCyclesUsed << endl;
        fout << "--------------------------------------\n";

        memory_index += 10 + numInstructions + numInstructions * 2;
    }

    fout.close();
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

    for (int i = 0; i < num_of_processes; ++i)
    {
        PCB process;
        cin >> process.process_id >> process.max_memory_needed >> process.num_of_instructions;

        for (int j = 0; j < process.num_of_instructions; ++j)
        {
            int opcode;
            cin >> opcode;
            vector<int> params;

            if (opcode == 1)
            { // Compute
                int iterations, cycles;
                cin >> iterations >> cycles;
                params.push_back(iterations);
                params.push_back(cycles);
            }
            else if (opcode == 2)
            { // Print
                int cycles;
                cin >> cycles;
                params.push_back(cycles);
            }
            else if (opcode == 3)
            { // Store
                int value, address;
                cin >> value >> address;
                params.push_back(value);
                params.push_back(address);
            }
            else if (opcode == 4)
            { // Load
                int address;
                cin >> address;
                params.push_back(address);
            }

            process.instructions.push_back(make_tuple(opcode, params));
        }

        processes.push_back(process);
    }

    writeToMemory(main_memory, processes, main_memory_size);
    writeMemoryToFile(main_memory, "output.txt");
    executeProcesses(main_memory, "output.txt");

    return 0;
}