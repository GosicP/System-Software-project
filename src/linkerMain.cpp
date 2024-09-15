#include <iostream>
#include <vector>
#include "../inc/linker.hpp"

using namespace std;

int main(int argc, char** argv){

  Linker *l = new Linker();


  l->readAssembly(argc, argv);

  l->printTablesToFile("output_linker.txt");
  
  return 0;
}