#include "../inc/assembler.hpp"


extern int mainp(int argc, char *argv[], Assembler *a);

int main(int argc, char *argv[]) {

  Assembler *a = new Assembler();
  
  mainp(argc, argv, a);
  
  // a->printSectionTable();
  // a->printSymbolTable();
  a->printJumpRelocationEntries();
  a->printTablesToFile("output.txt");
  a->printTablesToFileProba("output_proba.txt");
  a->printOutputForLinker("output_test");
  return 0;
}