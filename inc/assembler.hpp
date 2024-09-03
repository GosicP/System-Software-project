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

//dodaj ReallocEntry sto je na 1:47:06

enum visibility { local, global, external, section, undefined }; 
//0 local, 1 global, 2 extern, 3 section, 4 undefined

enum relocationType { relative, absolute };

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
  int sectionOffset; //ofset od pocetka sekcije gde pravim relokaciju
  string symbolName;
  int addend;
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
  int locationCounter;
  int size;
  int startAddr;
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
  int relocationIndexId = 0;

  int current_data_literal = -1;
  string* current_data_symbol = nullptr;

  string current_section = "NONE";
  int location_counter = 0;

  Assembler();
  bool matches92xF0000(const std::string& opcode);
  void skipAssemblyDirective(int numBytes);
  //OVDE JE HARDKODOVANO DA OBRADJUJE SAMO KOD ZA JUMP, KADA DODAJES NOVE JUMO KODOVE, MORAS I OVDE DA PROMENIS IF
  //dodato da radi jedino ako su simbol i direktiva u istoj sekciji, jer ako je u drugoj sekciji
  //besmisleno je raditi FRT, takodje razmisli da isto dodas i za .word, odnosno iznad u 50.liniji
  //OVAJ IF MORA DA SE HITNO MENJA, MORA DA SE STAVI NEKA BOOLEAN VREDNOST
  //if(sectionToBacktrack.opCodeList[entry.sectionOffset/4] != "38F00000" && !matches92xF0000(sectionToBacktrack.opCodeList[entry.sectionOffset/4]))
  void addSymbol(string* symbol);
  
  void globalAssemblyDirective(string* globalSymbol);
  void externAssemblyDirective(string* externSymbol);
  void sectionAssemblyDirective(string* sectionSymbol); 
  //delovi worda moraju da se provere
  //PROVERI: SCOPE LOCAL I GLOBAL I EXTERN
  void wordAssemblyDirective(int literalParam, string* wordSybol);
  //start adresu ne diram, mozda bih trebao?
  void endAssemblyDirective();


  void haltAssemblyInstruction();
  void intAssemblyInstruction();
  //losa implementacija iret
  void iretAssemblyInsruction();
  //treba uraditi call
  //treba uraditi ret

  //treba implementirati
  void storeData(int literal, string* symbol);
  string constructHexCodeForLD(int startCode, const std::string* registerSrc);

  void jmpAssemblyInstructionLiteral(int literalParam);
  void jmpAssemblyInstructionSymbol(string* wordSymbol);

  void processLiteralForInstruction(int literalParam, const std::string& instructionCode);
  void processSymbolForInstruction(std::string* wordSymbol, const std::string& instructionCode);

  void dataLdAssemblyInstruction(string* registerDst);


  //svi ovi jumpovi koriste bazen literala, to je na 6. predavanjima, 1:19:27

  //beq, bne, bgt
  //push, pop
  void xchgAssemblyInstruction(string* registerDst, string* registerSrc);
  void addAssemblyInstruction(string* registerDst, string* registerSrc);
  void subAssemblyInstruction(string* registerDst, string* registerSrc);
  void mulAssemblyInstruction(string* registerDst, string* registerSrc);
  void divAssemblyInstruction(string* registerDst, string* registerSrc);
  //treba uraditi not and or xor shl shr, presao sam na komplikovanije direktive

  //nista od ovog jos nije implementirano jer je brainrot implementiranje
  void notAssemblyInstruction(string* registerOnly);
  void andAssemblyInstruction(string* registerDst, string* registerSrc);
  void orAssemblyInstruction(string* registerDst, string* registerSrc);
  void xorAssemblyInstruction(string* registerDst, string* registerSrc);
  void shlAssemblyInstruction(string* registerDst, string* registerSrc);
  void shrAssemblyInstruction(string* registerDrc, string* registerSrd);

  //vodic asembler txt iskustvo

  //ld i st sa [reg + symbol] je nemoguce uraditi ako nemate .equ odradejno tako da je to samo za nivo C !
  //Da li se ovo odnosi i na jmp?

  // ako imate instrukciju: ld symbol, reg nju morate da relaizujete kao 2 insutrkcije:
  // ld symbol, %r3
  // prva instrukcija je ld koja dovlaci vrednost simbola u registar r3
  // druga insturkcija je koja ceta iz memorije sa vrendoscu r3 u r3 tj -> ld [%r3], %r3

//   I ovako dovlacite simbol koriteci bazen literala. 

// Nije totalno simetricno za st, posto st ima dodatne nacine rada sa memorijom tj:

// mem[mem[B + C + D]] <= A ->  i  onda nema potrebe za ovom indikrejciom

// 3) Insturkcije skoka su manje vise sve identicne samo imaju razliku u op_code i mode.

// 4) Problem sa bazenom literala:

// Moze da nastane sledeci problem:

// ld symbol, %r3 

// ... -> mnogo linija koda 

// ... -> ovde treba da se nalepi bazen 

// Problem ej ako je bazen na razdaljini > 2^11 bit-a onda nemozemo relativnim adresiranjem od insturkcije da dohvatimo vrednost iz bazena.

// Imamo par resenja za to, da detektujemo kada vise ne moze zbog offseta zabelezimo jedan bazen, stavimo intrukciju skoka da ga prekosci
// i krenemo da zapisujemo novi kod.

// Ili da bacamo gresku "section too big"

// Drugo resenje je dosta brze ali ja licno ne mogu da kazem da li bi na odbrani ikada zamerali za nesto ovako. 
  //5) iret mora kao 3 insturkcije, add / ld / ld 

  
  void printSectionTable();
  void printSymbolTable();
  void printSymbolEntry(string* symbolToPrint);
  void printJumpRelocationEntries();
  void printLiteralPool(const std::string& sectionName);
  string intToLittleEndianHexString(int value);
  string modifyCodeWithDisplacement(const std::string& code, int displacement);

  void printTablesToFile(const std::string& filename);

  int convertHexToInt(string* hexString);
  
};
#endif