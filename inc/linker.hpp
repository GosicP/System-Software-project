#ifndef LINKER_HPP
#define LINKER_HPP
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <string>
#include <bitset>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

using namespace std;

enum visibilityLinker { local_linker, global_linker, external_linker, section_linker, undefined_linker }; 

struct SectionEntryLinker{
  int idSection;
  int size;
  bool isPlaced;
  int startAdress;
  vector<uint8_t> opCodeList;
};

struct SymbolTableEntryLinker{
  int sectionNumber;
  int idSymbol; //id datog simbola
  string sectionName; //Izaberi jedan od ova dva
  int offset; //koji je offset simbola (vrednost simbola)
  //int scope; //0 local, 1 global, 2 extern, 3 section, 4 undefined
  visibilityLinker scope;
};

struct symTableFileTracker{
  string sectionName;
  int inputFileIndex;
  string inputFile;
};

class Linker{
  public:
  Linker();
  map<string, SectionEntryLinker> sectionTableLinker;
  map<string, SymbolTableEntryLinker> symbolTableLinker;
  map<string, unsigned int> sectionPlaceMap;


  int symbolIndexId = 0;
  int sectionIndexId = 0;

  vector<string> notPlacedSections;
  vector<symTableFileTracker> descriptorTracker;

  unsigned long convertHexToUL(const std::string& hexStr);
  void processPlaceCommand(const std::string& placeCommand);
  void readAssembly(int argc, char* argv[]);


  void printTablesToFile(const std::string& filename);
};

#endif