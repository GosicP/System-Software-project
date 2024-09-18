#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP
using namespace std; 
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <fstream>
#include <cctype>
#include <cstdlib>


//dodaj ReallocEntry sto je na 1:47:06

enum typeOfBranchJump { beq, bne, bgt };

enum visibility { local, global, external, section, undefined }; 
//0 local, 1 global, 2 extern, 3 section, 4 undefined

enum relocationType { local_absolute, global_absolute };

enum AdressingType { none, immed, memory, registerDirect, registerIndirect, memoryDirectLiteral, memoryDirectSymbol, regIndOffset };

struct ForwardReferencingTableEntry{
  string sectionName;
  int sectionOffset; //patch local counter
};

struct JumpRelocationEntry {
  string sectionName; //ako zelim da ga koristim, ne znam da li ce mi trebati
  int sectionOffset; //ofset od pocetka sekcije gde pravim relokaciju
  string symbolName;
};

struct RelocationTableEntry {
  int id;
  string sectionName; //ako zelim da ga koristim, ne znam da li ce mi trebati
  int sectionId;
  int sectionOffset; //ofset od pocetka sekcije gde pravim relokaciju
  string symbolName;
  int symbolId;
  int addend = 0;
  relocationType typeOfRelocation;
  //mozda da dodas ako je simbol iz druge sekcije, ime te druge sekcije umesto da stavljas u symbol name?
};

struct SymbolTableEntry{
  //section number trenutno ne koristim
  int sectionNumber;
  int id; //id datog simbola
  string sectionName; //Izaberi jedan od ova dva
  int offset; //koji je offset simbola (vrednost simbola)
  bool isDefined; //da li je simbol definisan
  //int scope; //0 local, 1 global, 2 extern, 3 section, 4 undefined
  visibility scope;
  vector<ForwardReferencingTableEntry> flink;
};

struct poolOfLiterals{
  int poolLC = 0;
  vector<int> literalList; //vrednosti literala/simbola, to su kodovi
  map<string, int> offsetInPoolForSymbol; //map of string(symbol name) and it's adress in the pool of literals
};

struct SectionEntry{
  int idSection;
  int locationCounter = 0;
  int size;
  int startAddr;
  int relocationIndexId = 0;
  vector<string> opCodeList;

  int poolLocation = 0;
  
  //formula je poolLocation-location_counter+offsetInPool
  poolOfLiterals literalPool;

  vector<JumpRelocationEntry> jumpRelocationVector;
};

class Assembler{
  public:
  map<string, SymbolTableEntry> symbolTable;
  map<string, SectionEntry> sectionTable;
  map<string, vector<RelocationTableEntry>> relocationTable;

  int symbolIndexId = 0;
  int sectionIndexId = 0;
  

  int current_data_literal = -1;
  string* current_data_symbol = nullptr;

  string current_section = "NONE";
  int location_counter = 0;
  AdressingType current_data_address =  none;
  string* current_register = nullptr;

  int branch_literal = 0;
  string* branch_symbol = nullptr;

  Assembler();
  //MORAM SVE DA PROMENIM DA DISPLACEMENT BUDE LITTLE ENDIAN
  bool matches92xF0000(const std::string& opcode);
  bool matches82Fxxxxx(const std::string& opcode); 
  bool matches3xFxxxxx(const std::string& opcode);
  void skipAssemblyDirective(int numBytes);
  //OVDE JE HARDKODOVANO DA OBRADJUJE SAMO KOD ZA JUMP, KADA DODAJES NOVE JUMO KODOVE, MORAS I OVDE DA PROMENIS IF
  //dodato da radi jedino ako su simbol i direktiva u istoj sekciji, jer ako je u drugoj sekciji
  //besmisleno je raditi FRT, takodje razmisli da isto dodas i za .word, odnosno iznad u 50.liniji
  //OVAJ IF MORA DA SE HITNO MENJA, MORA DA SE STAVI NEKA BOOLEAN VREDNOST
  //if(sectionToBacktrack.opCodeList[entry.sectionOffset/4] != "38F00000" && !matches92xF0000(sectionToBacktrack.opCodeList[entry.sectionOffset/4]))
  //else if gde treba da ne popuni adresu simbola ako nisu u istoj sekciji ne radi kada je simbol u sekciji iznad
  
  //ovde mora provera da li staje u 12 bita
  void addSymbol(string* symbol);
  
  void globalAssemblyDirective(string* globalSymbol);
  void externAssemblyDirective(string* externSymbol);
  void sectionAssemblyDirective(string* sectionSymbol); 
  //delovi worda moraju da se provere
  //PROVERI: SCOPE LOCAL I GLOBAL I EXTERN
  void wordAssemblyDirective(int literalParam, string* wordSybol);
  //start adresu ne diram, mozda bih trebao?
  void endAssemblyDirective();
  bool isInteger(const std::string& str);
  bool symbolExists(const std::vector<RelocationTableEntry>& relocationTable, std::string& symbolName);


