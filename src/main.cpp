#include "../inc/assembler.hpp"


extern int mainp(int argc, char *argv[], Assembler *a);

int main(int argc, char *argv[]) {

  Assembler *a = new Assembler();

  string input, output;

  output = argv[2];
  input = argv[3];

  cout << "input je " << input << "output je " << output;
  
  mainp(argc, argv, a);
  
  // a->printSectionTable();
  // a->printSymbolTable();
  a->printJumpRelocationEntries();
  //a->printTablesToFile("output.txt");
  // a->printTablesToFileProba("output_proba.txt");
  // a->printOutputForLinker("output_test");
  a->printTablesToFileProba("output_proba.txt");
  a->printOutputForLinker(output);
  return 0;
}
