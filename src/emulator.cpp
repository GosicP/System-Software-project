
#include <fstream>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <vector>
#include "../inc/emulator.hpp"

using namespace std;

std::map<unsigned int, uint8_t> memory;
registersCpu registers_cpu;

// Function to print memory
void printMemory() {
    // Open the output file
    std::ofstream outputFile("emulator_output.txt");

    if (!outputFile.is_open()) {
        std::cerr << "Error: Unable to open file 'emulator_output.txt' for writing!" << std::endl;
        return;
    }

    // Iterate through memory and write to the file
    for (const auto& entry : memory) {
        outputFile << std::hex << std::setw(8) << std::setfill('0') << entry.first
                   << ": " << std::setw(2) << static_cast<unsigned int>(entry.second) << std::endl;
    }

    // Close the file
    outputFile.close();
}

// void printMemoryVector() {
//     // Open the output file
//     std::ofstream outputFile("emulator_output.txt");

//     if (!outputFile.is_open()) {
//         std::cerr << "Error: Unable to open file 'emulator_output.txt' for writing!" << std::endl;
//         return;
//     }

//     // Iterate through the vector 'mem' and write to the file
//     for (size_t address = 0; address < mem.size(); ++address) {
//         outputFile << std::hex << std::setw(8) << std::setfill('0') << address
//                    << ": " << std::setw(2) << static_cast<unsigned int>(mem[address]) << std::endl;
//     }

//     // Close the file
//     outputFile.close();
// }

uint32_t fetchWordFromMemory(unsigned int address) {
    // Prevent reading from the top of the address space
    if (address > 0xfffffffcU) return static_cast<uint32_t>(-1);

    // Read 4 consecutive bytes from memory and combine them
    uint8_t byte1 = memory[address];
    uint8_t byte2 = memory[address + 1];
    uint8_t byte3 = memory[address + 2];
    uint8_t byte4 = memory[address + 3];

    // Combine bytes into a 32-bit word in little-endian format
    return (byte1 | (byte2 << 8) | (byte3 << 16) | (byte4 << 24));
}

void storeToMemory(uint32_t regData, unsigned int addr) {
    // Prevent writing to the top of memory
    if (addr > 0xfffffffcU) return;

    // Write each byte of the 32-bit registerSrc to the map
    memory[addr]     = static_cast<uint8_t>(regData & 0xFF);            // Byte 0 (least significant)
    memory[addr + 1] = static_cast<uint8_t>((regData >> 8) & 0xFF);     // Byte 1
    memory[addr + 2] = static_cast<uint8_t>((regData >> 16) & 0xFF);    // Byte 2
    memory[addr + 3] = static_cast<uint8_t>((regData >> 24) & 0xFF);    // Byte 3 (most significant)

}

void displayProcessorState(registersCpu registers) {
    cout << "\n===============================================================\n";
    cout << "Processor halted. Current state:\n";
    for (int i = 0; i < 16; ++i) {
        cout << " r" << i << "=0x" << hex << setfill('0') << setw(8) << registers.gpr[i] << ((i % 4 == 3) ? "\n" : "\t");
    }
    cout << "===============================================================\n";
}

void handleIllegalInstruction() {
    // Log error to cerr if illegal instruction interrupt is triggered
    cerr << "Illegal instruction encountered. Halting program.\n"; 

    // Halt the program by exiting with failure status
    exit(-1);
}