  void haltAssemblyInstruction();
  void intAssemblyInstruction();
  

  //treba implementirati
  void storeData(int literal, string* symbol, AdressingType adressType, string* registerReg);
  string constructHexCodeForLD(int startCode, const std::string* registerSrc);

  //mora prvo push i pop register
  void callAssemblyInstruction(int literalParam, string* wordSymbol);
  void retAssemblyInstruction();

  //losa implementacija iret
  // iret ne moze da bude pop pc pop status jer ovako status nikada nece biti pop-ovan nego cemo samo skociti na mesto naznaceno pc-jem. Potrenbo je iret implementirati kao naredbu od 3 masinke sintrukcije:
  // add sp + 8
  // ld status
  // ld pc
  //treba uraditi call
  //treba uraditi ret

  void iretAssemblyInstruction();

  void jmpAssemblyInstructionLiteral(int literalParam);
  void jmpAssemblyInstructionSymbol(string* wordSymbol);

  void storeDataForBranch(int literalToStore, std::string* symbolToStore);
  string constructHexCodeForBranch(int startCode, const std::string* registerSrc, const std::string* registerDst);
  void branchAssemblyInstruction(const std::string* registerSrc, const std::string* registerDst, typeOfBranchJump typeOfBranch);

  void processLiteralForInstruction(int literalParam, const std::string& instructionCode);
  void processSymbolForInstruction(std::string* wordSymbol, const std::string& instructionCode);

  void dataLdAssemblyInstruction(string* registerDst);
  void dataStAssemblyInstruction(string* registerSrc);


  string constructHexCodeForST(int startCode, const std::string* registerSrc);
  void generateRegisterInstructionSt(int firstByte, int mmmm, std::string* registerSrc);

  void generateRegisterInstruction(int firstByte, int mmmm, std::string* registerDst);
  void generateRegisterInstructionSameRegisters(int firstByte, int mmmm, std::string* registerDst);
  void loadRegisterWithOffset(std::string* registerDst);
  void generateRegisterInstructionregIndOffset(int firstByte, int mmmm, std::string* registerSrc, int displacement); 


  void pushRegisterInstruction(string* registerSrc);
  void popRegisterInstruction(string* registerDst);
  //svi ovi jumpovi koriste bazen literala, to je na 6. predavanjima, 1:19:27

  //beq, bne, bgt
  //push, pop
  void xchgAssemblyInstruction(string* registerDst, string* registerSrc);
  void addAssemblyInstruction(string* registerDst, string* registerSrc);
  void subAssemblyInstruction(string* registerDst, string* registerSrc);
  void mulAssemblyInstruction(string* registerDst, string* registerSrc);
  void divAssemblyInstruction(string* registerDst, string* registerSrc);
  //treba uraditi not and or xor shl shr, presao sam na komplikovanije direktive
  void generateLogicRegisterInstruction(int opCode, string* registerSrc, string* registerDst);
  //nista od ovog jos nije implementirano jer je brainrot implementiranje
  void notAssemblyInstruction(string* registerOnly);
  void andAssemblyInstruction(string* registerSrc, string* registerDst);
  void orAssemblyInstruction(string* registerSrc, string* registerDst);
  void xorAssemblyInstruction(string* registerSrc, string* registerDst);
  void shlAssemblyInstruction(string* registerSrc, string* registerDst);
  void shrAssemblyInstruction(string* registerSrc, string* registerDst);

  void csrrdAssemblyInstruction(string* systemRegisterSrc, string* registerDst);
  void csrwrAssemblyInstruction(string* registerSrc, string* systemRegisterDst);

  void printSectionTable();
  void printSymbolTable();
  void printSymbolEntry(string* symbolToPrint);
  void printJumpRelocationEntries();
  void printLiteralPool(const std::string& sectionName);
  string intToLittleEndianHexString(int value);
  string modifyCodeWithDisplacement(const std::string& code, int displacement);

  void printTablesToFile(const std::string& filename);
  void printTablesToFileProba(const std::string& filename);
  void printOutputForLinker(const std::string& filename);

  //ovo konvertuje u unsigned long i vraca int, ali nigde se ne ne koristi funkcija pa ne pravi problem?
  int convertHexToInt(string* hexString);

  void splitOpCodeListInPlace(); //splits opcode in 1 byte strings (12345678 to 12 34 56 78)
  std::vector<uint8_t> hexStringToBytes(const std::vector<std::string>& opCodeList);
  
};
#endif