int main(int argc, char* argv[]) {
  
  std::ifstream inFile(argv[1]);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return -1;
    }

  std::string line;
  while (std::getline(inFile, line)) {
      // Parse the address
      std::string addressStr = line.substr(0, 8);
      unsigned int address = std::stoul(addressStr, nullptr, 16);

      // Parse the data bytes
      int i = 10;
      while (i < line.size()) {
          std::string byteStr = line.substr(i, 2);
          uint8_t byteValue = std::stoul(byteStr, nullptr, 16);
          memory[address++] = byteValue;
          i += 3;
      }
  }

  printMemory(); 
  //nesto nesto

  inFile.close();

  //init registers to 0
  for(int i = 0; i< 15; i++) {
    registers_cpu.gpr[i] = 0;
  }

  registers_cpu.gpr[pc] = 0x40000000U;
  registers_cpu.csr[status] = 0;
  registers_cpu.csr[handler] = 0;
  registers_cpu.csr[cause] = 0;

  bool running = true;
  uint8_t opcode_instr, mode_instr, A_instr, B_instr, C_instr;
  unsigned int opcode_mode_instr; 
  uint32_t instruction;
  int displacement_instr;


  while(running) {
   
    // Fetch the instruction (32 bits from 4 consecutive bytes in memory)
    instruction = 0;
    for (int i = 0; i < 4; ++i) {
        instruction |= memory[registers_cpu.gpr[pc] + i] << ((3 - i) * 8);  // Combine 4 bytes
    }

    // Increment the program counter by 4 (32-bit instruction)
    registers_cpu.gpr[pc] += 4;

    // Extract the instruction fields
    opcode_mode_instr = (instruction & 0xff000000U) >> 24; // Opcode and mode (top 8 bits)
    A_instr = (instruction & 0x00f00000U) >> 20;       // Operand A (next 4 bits)
    B_instr = (instruction & 0x000f0000U) >> 16;       // Operand B (next 4 bits)
    C_instr = (instruction & 0x0000f000U) >> 12;       // Operand C (next 4 bits)
    displacement_instr = ((instruction & 0x00000f00U) >> 8) | ((instruction & 0x000000ffU) << 4);  // Operand D

    // Probaj i ovo
    // uint8_t lower_D = instruction & 0xFF;   // Extract the lower 8 bits of D
    // uint8_t upper_D = (instruction >> 8) & 0xF;  // Extract the upper 4 bits of D

    // displacement_instr = (upper_D << 4) | lower_D;  // Merge the two parts of D

    // Extend sign for displacement if needed (sign extension)
    if ((displacement_instr & 0x800U) != 0) {
        displacement_instr |= 0xfffff000;  // Sign extend to preserve negative values
    }

    switch (opcode_mode_instr) {
      case HALT:{
          if(A_instr == 0 && B_instr == 0 && C_instr == 0){
            running = false;
          }else{
            handleIllegalInstruction();
          }
          break;
      }
      case SWI:{
        if((registers_cpu.csr[status] & masksStatus::interruptStatus) == 0) { 
          registers_cpu.gpr[sp] -= 4;
          storeToMemory(registers_cpu.csr[status], registers_cpu.gpr[sp]); 
          registers_cpu.gpr[sp] -= 4;
          storeToMemory(registers_cpu.gpr[pc], registers_cpu.gpr[sp]); 
          //ovo mozda ne treba ovako, tako kazu na discu
          //registers_cpu.csr[status] &= ~0x1;
          registers_cpu.csr[status] |= masksStatus::interruptStatus;
          registers_cpu.csr[cause] = masksCause::software; 
          registers_cpu.gpr[pc] = registers_cpu.csr[handler]; 
        }else{
          handleIllegalInstruction();
        }
          // Handle software interrupt (SWI) instruction
          break;
      }
      case CALL_REG:{
          registers_cpu.gpr[sp] -= 4;
          storeToMemory(registers_cpu.gpr[pc], registers_cpu.gpr[sp]); //Push pc
          registers_cpu.gpr[pc] = registers_cpu.gpr[A_instr] + registers_cpu.gpr[B_instr] + displacement_instr;
          break;
      }
      case CALL_MEM:{
          registers_cpu.gpr[sp] -= 4;
          storeToMemory(registers_cpu.gpr[pc], registers_cpu.gpr[sp]); //Push pc
          long address_to_fetch = registers_cpu.gpr[A_instr] + registers_cpu.gpr[B_instr] + displacement_instr;
          registers_cpu.gpr[pc] = fetchWordFromMemory(address_to_fetch);
          break;
      }
      case JMP_REG:{
          registers_cpu.gpr[pc] = registers_cpu.gpr[A_instr] + displacement_instr;
          break;
      }
      case JMP_EQ:{
          if(registers_cpu.gpr[B_instr] == registers_cpu.gpr[C_instr]) {
            registers_cpu.gpr[pc] = registers_cpu.gpr[A_instr] + displacement_instr;
          }
          break;
      }
      case JMP_NEQ:{
          if(registers_cpu.gpr[B_instr] != registers_cpu.gpr[C_instr]) {
            registers_cpu.gpr[pc] = registers_cpu.gpr[A_instr] + displacement_instr;
          }
          break;
      }
      case JMP_GT:{
          if(static_cast<int>(registers_cpu.gpr[B_instr]) > static_cast<int>(registers_cpu.gpr[C_instr])) { //zato sto kaze signed da se uporedjuje
            registers_cpu.gpr[pc] = registers_cpu.gpr[A_instr] + displacement_instr;
          }
          break;
      }
      case JMP_MEM:{
        long address = registers_cpu.gpr[A_instr] + displacement_instr;
        registers_cpu.gpr[pc] = fetchWordFromMemory(address);
        break;
      }
      case JMP_MEM_EQ:{
        long address = registers_cpu.gpr[A_instr] + displacement_instr;
        if(registers_cpu.gpr[B_instr] == registers_cpu.gpr[C_instr]) {
          registers_cpu.gpr[pc] = fetchWordFromMemory(address);
        }
        break;
      }
      case JMP_MEM_NEQ:{
        long address = registers_cpu.gpr[A_instr] + displacement_instr;
        if(registers_cpu.gpr[B_instr] != registers_cpu.gpr[C_instr]) {
          registers_cpu.gpr[pc] = fetchWordFromMemory(address);
        }
        break;
      }
      case JMP_MEM_GT:{
        long address = registers_cpu.gpr[A_instr] + displacement_instr;
        if(static_cast<int>(registers_cpu.gpr[B_instr]) > static_cast<int>(registers_cpu.gpr[C_instr])) {
          registers_cpu.gpr[pc] = fetchWordFromMemory(address);
        }
        break;
      }
      case XCHG:{
          if (B_instr == 0 || displacement_instr == 0) {
            break;
          }
          int tmp = registers_cpu.gpr[B_instr];
          registers_cpu.gpr[B_instr] = registers_cpu.gpr[C_instr];
          registers_cpu.gpr[C_instr] = tmp;
        break;
      }
      case ADD:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] + registers_cpu.gpr[C_instr];
          break;
      }
      case SUB:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] - registers_cpu.gpr[C_instr];
          break;
      }
      case MUL:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] * registers_cpu.gpr[C_instr];
          break;
      }
      case DIV:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] / registers_cpu.gpr[C_instr];
          break;
      }
      case NOT:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = ~registers_cpu.gpr[B_instr];
          break;
      }
      case AND:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] & registers_cpu.gpr[C_instr];
          break;
      }
      case OR:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] | registers_cpu.gpr[C_instr];
          break;
      }
      case XOR:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] ^ registers_cpu.gpr[C_instr];
          break;
      }
      case SHL:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] << registers_cpu.gpr[C_instr];
          break;
      }
      case SHR:{
          if (A_instr == 0) {
            break;
          }
          if(displacement_instr != 0){
            handleIllegalInstruction();
          }
          registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] >> registers_cpu.gpr[C_instr];
          break;
      }
      case STR_REG:{
          storeToMemory(registers_cpu.gpr[C_instr], registers_cpu.gpr[A_instr] + registers_cpu.gpr[B_instr] + displacement_instr);
          break;
      }
      case STR_MEM:{
          unsigned int address = fetchWordFromMemory(registers_cpu.gpr[A_instr] + registers_cpu.gpr[B_instr] + displacement_instr);
          storeToMemory(registers_cpu.gpr[C_instr], address);
          break;
      }
      case STR_INC:{
          registers_cpu.gpr[A_instr] += displacement_instr;
          storeToMemory(registers_cpu.gpr[C_instr], registers_cpu.gpr[A_instr]);
          break;
      }
      case LDR_CSR:{
          if (A_instr != 0) {
            registers_cpu.gpr[A_instr] = registers_cpu.csr[B_instr];
          }else{
            registers_cpu.gpr[A_instr] = 0;
          }
          break;
      }
      case LDR_REG:{
          if (A_instr != 0) {
            registers_cpu.gpr[A_instr] = registers_cpu.gpr[B_instr] + displacement_instr;
          }else{
            registers_cpu.gpr[A_instr] = 0;
          }
          break;
      }
      case LDR_MEM:{
          if (A_instr != 0) {
            if(A_instr == 4){
              cout<<std::hex<< registers_cpu.gpr[B_instr]<<endl;
            }
              long address = registers_cpu.gpr[B_instr] + registers_cpu.gpr[C_instr] + displacement_instr;
              registers_cpu.gpr[A_instr] = fetchWordFromMemory(address);
          } else {
              registers_cpu.gpr[A_instr] = 0;
          }
          break;
      }
      case LDR_MEM_INC:{
          registers_cpu.gpr[A_instr] = fetchWordFromMemory(registers_cpu.gpr[B_instr]);
          registers_cpu.gpr[B_instr] += displacement_instr;
          break;
      }
      case CSR_LOAD:{
          registers_cpu.csr[A_instr] = registers_cpu.gpr[B_instr];
          break;
      }
      case CSR_OR:{
          registers_cpu.csr[A_instr] = registers_cpu.gpr[B_instr] | displacement_instr;
          break;
      }
      case CSR_MEM:{
          registers_cpu.csr[A_instr] = fetchWordFromMemory(registers_cpu.gpr[B_instr] + registers_cpu.gpr[C_instr] + displacement_instr);
          break;
      }
      case CSR_MEM_INC:{
          registers_cpu.csr[A_instr] = fetchWordFromMemory(registers_cpu.gpr[B_instr]);
          registers_cpu.gpr[B_instr] += displacement_instr;
          break;
      }
      default:{
          handleIllegalInstruction();
          break;
      }
    }
  }

  displayProcessorState(registers_cpu);
  return 0;
}