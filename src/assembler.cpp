#include "../inc/assembler.hpp"

Assembler::Assembler() : current_section("NONE"), location_counter(0) {}


void Assembler::skipAssemblyDirective(int numBytes){
  std::cout<<"preskacem za: "<< numBytes << "bajtova";
  for(int i = 0; i<numBytes; i++){
    //ovaj opCodeList mozes da obrises ako zelis da ti radi i kada imas skip 2 npr
    sectionTable[current_section].opCodeList.push_back("00");
  }
  location_counter += numBytes;
}

bool Assembler::matches92xF0000(const std::string& opcode) {
    // Define the regex pattern: ^92[A-F0-9]F0000$
    std::regex pattern("^92[A-Fa-f0-9]F0000$");

    // Check if the opcode matches the pattern
    return std::regex_match(opcode, pattern);
}

bool Assembler::matches82Fxxxxx(const std::string& opcode) {
    // Define the regex pattern: ^82[Ff][A-Fa-f0-9]{5}$
    std::regex pattern("^82[Ff][A-Fa-f0-9]{5}$");

    // Check if the opcode matches the pattern
    return std::regex_match(opcode, pattern);
}

bool Assembler::matches3xFxxxxx(const std::string& opcode) {
    // Define the regex pattern: ^3[9AaBb]F[A-Fa-f0-9]{5}$
    std::regex pattern("^3[9AaBb]F[A-Fa-f0-9]{5}$");

    // Check if the opcode matches the pattern
    return std::regex_match(opcode, pattern);
}

void Assembler::addSymbol(string *symbol)
{
  if (symbolTable.find(*symbol) == symbolTable.end()) {
    // Symbol does not exist, znaci da je simbol lokalan i nedefinisan
    std::cout << "Symbol not found in the map!" << endl;
    SymbolTableEntry newSymbol;
    newSymbol.sectionName = current_section;
    newSymbol.sectionNumber = sectionTable[current_section].idSection;
    newSymbol.scope = local;
    newSymbol.offset = location_counter;
    newSymbol.id = ++symbolIndexId;
    newSymbol.isDefined = true;
    symbolTable[*symbol] = newSymbol;
  } else {
    // Symbol exists
    cout << "Symbol exists in the map!" << endl;
    SymbolTableEntry& current_symbol = symbolTable[*symbol];
    if(current_symbol.isDefined==true) {
      cerr << "Symbol" + *symbol + "is already defined!" << endl;
      exit(-1);
    }
    //if(current_symbol.sectionName == "NONE"){ //current_symbol.sectionName == "NONE", zamenjen uslov, 
    //sada ce uvek da udje ovde ako je isDefined = false
    if(current_symbol.scope == global || current_symbol.scope == external){
      //odnosno samo ovaj deo se desi za global i extern, a za undefined i ostalo
      current_symbol.isDefined=true;
      current_symbol.sectionName = current_section;
      current_symbol.sectionNumber = sectionTable[current_section].idSection;
      current_symbol.offset = location_counter;
    } 
    if(current_symbol.scope == undefined){
      current_symbol.isDefined=true;
      current_symbol.sectionName = current_section;
      current_symbol.sectionNumber = sectionTable[current_section].idSection;
      current_symbol.offset = location_counter;
      current_symbol.scope = local;
      //MOZDA DA DODAS DA SE OVO RADI SAMO AKO SU SIMBOL I DIREKTIVA U ISTOJ SEKCIJI
      vector<ForwardReferencingTableEntry> current_flink = current_symbol.flink;
        for (const auto& entry : current_flink) {
          string newCode = intToLittleEndianHexString(current_symbol.offset);

          SectionEntry& sectionToBacktrack = sectionTable[entry.sectionName];
          if(sectionToBacktrack.opCodeList[entry.sectionOffset/4] != "38F00000" 
          && !matches92xF0000(sectionToBacktrack.opCodeList[entry.sectionOffset/4])
          && !matches82Fxxxxx(sectionToBacktrack.opCodeList[entry.sectionOffset/4])
          && sectionToBacktrack.opCodeList[entry.sectionOffset/4] != "21F00000" 
          && !matches3xFxxxxx(sectionToBacktrack.opCodeList[entry.sectionOffset/4])){
            //USAO SAM U OBRADU ZA WORD
            sectionToBacktrack.opCodeList[entry.sectionOffset/4] = newCode;
            //dodato da radi jedino ako su simbol i direktiva u istoj sekciji, jer ako je u drugoj sekciji
            //besmisleno je raditi FRT, takodje razmisli da isto dodas i za .word, odnosno iznad u 50.liniji
          }else if(current_section==entry.sectionName){
            cout << "Usao sam u obradu sa jump";
            // Create a reference to the literal pool
            poolOfLiterals& literalPoolForSection = sectionToBacktrack.literalPool;

            // Access the offset associated with the symbol
            int offsetValue = literalPoolForSection.offsetInPoolForSymbol[*symbol]; 

            // Perform the division to calculate the index
            int index = offsetValue / 4;

            // Use the result to modify the literal list at the calculated index
            literalPoolForSection.literalList[index] = current_symbol.offset;
          }
        }
        // for (auto& entry : relocationTable) {
        // const string& sectionName = entry.first;  // The section name (key)
        // vector<RelocationTableEntry>& relocationEntries = entry.second;  // The vector of RelocationTableEntry objects

        // // Iterate through all RelocationTableEntry objects in the vector
        // for (auto& relocationEntry : relocationEntries) {
        //    if(relocationEntry.symbolName==*symbol)
        //     // Update the symbolName and addend as needed
        //     // zasto da updateujem symbolName na section name?
        //     //relocationEntry.symbolName = current_symbol.sectionName;
        //     //relocationEntry.sectionId = current_symbol.sectionNumber;
        //     relocationEntry.addend += current_symbol.offset;
        //   }
        // }
      }

      }
    }

void Assembler::globalAssemblyDirective(string* globalSymbol){
  if(symbolTable.find(*globalSymbol) != symbolTable.end()){
    //provera da li je ekterni vec
    if(symbolTable[*globalSymbol].scope = external) {
      cerr << "Symbol " + *globalSymbol + " is already defined!" << endl;
      exit(-1);
    }else{symbolTable[*globalSymbol].scope = global;
    //mozda da dodam ovde da defined stavim na true i offset na LC, u zavisnosti od implementacije addSybmola
    }
  }else{
    //0 local, 1 global, 2 extern, 3 section, 4 undefined
    SymbolTableEntry newSymbol;
    newSymbol.sectionName = "NONE"; //mozda treba current_section?
    newSymbol.sectionNumber = 0;
    newSymbol.scope = global;
    newSymbol.offset = -1;
    newSymbol.id = ++symbolIndexId;
    newSymbol.isDefined = false;
    symbolTable[*globalSymbol] = newSymbol;
  }
}

void Assembler::externAssemblyDirective(string* externSymbol) {
  if(symbolTable.find(*externSymbol) != symbolTable.end()){
    symbolTable[*externSymbol].scope = external;
  }else{
    SymbolTableEntry newSymbol;
    newSymbol.sectionName = "NONE"; //mozda treba current_section?
    newSymbol.sectionNumber = 0;
    //0 local, 1 global, 2 extern, 3 section, 4 undefined
    newSymbol.scope = external;
    newSymbol.offset = -1;
    newSymbol.id = ++symbolIndexId;
    newSymbol.isDefined = false;
    symbolTable[*externSymbol] = newSymbol;
  }
}

void Assembler::sectionAssemblyDirective(string* sectionSymbol){
  if (current_section != "NONE") {
    SectionEntry& currentSection = sectionTable[current_section];
    cout<<"ime trenutne sekcije" << currentSection.idSection <<endl;
    currentSection.locationCounter += location_counter;
    currentSection.size = currentSection.locationCounter;
    currentSection.poolLocation = currentSection.locationCounter; 
    cout<< currentSection.locationCounter <<endl;
    cout<< currentSection.size <<endl;
    cout<< "Trebalo bi da sam promenio LC i size"<<endl;
  } 

  if(symbolTable.find(*sectionSymbol) != symbolTable.end()){
    //sekcija postoji u tabeli simbola
    SectionEntry currentSection = sectionTable[*sectionSymbol];
    location_counter = currentSection.locationCounter;
    currentSection.size = currentSection.locationCounter;
    currentSection.poolLocation = currentSection.locationCounter; 
    current_section = *sectionSymbol;
    cout<< "Izmenio sam parametre za kraj simbola u sekciji"<<endl;
  }else{
    //sekcija ne postoji u tabeli simbola
    current_section = *sectionSymbol;
    SymbolTableEntry newSymbol;
    newSymbol.sectionName = *sectionSymbol; //ja sam stavio da ima isto ime i section name, sto znaci da moze da se koristi za ispitivanje da li je sekcija, iako je to u scope
    newSymbol.sectionNumber = ++sectionIndexId;
    //0 local, 1 global, 2 extern, 3 section, 4 undefined
    newSymbol.scope = section;
    newSymbol.offset = -1;
    newSymbol.id = ++symbolIndexId;
    newSymbol.isDefined = true; //mozda i treba da bude false?
    symbolTable[*sectionSymbol] = newSymbol;

    SectionEntry newSection;
    newSection.idSection = sectionIndexId;
    newSection.size = 0;
    newSection.locationCounter = 0;
    newSection.relocationIndexId = 0;
    sectionTable[*sectionSymbol] = newSection;
    //FALI MAPA RELOKACIJA

    // RelocationTableEntry newRelocationEntry;
    // newRelocationEntry.id = ++newSection.relocationIndexId;  // Assuming relocationEntryId is an existing variable tracking IDs
    // newRelocationEntry.sectionName = current_section;  // Replace with the appropriate section name or leave as default if not used
    // newRelocationEntry.sectionId = sectionIndexId;
    // newRelocationEntry.symbolId = symbolIndexId;
    // newRelocationEntry.sectionOffset = 0;  // Initialize with 0 or another appropriate value
    // newRelocationEntry.addend = 0;  // Initialize with 0 or another appropriate value

    // relocationTable[current_section].push_back(newRelocationEntry);

    location_counter = 0;
    cout<<"zavrsio sam pravljenje nove sekcije"<<endl;
  }
}

void Assembler::wordAssemblyDirective(int literalParam, string* wordSymbol) {
  if(wordSymbol==nullptr){ //mozda ovde treba wordSymbol != nullptr?
    string hexLiteral = intToLittleEndianHexString(literalParam);
    sectionTable[current_section].opCodeList.push_back(hexLiteral);
    location_counter += 4;
  }else{
    if (symbolTable.find(*wordSymbol) == symbolTable.end()){
      //simbol ne postoji u tabeli simbola, znaci da mora da se referise unapred
      
      SymbolTableEntry newSymbol;
      newSymbol.sectionName = "NONE";
      newSymbol.sectionNumber = 0;
      newSymbol.scope = undefined;
      newSymbol.offset = -1;
      newSymbol.id = ++symbolIndexId;
      newSymbol.isDefined = false;
      
      sectionTable[current_section].opCodeList.push_back("00000000");
      
      ForwardReferencingTableEntry newFRT;
      newFRT.sectionName = current_section;
      newFRT.sectionOffset = location_counter;

      newSymbol.flink.push_back(newFRT);
      symbolTable[*wordSymbol] = newSymbol;

      //oov ne obradjuje ludi slucaj da je ovde referenciran simbol koji ce kasnije postati global/extern
      //nigde u testovima to nema , i u vecini asemblerkih jezika je to nemoguce, tako da necu ni pisati
      //tretiram ga ovde kao da je lokalni simbol  

      RelocationTableEntry rtEntry;
      rtEntry.id = ++sectionTable[current_section].relocationIndexId;
      rtEntry.sectionName = current_section;
      rtEntry.sectionOffset = location_counter;
      rtEntry.sectionId = sectionTable[current_section].idSection;
      rtEntry.symbolId = newSymbol.id;
      rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
      rtEntry.addend = 0;
      rtEntry.typeOfRelocation = local_absolute; /* set the appropriate relocation type */
      relocationTable[current_section].push_back(rtEntry);

      // RelocationTableEntry rtEntry;
      // rtEntry.id = ++sectionTable[current_section].relocationIndexId;
      // rtEntry.sectionName = current_section;
      // rtEntry.sectionOffset = location_counter;
      // rtEntry.sectionId = sectionTable[current_section].idSection;
      // rtEntry.symbolId = symbolTable[*wordSymbol].id;
      // rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
      // rtEntry.addend = 0;
      // rtEntry.typeOfRelocation = global_absolute; /* set the appropriate relocation type */
      // relocationTable[current_section].push_back(rtEntry);
      
      //treba dodati tabelu relokacija
    }else{
      SymbolTableEntry& definedSymbol = symbolTable[*wordSymbol];
      if(definedSymbol.scope == local){
        //PROVERI OVAJ DEO
        string newCode;
        SectionEntry& currentSection = sectionTable[current_section];
        if (current_section==definedSymbol.sectionName) {
            newCode = intToLittleEndianHexString(definedSymbol.offset);
          }else{
            newCode = "00000000";
            
            RelocationTableEntry rtEntry;
            rtEntry.id = ++sectionTable[current_section].relocationIndexId;
            rtEntry.sectionName = current_section;
            rtEntry.sectionOffset = location_counter;
            rtEntry.sectionId = sectionTable[current_section].idSection;
            rtEntry.symbolId = currentSection.idSection;
            rtEntry.symbolName = definedSymbol.sectionName; // Assuming wordSymbol is the symbol name
            rtEntry.addend += definedSymbol.offset;
            rtEntry.typeOfRelocation = local_absolute; /* set the appropriate relocation type */
            relocationTable[current_section].push_back(rtEntry);
        }
        
        currentSection.opCodeList.push_back(newCode);
        
        //Ne znam da li treba relokacija ali stavio sam je
        // RelocationTableEntry rtEntry;
        // rtEntry.id = ++currentSection.relocationIndexId;
        // rtEntry.sectionName = definedSymbol.sectionName;
        // rtEntry.sectionOffset = location_counter;
        // rtEntry.sectionId = definedSymbol.sectionNumber;
        // rtEntry.symbolId = definedSymbol.id;
        // rtEntry.symbolName = definedSymbol.sectionName; // Assuming wordSymbol is the symbol name
        // rtEntry.addend = definedSymbol.offset;
        // rtEntry.typeOfRelocation = local_absolute; /* set the appropriate relocation type */
        // relocationTable[current_section].push_back(rtEntry);

      }else if(definedSymbol.scope == global || definedSymbol.scope == external){
        //POTREBNA PROVERA
        //kada je definisan treba samo tabela relokacija, u suprotnom treba i FRT
        sectionTable[current_section].opCodeList.push_back("00000000");
        
        if (definedSymbol.isDefined==false) {
          //OVO MOZDA NIJE NI POTREBNO        
          ForwardReferencingTableEntry newFRT;
          newFRT.sectionName = current_section;
          newFRT.sectionOffset = location_counter;

          definedSymbol.flink.push_back(newFRT);
          //ovde je potrebno i FRT i RT, ali to cu staviti posle ifa
        }
        //da li mi treba tabela relokacija ako je vec definisan?
        RelocationTableEntry rtEntry;
        rtEntry.id = ++sectionTable[current_section].relocationIndexId;
        rtEntry.sectionName = current_section;
        rtEntry.sectionOffset = location_counter;
        rtEntry.sectionId = sectionTable[current_section].idSection;
        rtEntry.symbolId = definedSymbol.id;
        rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
        rtEntry.addend = 0;
        rtEntry.typeOfRelocation = global_absolute; /* set the appropriate relocation type */
        relocationTable[current_section].push_back(rtEntry);

      }else{ //na primer da se desi da je undefined simbol, mada mislim da je nemoguce?
        //isto kao local
        cout << "USAO SAM U ELSE U WORDU";
        sectionTable[current_section].opCodeList.push_back("00000000");
      
        //sada ubacujem FRT
        ForwardReferencingTableEntry newFRT;
        newFRT.sectionName = current_section;
        newFRT.sectionOffset = location_counter;

        definedSymbol.flink.push_back(newFRT);

        RelocationTableEntry rtEntry;
        rtEntry.id = ++sectionTable[current_section].relocationIndexId;
        rtEntry.sectionName = current_section;
        rtEntry.sectionOffset = location_counter;
        rtEntry.sectionId = sectionTable[current_section].idSection;
        rtEntry.symbolId = definedSymbol.id;
        rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
        rtEntry.addend = 0;
        rtEntry.typeOfRelocation = local_absolute; /* set the appropriate relocation type */
        relocationTable[current_section].push_back(rtEntry);
      }

    }
    location_counter += 4;
  }
}

void Assembler::endAssemblyDirective() {
  SectionEntry& currentSection = sectionTable[current_section];
  cout<<"ime trenutne sekcije" << currentSection.idSection <<endl;
  currentSection.locationCounter += location_counter;
  currentSection.size = currentSection.locationCounter;
  currentSection.poolLocation = currentSection.locationCounter; 
  cout<< currentSection.locationCounter <<endl;
  cout<< currentSection.size <<endl;
  cout<< "Trebalo bi da sam promenio LC i size"<<endl;
  // sada izvlacim iz FRT svih sekcija
  for (auto& pair : sectionTable) {
    const std::string& key = pair.first;         // The key (section name)
    SectionEntry& section = pair.second;   // The value (SectionEntry object)

    vector<JumpRelocationEntry> current_JRvector = section.jumpRelocationVector;
    for (const auto& entry : current_JRvector) {
      string jmpValue = entry.symbolName;
      int poolOffset = section.literalPool.offsetInPoolForSymbol[entry.symbolName];
      //edituj novi kod sa lokacijom onoga
      int newDisplacement = section.poolLocation - entry.sectionOffset + poolOffset - 4;

      int absoluteSymbolAdress = section.poolLocation + section.literalPool.offsetInPoolForSymbol[entry.symbolName];

      std::cout << std::left << std::setw(35) << "Jump value"
                << std::setw(10) << jmpValue << std::endl;
      std::cout << std::left << std::setw(35) << "Displacement for this jump"
                << std::setw(10) << newDisplacement << std::endl;
      if(!isInteger(jmpValue)){
      std::cout << std::left << std::setw(35) << "Absolute value for the symbol " << jmpValue << " is "
                << std::setw(10) << absoluteSymbolAdress << std::endl;
      }
      //ovde mora provera da li staje u 12 bita
      string newCode = section.opCodeList[entry.sectionOffset/4];
      string codeWithDisplacement = modifyCodeWithDisplacement(newCode, newDisplacement);
      cout<<codeWithDisplacement<<endl;

      SectionEntry& sectionToBacktrack = sectionTable[entry.sectionName]; //ovde mozda treba key, kao gornja sekcija, ali necu da diram jer radi za sada
      sectionToBacktrack.opCodeList[entry.sectionOffset/4] = codeWithDisplacement;

      if(!isInteger(jmpValue) && symbolExists(relocationTable[key], jmpValue) == false){
        SymbolTableEntry& definedSymbol = symbolTable[jmpValue];
        if(definedSymbol.scope==global || definedSymbol.scope==external){
          RelocationTableEntry rtEntry;
          rtEntry.id = ++sectionToBacktrack.relocationIndexId;
          rtEntry.sectionName = key;
          rtEntry.sectionOffset = absoluteSymbolAdress;
          rtEntry.sectionId = section.idSection; //section je trenutna sekcija koja obradjuje literalpool
          rtEntry.symbolId = definedSymbol.id;
          rtEntry.symbolName = jmpValue; // Assuming wordSymbol is the symbol name
          rtEntry.addend = 0;
          rtEntry.typeOfRelocation = global_absolute; /* set the appropriate relocation type */
          relocationTable[key].push_back(rtEntry);
        }else{
          RelocationTableEntry rtEntry;
          rtEntry.id = ++sectionToBacktrack.relocationIndexId;
          rtEntry.sectionName = key;
          rtEntry.sectionOffset = absoluteSymbolAdress;
          rtEntry.sectionId = section.idSection; //section je trenutna sekcija koja obradjuje literalpool
          rtEntry.symbolId = sectionToBacktrack.idSection;
          rtEntry.symbolName = definedSymbol.sectionName; // ovo je ime sekcije simbola
          rtEntry.addend += definedSymbol.offset;
          rtEntry.typeOfRelocation = local_absolute; /* set the appropriate relocation type */
          relocationTable[key].push_back(rtEntry);
        }
      }
    }
    for (const auto& literal : section.literalPool.literalList) {
        // Convert the literal to a hexadecimal string
        std::string hexString = intToLittleEndianHexString(literal);

        // Add the literal to the opCodeList
        section.opCodeList.push_back(hexString);
        section.locationCounter += 4;
    }
    section.size = section.locationCounter;
  }
  //splitOpCodeListInPlace();
  //mozda treba da stavim da je current_section = 0
}

// Function to check if a symbolName already exists in the relocation table
bool Assembler::symbolExists(const std::vector<RelocationTableEntry>& relocationTable, std::string& symbolName) {
    for (const auto& entry : relocationTable) {
        if (entry.symbolName == symbolName) {
            return true;  // Symbol name found
        }
    }
    return false;  // Symbol name not found
}

bool Assembler::isInteger(const std::string& str) {
    if (str.empty()) return false;
    size_t start = (str[0] == '-' || str[0] == '+') ? 1 : 0; // Handle sign
    return std::all_of(str.begin() + start, str.end(), ::isdigit);
}

void Assembler::haltAssemblyInstruction()
{
  sectionTable[current_section].opCodeList.push_back("00000000");
  location_counter+=4;
}

void Assembler::intAssemblyInstruction(){
  sectionTable[current_section].opCodeList.push_back("10000000");
  location_counter+=4;
}

void Assembler::jmpAssemblyInstructionLiteral(int literalParam){
  processLiteralForInstruction(literalParam, "38F00000");
}

void Assembler::jmpAssemblyInstructionSymbol(string* wordSymbol) {
  processSymbolForInstruction(wordSymbol, "38F00000");
}

void Assembler::storeDataForBranch(int literalToStore, std::string* symbolToStore){
  cout<<"usao sam u storeData";
  branch_literal = literalToStore;
  branch_symbol = symbolToStore;
}

string Assembler::constructHexCodeForBranch(int startCode, const std::string* registerSrc, const std::string* registerDst){
    cout<<"usao sam u constructHexCode";
    int regSrc = std::stoi(registerSrc->substr(1));
    int regDst = std::stoi(registerDst->substr(1));

    int instruction = startCode << 20;
    instruction |= (regSrc << 16);
    instruction |= (regDst << 12);
    instruction |= (0x0 & 0xFFF); 

    std::stringstream stream;
    stream << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << instruction;

    return stream.str();
}


void Assembler::branchAssemblyInstruction(const std::string* registerSrc, const std::string* registerDst, typeOfBranchJump typeOfBranch){
  switch(typeOfBranch){
    case beq:{
      string newHexCode = constructHexCodeForBranch(0x39F, registerSrc, registerDst);
      cout<<newHexCode;
      if(branch_symbol==nullptr) {
        processLiteralForInstruction(branch_literal, newHexCode);
      }else{
        processSymbolForInstruction(branch_symbol, newHexCode);
      }
      break;
    }
    case bne:{
      string newHexCode = constructHexCodeForBranch(0x3AF, registerSrc, registerDst);
      cout<<newHexCode;
      if(branch_symbol==nullptr) {
        processLiteralForInstruction(branch_literal, newHexCode);
      }else{
        processSymbolForInstruction(branch_symbol, newHexCode);
      }
      break;
    }
    case bgt:{
      string newHexCode = constructHexCodeForBranch(0x3BF, registerSrc, registerDst);
      cout<<newHexCode;
      if(branch_symbol==nullptr) {
        processLiteralForInstruction(branch_literal, newHexCode);
      }else{
        processSymbolForInstruction(branch_symbol, newHexCode);
      }
      break;
    }
  }
}

void Assembler::processLiteralForInstruction(int literalParam, const std::string& instructionCode) {
    // Add the instruction to the opcode list
    sectionTable[current_section].opCodeList.push_back(instructionCode);

    // Check if the literal already exists in the pool to avoid duplication
    if (sectionTable[current_section].literalPool.offsetInPoolForSymbol.find(std::to_string(literalParam)) == sectionTable[current_section].literalPool.offsetInPoolForSymbol.end()) {
        // Add literal to the pool
        sectionTable[current_section].literalPool.literalList.push_back(literalParam);
        
        // Update the pool location counter
        sectionTable[current_section].literalPool.offsetInPoolForSymbol[std::to_string(literalParam)] = sectionTable[current_section].literalPool.poolLC;
        sectionTable[current_section].literalPool.poolLC += 4;  // Adjust size calculation if necessary
    }

    // Create a new JumpRelocationEntry
    JumpRelocationEntry newJRentry;
    newJRentry.sectionName = current_section;
    newJRentry.sectionOffset = location_counter;
    newJRentry.symbolName = std::to_string(literalParam);

    std::cout << "Location counter for this instruction is " << newJRentry.sectionOffset << std::endl;

    sectionTable[current_section].jumpRelocationVector.push_back(newJRentry);

    // Update the location counter
    location_counter += 4;
}

void Assembler::processSymbolForInstruction(std::string* wordSymbol, const std::string& instructionCode) {
    if (symbolTable.find(*wordSymbol) == symbolTable.end()){
        // Symbol does not exist in the symbol table, must be forward-referenced
        
        SymbolTableEntry newSymbol;
        newSymbol.sectionName = "NONE";
        newSymbol.sectionNumber = 0;
        newSymbol.scope = undefined;
        newSymbol.offset = -1;
        newSymbol.id = ++symbolIndexId;
        newSymbol.isDefined = false;
        
        ForwardReferencingTableEntry newFRT;
        newFRT.sectionName = current_section;
        newFRT.sectionOffset = location_counter;

        newSymbol.flink.push_back(newFRT);
        symbolTable[*wordSymbol] = newSymbol;
        
        sectionTable[current_section].opCodeList.push_back(instructionCode); // Using the instruction code provided as argument

        if (sectionTable[current_section].literalPool.offsetInPoolForSymbol.find(*wordSymbol) == sectionTable[current_section].literalPool.offsetInPoolForSymbol.end()) {
          // Add literal to the pool
          sectionTable[current_section].literalPool.literalList.push_back(-1);
          // Update the pool location counter
          sectionTable[current_section].literalPool.offsetInPoolForSymbol[*wordSymbol] = sectionTable[current_section].literalPool.poolLC;
          sectionTable[current_section].literalPool.poolLC += 4;  // Adjust size calculation if necessary
        }

        JumpRelocationEntry newJRentry;
        newJRentry.sectionName = current_section;
        newJRentry.sectionOffset = location_counter;
        newJRentry.symbolName = *wordSymbol;

        sectionTable[current_section].jumpRelocationVector.push_back(newJRentry);

        // RelocationTableEntry rtEntry;
        // rtEntry.id = ++sectionTable[current_section].relocationIndexId;
        // rtEntry.sectionName = "NONE";
        // rtEntry.sectionOffset = location_counter;
        // rtEntry.sectionId = 0;
        // rtEntry.symbolId = newSymbol.id;
        // rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
        // rtEntry.addend = 0; //za lokalni treba offset simbola
        // rtEntry.typeOfRelocation = local_absolute; /* set the appropriate relocation type */
        // relocationTable[current_section].push_back(rtEntry);
        
    } else {
        SymbolTableEntry& definedSymbol = symbolTable[*wordSymbol];
        if (definedSymbol.scope == local) {
            
            sectionTable[current_section].opCodeList.push_back(instructionCode); // Using the instruction code provided as argument
            
            if (sectionTable[current_section].literalPool.offsetInPoolForSymbol.find(*wordSymbol) == sectionTable[current_section].literalPool.offsetInPoolForSymbol.end()
            && current_section==definedSymbol.sectionName) {
              // Add literal to the pool
              sectionTable[current_section].literalPool.literalList.push_back(definedSymbol.offset);
              // Update the pool location counter
              sectionTable[current_section].literalPool.offsetInPoolForSymbol[*wordSymbol] = sectionTable[current_section].literalPool.poolLC;
              sectionTable[current_section].literalPool.poolLC += 4;  // Adjust size calculation if necessary
            }else if(sectionTable[current_section].literalPool.offsetInPoolForSymbol.find(*wordSymbol) == sectionTable[current_section].literalPool.offsetInPoolForSymbol.end()
            && current_section!=definedSymbol.sectionName){
              // Add literal to the pool
              sectionTable[current_section].literalPool.literalList.push_back(-1);
              // Update the pool location counter
              sectionTable[current_section].literalPool.offsetInPoolForSymbol[*wordSymbol] = sectionTable[current_section].literalPool.poolLC;
              sectionTable[current_section].literalPool.poolLC += 4;
            }

            JumpRelocationEntry newJRentry;
            newJRentry.sectionName = current_section;
            newJRentry.sectionOffset = location_counter;
            newJRentry.symbolName = *wordSymbol;

            std::cout << "location counter for this jmp is " << newJRentry.sectionOffset << std::endl;

            sectionTable[current_section].jumpRelocationVector.push_back(newJRentry);
            
            // RelocationTableEntry rtEntry;
            // rtEntry.id = ++sectionTable[current_section].relocationIndexId;
            // rtEntry.sectionName = definedSymbol.sectionName;
            // rtEntry.sectionOffset = location_counter;
            // rtEntry.sectionId = definedSymbol.sectionNumber;
            // rtEntry.symbolId = definedSymbol.id;
            // rtEntry.symbolName = definedSymbol.sectionName; // Assuming wordSymbol is the symbol name
            // rtEntry.addend = definedSymbol.offset;
            // rtEntry.typeOfRelocation = local_absolute; /* set the appropriate relocation type */
            // relocationTable[current_section].push_back(rtEntry);

        } else if (definedSymbol.scope == global || definedSymbol.scope == external) {
            
            if (!definedSymbol.isDefined) {
                // If the symbol is not defined, add a forward reference entry
                
                ForwardReferencingTableEntry newFRT;
                newFRT.sectionName = current_section;
                newFRT.sectionOffset = location_counter;

                definedSymbol.flink.push_back(newFRT);
            }
            
            sectionTable[current_section].opCodeList.push_back(instructionCode); // Using the instruction code provided as argument

            if (sectionTable[current_section].literalPool.offsetInPoolForSymbol.find(*wordSymbol) == sectionTable[current_section].literalPool.offsetInPoolForSymbol.end()) {
              // Add literal to the pool
              sectionTable[current_section].literalPool.literalList.push_back(definedSymbol.offset);
              // Update the pool location counter
              sectionTable[current_section].literalPool.offsetInPoolForSymbol[*wordSymbol] = sectionTable[current_section].literalPool.poolLC;
              sectionTable[current_section].literalPool.poolLC += 4;  // Adjust size calculation if necessary
            }

            JumpRelocationEntry newJRentry;
            newJRentry.sectionName = current_section;
            newJRentry.sectionOffset = location_counter;
            newJRentry.symbolName = *wordSymbol;

            std::cout << "location counter for this jmp is " << newJRentry.sectionOffset << std::endl;

            sectionTable[current_section].jumpRelocationVector.push_back(newJRentry);

            // RelocationTableEntry rtEntry;
            // rtEntry.id = ++sectionTable[current_section].relocationIndexId;
            // rtEntry.sectionName = definedSymbol.sectionName;
            // rtEntry.sectionOffset = location_counter;
            // rtEntry.sectionId = definedSymbol.sectionNumber;
            // rtEntry.symbolId = definedSymbol.id;
            // rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
            // rtEntry.addend = 0;
            // rtEntry.typeOfRelocation = global_absolute; /* set the appropriate relocation type */
            // relocationTable[current_section].push_back(rtEntry);

        } else { // Handling undefined symbol case
            std::cout << "Entered ELSE in JMP";

            // Add a forward reference entry
            ForwardReferencingTableEntry newFRT;
            newFRT.sectionName = current_section;
            newFRT.sectionOffset = location_counter;

            definedSymbol.flink.push_back(newFRT);

            sectionTable[current_section].opCodeList.push_back(instructionCode); // Using the instruction code provided as argument

            if (sectionTable[current_section].literalPool.offsetInPoolForSymbol.find(*wordSymbol) == sectionTable[current_section].literalPool.offsetInPoolForSymbol.end()) {
              // Add literal to the pool
              sectionTable[current_section].literalPool.literalList.push_back(-1);
              // Update the pool location counter
              sectionTable[current_section].literalPool.offsetInPoolForSymbol[*wordSymbol] = sectionTable[current_section].literalPool.poolLC;
              sectionTable[current_section].literalPool.poolLC += 4;  // Adjust size calculation if necessary
            }
            JumpRelocationEntry newJRentry;
            newJRentry.sectionName = current_section;
            newJRentry.sectionOffset = location_counter;
            newJRentry.symbolName = *wordSymbol;

            sectionTable[current_section].jumpRelocationVector.push_back(newJRentry);

            // RelocationTableEntry rtEntry;
            // rtEntry.id = ++sectionTable[current_section].relocationIndexId;
            // rtEntry.sectionName = definedSymbol.sectionName;
            // rtEntry.sectionOffset = location_counter;
            // rtEntry.sectionId = definedSymbol.sectionNumber;
            // rtEntry.symbolId = definedSymbol.id;
            // rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
            // rtEntry.addend = 0;
            // rtEntry.typeOfRelocation = local_absolute; /* set the appropriate relocation type */
            // relocationTable[current_section].push_back(rtEntry);
        }
    }
    location_counter += 4;
}

void Assembler::storeData(int literal, string* symbol, AdressingType adressType, string* registerReg){
  current_data_literal = literal;
  current_data_symbol = symbol;
  current_data_address = adressType;
  current_register = registerReg;
}

string Assembler::constructHexCodeForLD(int startCode, const std::string* registerSrc){
   // Convert the register name to a number (assuming format is "%r0", "%r1", ..., "%r15")
    int regNum = std::stoi(registerSrc->substr(1)); // Extract the number part from "%rX"

    // Start with the given startCode (e.g., 0x92 for ld instruction)
    int instruction = startCode << 24;  // Shifted to the most significant byte

    // Set AAAA (4 bits for registerSrc)
    instruction |= (regNum << 20); // Shift the register number to the correct position

    // Set BBBB to 1111 (4 bits)
    instruction |= (0xF << 16);    // Shift 0xF to the correct position

    // CCCC is 0000 and DDDD DDDD DDDD is 000, so no need to change these

    // Convert to a hexadecimal string with uppercase letters
    std::stringstream stream;
    stream << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << instruction;

    // Return the final hex string
    return stream.str();
}

void Assembler::dataLdAssemblyInstruction(string* registerDst){
  switch (current_data_address) {
    case immed: {
        if (current_data_symbol == nullptr) {
            // It's a literal
            std::string newHexCode = constructHexCodeForLD(0x92, registerDst);
            processLiteralForInstruction(current_data_literal, newHexCode);
        } else {
            // It's a symbol
            std::string newHexCode = constructHexCodeForLD(0x92, registerDst);
            processSymbolForInstruction(current_data_symbol, newHexCode);
        }
        break;
    }
    case registerDirect: {
        if (current_register != nullptr) {
          generateRegisterInstruction(0x9, 0x1, registerDst);  // MMMM = 0001
        } else {
            cerr << "Register doesn't exist!" << endl;
            exit(-1);
        }
        break;
    }
    case registerIndirect: {
        if (current_register != nullptr) {
          generateRegisterInstruction(0x9, 0x2, registerDst);  // MMMM = 0010
        } else {
            cerr << "Register doesn't exist!" << endl;
            exit(-1);
        }
        break;
    }
    case memoryDirectLiteral: {
        std::string newHexCode = constructHexCodeForLD(0x92, registerDst);
        processLiteralForInstruction(current_data_literal, newHexCode);

        generateRegisterInstructionSameRegisters(0x9, 0x2, registerDst);
        break;
    }
    case memoryDirectSymbol: {
        cout << "Usao sam u memoryDirectLiteral sa simbolom " << *current_data_symbol << endl;
        std::string newHexCode = constructHexCodeForLD(0x92, registerDst);
        processSymbolForInstruction(current_data_symbol, newHexCode);

        generateRegisterInstructionSameRegisters(0x9, 0x2, registerDst);
        break;
    }
    case regIndOffset: {
      cout << "Usao sam u regIndOffset sa registrom "<< *current_register <<" i sa offsetom "<< current_data_literal << endl;
      if(current_data_literal>2047 || current_data_literal<-2048){
        cerr << "Displacement " << current_data_literal << " is too big!" << endl;
        exit(-1);
      }else{
        loadRegisterWithOffset(registerDst);
      }
      
      break;
    }
}
}

string Assembler::constructHexCodeForST(int startCode, const std::string* registerSrc) {
    // Convert the source register name to a number (assuming format is "%r0", "%r1", ..., "%r15")
    int regNum = std::stoi(registerSrc->substr(1)); // Extract the number part from "%rX"

    // Start with the given startCode (e.g., 0x92 for ld instruction)
    int instruction = startCode << 24;  // Shifted to the most significant byte

    // Set AAAA to 0xF (hardcoded)
    instruction |= (0xF << 20);  // Set AAAA to 0xF

    // Set BBBB to 0 (hardcoded)
    instruction |= (0x0 << 16);  // Set BBBB to 0

    // Set CCCC to registerSrc (source register number)
    instruction |= (regNum << 12);  // Shift the register number to the correct position for CCCC

    // DDDD DDDD DDDD is 000, so no need to change these

    // Convert to a hexadecimal string with uppercase letters
    std::stringstream stream;
    stream << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << instruction;

    // Return the final hex string
    return stream.str();
}


void Assembler::dataStAssemblyInstruction(string* registerSrc){
  switch (current_data_address) {
    case immed:{
      cerr << "Greska tokom asembliranja, St i immed ne mogu zajedno " << endl;
      exit(-1);
      break;
    }    
    case registerDirect: { //ovo je isto kao za ld, samo sam obrnuo registrima mesta
        if (current_register != nullptr) {
          // Log the processing of the instruction
          cout << "Usao sam u memorijsko adresiranje sa registrom " << *current_register << endl;

          // Extract destination register from registerDst and source register from the global current_register
          int regSrc = std::stoi(registerSrc->substr(1));  // Extract the destination register number from %rX
          int instruction = 0x9 << 28;  // Start with the first byte passed as an argument (instead of 0x9)

          int regDest = std::stoi(current_register->substr(1));  // Extract source register number from global %rX

          instruction |= (0x1 << 24);  // Set MMMM bits
          instruction |= (regSrc << 20); 
          instruction |= (regDest << 16);  

          // Convert instruction to a hex string
          char hexInstruction[9];
          snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

          // Push the hex string to the opCodeList in the current section
          sectionTable[current_section].opCodeList.push_back(hexInstruction);

          // Update the location counter
          location_counter += 4;
        } else {
            cerr << "Register doesn't exist!" << endl;
            exit(-1);
        }
        break;
    }
    case registerIndirect: {
        generateRegisterInstructionSt(0x8, 0x0, registerSrc);
        break;
    }
    //PROVERI OVAJ MEMORYDIRECT LITERAL
    case memoryDirectLiteral:{
      std::string newHexCode = constructHexCodeForST(0x82, registerSrc);

      processLiteralForInstruction(current_data_literal, newHexCode);
      break;
    }
    case memoryDirectSymbol:{
      std::string newHexCode = constructHexCodeForST(0x82, registerSrc);

      processSymbolForInstruction(current_data_symbol, newHexCode);
      break;
    }
    case regIndOffset:{
      if(current_data_literal>2047 || current_data_literal<-2048){
        cerr << "Displacement " << current_data_literal << " is too big!" << endl;
        exit(-1);
      }else{
        generateRegisterInstructionregIndOffset(0x8, 0x0, registerSrc, current_data_literal);
      }
      break;
    }
  }
}

void Assembler::generateRegisterInstructionregIndOffset(int firstByte, int mmmm, std::string* registerSrc, int displacement) {
    // Log the processing of the instruction
    cout << "Processing memory addressing with destination register: " << *current_register << endl;

    // Extract destination register from the global current_register (gpr[A])
    int regDest = std::stoi(current_register->substr(1));  // Extract the destination register number from %rX (gpr[A])

    // Extract source register from registerSrc (gpr[C])
    int regSrc = std::stoi(registerSrc->substr(1));  // Extract the source register number from %rX (gpr[C])

    // Start with the first byte passed as an argument
    int instruction = firstByte << 28;

    // Set MMMM bits for the instruction
    instruction |= (mmmm << 24);  // Set MMMM bits to specify the operation mode

    // Set AAAA (destination register gpr[A])
    instruction |= (regDest << 20);

    // Set BBBB to 0 (since you want BBBB to be 0)
    instruction |= (0x0 << 16);

    // Set CCCC (source register gpr[C])
    instruction |= (regSrc << 12);  // Set the source register number in CCCC

    // Reorder the displacement from 'abc' to 'cab' (right circular shift)
    int reorderedDisplacement = ((displacement & 0xF00) >> 4) |  // Move the third hex digit (c) to the first position
                                ((displacement & 0x0F0) >> 4) |  // Move the middle hex digit (b) to the third position
                                ((displacement & 0x00F) << 8);   // Move the first hex digit (a) to the middle position

    // Add the reordered displacement (DDDD DDDD DDDD)
    instruction |= (reorderedDisplacement & 0xFFF);  // Ensure the displacement is 12 bits

    // Convert the instruction to a hex string
    char hexInstruction[9];  // 8 hex digits + null terminator
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    // Update the location counter
    location_counter += 4;
}


void Assembler::generateRegisterInstructionSt(int firstByte, int mmmm, std::string* registerSrc) {
    // Log the processing of the instruction
    cout << "Processing memory addressing with destination register: " << *current_register << endl;

    // Extract destination register from the global current_register (gpr[A])
    int regDest = std::stoi(current_register->substr(1));  // Extract the destination register number from %rX (gpr[A])

    // Extract source register from registerSrc (gpr[C])
    int regSrc = std::stoi(registerSrc->substr(1));  // Extract the source register number from %rX (gpr[C])

    // Start with the first byte passed as an argument (instead of 0x9)
    int instruction = firstByte << 28;

    // Set MMMM bits for the instruction
    instruction |= (mmmm << 24);  // Set MMMM bits to specify the operation mode

    // Set AAAA (destination register gpr[A])
    instruction |= (regDest << 20);

    // Set BBBB to 0 (since you want BBBB to be 0)
    instruction |= (0x0 << 16);

    // Set CCCC (source register gpr[C])
    instruction |= (regSrc << 12);  // Set the source register number in CCCC

    // Convert the instruction to a hex string
    char hexInstruction[9];  // 8 hex digits + null terminator
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    // Update the location counter
    location_counter += 4;
}



void Assembler::generateRegisterInstruction(int firstByte, int mmmm, std::string* registerDst) {
    // Log the processing of the instruction
    cout << "Usao sam u memorijsko adresiranje sa registrom " << *current_register << endl;

    // Extract destination register from registerDst and source register from the global current_register
    int regDest = std::stoi(registerDst->substr(1));  // Extract the destination register number from %rX
    int instruction = firstByte << 28;  // Start with the first byte passed as an argument (instead of 0x9)

    int regSrc = std::stoi(current_register->substr(1));  // Extract source register number from global %rX

    instruction |= (mmmm << 24);  // Set MMMM bits
    instruction |= (regDest << 20);  // Set AAAA (destination register)
    instruction |= (regSrc << 16);   // Set BBBB (source register)

    // Convert instruction to a hex string
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    // Update the location counter
    location_counter += 4;
}

void Assembler::generateRegisterInstructionSameRegisters(int firstByte, int mmmm, std::string* registerDst) {
    // Log the processing of the instruction
    cout << "Usao sam u memorijsko adresiranje sa registrom " << *registerDst << endl;

    // Extract destination register from registerDst
    int regDest = std::stoi(registerDst->substr(1));  // Extract the destination register number from %rX
    int instruction = firstByte << 28;  // Start with the first byte passed as an argument

    // Use the same register for both source and destination
    int regSrc = regDest;  // Both registers are the same

    instruction |= (mmmm << 24);  // Set MMMM bits
    instruction |= (regDest << 20);  // Set AAAA (destination register)
    instruction |= (regSrc << 16);   // Set BBBB (source register)

    // Convert instruction to a hex string
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    // Update the location counter
    location_counter += 4;
}


void Assembler::loadRegisterWithOffset(std::string* registerDst) {
    // Log the processing of the instruction
    cout << "Processing ld [rX + " << current_data_literal << "], " << *registerDst << endl;

    // Check for null pointers
    if (!registerDst || !current_register) {
        cerr << "Error: registerDst or current_register is null!" << endl;
        exit(-1);
    }

    // Extract destination register from registerDst and base register from current_register
    int regDest = std::stoi(registerDst->substr(1));  // Extract the destination register number from rX (e.g., r0 -> 0)
    int instruction = 0x9 << 28;  // Start with the opcode 0x9 (for ld)
    int regBase = std::stoi(current_register->substr(1));  // Extract base register number from rX (e.g., r1 -> 1)
    int regC = 0;  // Set to 0 if not using a second register

    // Set MMMM to 0010 for memory load with base + offset + (optional) second register
    instruction |= (0x2 << 24);  // Set MMMM bits to 0010
    instruction |= (regDest << 20);  // Set AAAA (destination register)
    instruction |= (regBase << 16);  // Set BBBB (base register)
    instruction |= (regC << 12);  // Set CCCC to 0 if no second register is used

    // The displacement literal (D) is stored in current_data_literal (class variable)

    // Reorder the displacement from 'abc' to 'cab' (right circular shift)
    int reorderedDisplacement = ((current_data_literal & 0xF00) >> 4) |  // Move the third hex digit (c) to the first position
                                ((current_data_literal & 0x0F0) >> 4) |  // Move the middle hex digit (b) to the third position
                                ((current_data_literal & 0x00F) << 8);   // Move the first hex digit (a) to the middle position

    // Add the reordered displacement (DDDD DDDD DDDD)
    instruction |= (reorderedDisplacement & 0xFFF);  // Ensure the displacement is 12 bits

    // Convert instruction to a hex string
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    // Update the location counter
    location_counter += 4;

    cout << "Generated instruction: " << hexInstruction << endl;
}

void Assembler::callAssemblyInstruction(int literalParam, string* wordSymbol){
  if(wordSymbol == nullptr){
    //obradjujemo literal
    processLiteralForInstruction(literalParam, "21F00000");
  }else{
    processSymbolForInstruction(wordSymbol, "21F00000");
  }
}

void Assembler::retAssemblyInstruction(){
  sectionTable[current_section].opCodeList.push_back("93FE0400");
  location_counter += 4;
}

void Assembler::iretAssemblyInstruction(){
  //prva instrukcija -> ld %status, [%sp + 4]
  sectionTable[current_section].opCodeList.push_back("960E0400");
  location_counter += 4;

  //druga instrukcija -> ld %pc, [%sp]; sp=sp+8
  sectionTable[current_section].opCodeList.push_back("93FE0800");
  location_counter += 4;
}

void Assembler::pushRegisterInstruction(string* registerSrc){
  //-4 u little-endian formatu, za toliko se povlaci sp (ffc je -4)
  int displacement = 0xcff;

  int regDest = 14;  
  int regSrc = std::stoi(registerSrc->substr(1));  
  int instruction = 0x8 << 28;
  instruction |= (0x1 << 24);  
  instruction |= (regDest << 20);
  instruction |= (0x0 << 16);
  instruction |= (regSrc << 12);  

  instruction |= (displacement & 0xFFF);  

  char hexInstruction[9];  
  snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

  sectionTable[current_section].opCodeList.push_back(hexInstruction);
  location_counter += 4;
}

void Assembler::popRegisterInstruction(string* registerDst){

  int displacement = 0x400; //+4 na sp, originalna vrednost je 04

  int regDest = std::stoi(registerDst->substr(1));  
  int instruction = 0x9 << 28;  
  int regC = 0;  

  instruction |= (0x3 << 24);  
  instruction |= (regDest << 20);  
  instruction |= (0xe << 16);  
  instruction |= (regC << 12);  
 

  instruction |= (displacement & 0xFFF);  

  char hexInstruction[9];
  snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

  sectionTable[current_section].opCodeList.push_back(hexInstruction);
  location_counter += 4;
}


void Assembler::xchgAssemblyInstruction(string* registerDst, string* registerSrc) {
    int regDstNumber = stoi(registerDst->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    int regSrcNumber = stoi(registerSrc->substr(1)) & 0xF; // Extract the last 4 bits (register number)

    // Construct the instruction:
    // First 12 bits: 0x400 (0100 0000 0000)
    // Next 4 bits: regDstNumber
    // Next 4 bits: regSrcNumber
    // Last 12 bits: 0x000 (0000 0000 0000)

    uint32_t instruction = 0x40000000; // Start with 0x400 followed by 20 bits of 0

    // Pack the numbers into the correct positions:
    instruction |= (regDstNumber << 16); // Place regDstNumber in bits 16-19
    instruction |= (regSrcNumber << 12); // Place regSrcNumber in bits 12-15

    // Convert the instruction to a hex string manually
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    location_counter += 4;
    
    // Output for debugging
    //cout << "Pushed instruction: " << hexInstruction << endl;
}


void Assembler::addAssemblyInstruction(string* registerDst, string* registerSrc){
    int regDstNumber = stoi(registerDst->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    int regSrcNumber = stoi(registerSrc->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    
    // Construct the instruction:
    // First byte is 50 (0101 0000)
    // Next 4 bits: regDstNumber
    // Next 4 bits: regDstNumber
    // Next 4 bits: regSrcNumber
    // Remaining bits: 0

    uint32_t instruction = 0x50000000; // Start with 0x50 followed by 24 bits of 0

    // Pack the numbers into the correct positions:
    instruction |= (regSrcNumber << 20); // Place regDstNumber in the next 4 bits
    instruction |= (regSrcNumber << 16); // Place regDstNumber in the next 4 bits after that
    instruction |= (regDstNumber << 12); // Place regSrcNumber in the next 4 bits

    // Convert the instruction to a hex string manually
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    location_counter += 4;
    
    // Output for debugging
    //cout << "Pushed instruction: " << hexInstruction << endl;
}

void Assembler::subAssemblyInstruction(string* registerDst, string* registerSrc){
    int regDstNumber = stoi(registerDst->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    int regSrcNumber = stoi(registerSrc->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    
    // Construct the instruction:
    // First byte is 51 (0101 0001)
    // Next 4 bits: regDstNumber
    // Next 4 bits: regDstNumber
    // Next 4 bits: regSrcNumber
    // Remaining bits: 0

    uint32_t instruction = 0x51000000; // Start with 0x51 followed by 24 bits of 0

    // Pack the numbers into the correct positions:
    instruction |= (regSrcNumber << 20); // Place regDstNumber in the next 4 bits
    instruction |= (regSrcNumber << 16); // Place regDstNumber in the next 4 bits after that
    instruction |= (regDstNumber << 12); // Place regSrcNumber in the next 4 bits

    // Convert the instruction to a hex string manually
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    location_counter += 4;
    
    // Output for debugging
    //cout << "Pushed instruction: " << hexInstruction << endl;
}

void Assembler::mulAssemblyInstruction(string* registerDst, string* registerSrc){
    int regDstNumber = stoi(registerDst->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    int regSrcNumber = stoi(registerSrc->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    
    // Construct the instruction:
    // First byte is 52 (0101 0010)
    // Next 4 bits: regDstNumber
    // Next 4 bits: regDstNumber
    // Next 4 bits: regSrcNumber
    // Remaining bits: 0

    uint32_t instruction = 0x52000000; // Start with 0x52 followed by 24 bits of 0

    // Pack the numbers into the correct positions:
    instruction |= (regSrcNumber << 20); // Place regDstNumber in the next 4 bits
    instruction |= (regSrcNumber << 16); // Place regDstNumber in the next 4 bits after that
    instruction |= (regDstNumber << 12); // Place regSrcNumber in the next 4 bits

    // Convert the instruction to a hex string manually
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    location_counter += 4;
    
    // Output for debugging
    //cout << "Pushed instruction: " << hexInstruction << endl;
}

void Assembler::divAssemblyInstruction(string* registerDst, string* registerSrc){

    int regDstNumber = stoi(registerDst->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    int regSrcNumber = stoi(registerSrc->substr(1)) & 0xF; // Extract the last 4 bits (register number)
    
    // Construct the instruction:
    // First byte is 53 (0101 0011)
    // Next 4 bits: regDstNumber
    // Next 4 bits: regDstNumber
    // Next 4 bits: regSrcNumber
    // Remaining bits: 0

    uint32_t instruction = 0x53000000; // Start with 0x53 followed by 24 bits of 0

    // Pack the numbers into the correct positions:
    instruction |= (regSrcNumber << 20); // Place regDstNumber in the next 4 bits
    instruction |= (regSrcNumber << 16); // Place regDstNumber in the next 4 bits after that
    instruction |= (regDstNumber << 12); // Place regSrcNumber in the next 4 bits

    // Convert the instruction to a hex string manually
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    location_counter += 4;
    
    // Output for debugging
    //cout << "Pushed instruction: " << hexInstruction << endl;
}

void Assembler::csrrdAssemblyInstruction(string* systemRegisterSrc, string* registerDst){
  int registerSrc = 0;
  if(*systemRegisterSrc == "status"){
    registerSrc = 0;
  }else if(*systemRegisterSrc == "handler"){
    registerSrc = 1;
  }else if(*systemRegisterSrc == "cause"){
    registerSrc = 2;
  }

  //-4 u little-endian formatu, za toliko se povlaci sp (ffc je -4)
  int displacement = 0x000;

  int regDest = std::stoi(registerDst->substr(1));  
  int regSrc = registerSrc;
  int instruction = 0x9 << 28;
  instruction |= (0x0 << 24);  
  instruction |= (regDest << 20);
  instruction |= (regSrc << 16);
  instruction |= (0x0 << 12);  

  instruction |= (displacement & 0xFFF);  

  char hexInstruction[9];  
  snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

  sectionTable[current_section].opCodeList.push_back(hexInstruction);
  location_counter += 4;
}

void Assembler::csrwrAssemblyInstruction(string* registerSrc, string* systemRegisterDst){
  int registerDst = 0;
  if(*systemRegisterDst == "status"){
    registerDst = 0;
  }else if(*systemRegisterDst == "handler"){
    registerDst = 1;
  }else if(*systemRegisterDst == "cause"){
    registerDst = 2;
  }

  //-4 u little-endian formatu, za toliko se povlaci sp (ffc je -4)
  int displacement = 0x000;

  int regDest = registerDst;  
  int regSrc = std::stoi(registerSrc->substr(1));
  int instruction = 0x9 << 28;
  instruction |= (0x4 << 24);  
  instruction |= (regDest << 20);
  instruction |= (regSrc << 16);
  instruction |= (0x0 << 12);  

  instruction |= (displacement & 0xFFF);  

  char hexInstruction[9];  
  snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

  sectionTable[current_section].opCodeList.push_back(hexInstruction);
  location_counter += 4;
}

void Assembler::notAssemblyInstruction(string* registerOnly){
  int displacement = 0x000; //+4 na sp, originalna vrednost je 04

  int regOnly = std::stoi(registerOnly->substr(1));  
  int instruction = 0x6 << 28;  
  int regC = 0;  

  instruction |= (0x0 << 24);  
  instruction |= (regOnly << 20);  
  instruction |= (regOnly << 16);  
  instruction |= (regC << 12);  
 

  instruction |= (displacement & 0xFFF);  

  char hexInstruction[9];
  snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

  sectionTable[current_section].opCodeList.push_back(hexInstruction);
  location_counter += 4;
}

void Assembler::generateLogicRegisterInstruction(int opCode, string* registerSrc, string* registerDst) {
  int displacement = 0x000; //+4 na sp, originalna vrednost je 04

  int regSrc = std::stoi(registerSrc->substr(1));  
  int regDst = std::stoi(registerDst->substr(1));  
  int instruction = opCode << 24;  
  int regC = 0;  
 
  instruction |= (regDst << 20);  
  instruction |= (regDst << 16);  
  instruction |= (regSrc << 12);  
 

  instruction |= (displacement & 0xFFF);  

  char hexInstruction[9];
  snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

  sectionTable[current_section].opCodeList.push_back(hexInstruction);
  location_counter += 4;
}

void Assembler::andAssemblyInstruction(string* registerSrc, string* registerDst){
  generateLogicRegisterInstruction(0x61, registerSrc, registerDst);
}

void Assembler::orAssemblyInstruction(string* registerSrc, string* registerDst){
  generateLogicRegisterInstruction(0x62, registerSrc, registerDst);
}

void Assembler::xorAssemblyInstruction(string* registerSrc, string* registerDst){
  generateLogicRegisterInstruction(0x63, registerSrc, registerDst);
}

void Assembler::shlAssemblyInstruction(string* registerSrc, string* registerDst){
  generateLogicRegisterInstruction(0x70, registerSrc, registerDst);
}

void Assembler::shrAssemblyInstruction(string* registerSrc, string* registerDst){
  generateLogicRegisterInstruction(0x71, registerSrc, registerDst);
}


int Assembler::convertHexToInt(string* hexString){
  string hexWithoutPrefix = hexString->substr(2);
  unsigned long decimalValue = std::stoul(hexWithoutPrefix, nullptr, 16);
  return decimalValue;
}

void Assembler::printSymbolTable(){
   std::cout << "Symbol Table:\n";
    std::cout << "--------------------------------------------------------------\n";
    std::cout << std::setw(15) << "Symbol" 
              << std::setw(15) << "Section Num" 
              << std::setw(10) << "ID" 
              << std::setw(15) << "Section Name"
              << std::setw(10) << "Offset" 
              << std::setw(10) << "Defined" 
              << std::setw(10) << "Scope" << std::endl;
    std::cout << "--------------------------------------------------------------\n";

    for (const auto& entry : symbolTable) {
      const std::string& symbol = entry.first;
      const SymbolTableEntry& entryValue = entry.second;
      
      std::cout << std::setw(15) << symbol
                << std::setw(15) << entryValue.sectionNumber
                << std::setw(10) << entryValue.id
                << std::setw(15) << entryValue.sectionName
                << std::setw(10) << entryValue.offset
                << std::setw(10) << (entryValue.isDefined ? "Yes" : "No")
                << std::setw(10) << entryValue.scope << std::endl;
    } 

    std::cout << "--------------------------------------------------------------\n";
}

void Assembler::printSectionTable() {
    // Print section information first (without the literal pool)
    cout << setw(20) << "Section Name"
         << setw(15) << "ID Section"
         << setw(20) << "Location Counter"
         << setw(10) << "Size"
         << setw(15) << "Start Address"
         << setw(30) << "OpCode List" << endl;

    cout << string(120, '-') << endl;

    for (const auto& entry : sectionTable) {
        const string& sectionName = entry.first;
        const SectionEntry& sectionData = entry.second;

        // Print section details in a formatted way
        cout << setw(20) << sectionName
             << setw(15) << sectionData.idSection
             << setw(20) << sectionData.locationCounter
             << setw(10) << sectionData.size
             << setw(15) << sectionData.startAddr;

        // Print the opCodeList
        cout << setw(30);
        for (const string& opCode : sectionData.opCodeList) {
            cout << opCode << " ";
        }

        cout << endl;
    }    

    // Now print the literal pools for each section
    cout << "\nLiteral Pools" << endl;
    cout << setw(20) << "Section Name"
         << setw(15) << "Pool Location"
         << setw(30) << "Literals"
         << setw(30) << "Offset in Pool" << endl;

    cout << string(120, '-') << endl;

    for (const auto& entry : sectionTable) {
        const string& sectionName = entry.first;
        const SectionEntry& sectionData = entry.second;

        // Print the literal pool details for this section
        cout << setw(20) << sectionName
             << setw(15) << sectionData.poolLocation;

        string literals;
        for (size_t i = 0; i < sectionData.literalPool.literalList.size(); ++i) {
            literals += to_string(sectionData.literalPool.literalList[i]) + " ";
        }
        cout << setw(30) << literals;

        string offsets;
        for (map<string, int>::const_iterator it = sectionData.literalPool.offsetInPoolForSymbol.begin();
             it != sectionData.literalPool.offsetInPoolForSymbol.end(); ++it) {
            offsets += it->first + ": " + to_string(it->second) + " ";
        }
        cout << setw(30) << offsets;

        cout << endl;
    }
}





void Assembler::printSymbolEntry(string* symbolToPrint) {
    // Assuming symbolTable is a map<string, SymbolTableEntry>
    auto it = symbolTable.find(*symbolToPrint);  // Lookup in symbol table

    if (it != symbolTable.end()) {
        // Retrieve the SymbolTableEntry
        const SymbolTableEntry& entry = it->second;

        std::cout << "Symbol Entry:\n";
        std::cout << "--------------------------------------------------------------\n";
        std::cout << std::setw(15) << "Symbol" 
                  << std::setw(15) << "Section Num" 
                  << std::setw(10) << "ID" 
                  << std::setw(15) << "Section Name"
                  << std::setw(10) << "Offset" 
                  << std::setw(10) << "Defined" 
                  << std::setw(10) << "Scope" << std::endl;
        std::cout << "--------------------------------------------------------------\n";

        // Print the symbol entry details
        std::cout << std::setw(15) << *symbolToPrint
                  << std::setw(15) << entry.sectionNumber
                  << std::setw(10) << entry.id
                  << std::setw(15) << entry.sectionName
                  << std::setw(10) << entry.offset
                  << std::setw(10) << (entry.isDefined ? "Yes" : "No");

        // Print scope (assuming visibility is an enum)
        std::string scopeStr;
        switch(entry.scope) {
            case local:
                scopeStr = "Local";
                break;
            case global:
                scopeStr = "Global";
                break;
            case external:
                scopeStr = "Extern";
                break;
            case section:
                scopeStr = "Section";
                break;
            case undefined:
                scopeStr = "Undefined";
                break;
        }
        std::cout << std::setw(10) << scopeStr << std::endl;

        // Optional: Print forward references
        if (!entry.flink.empty()) {
            std::cout << "\nForward References:\n";
            std::cout << "--------------------------------------------------------------\n";
            std::cout << std::setw(15) << "Section"
                      << std::setw(10) << "Offset" << std::endl;
            std::cout << "--------------------------------------------------------------\n";
            for (const auto& ref : entry.flink) {
                std::cout << std::setw(15) << ref.sectionName
                          << std::setw(10) << ref.sectionOffset << std::endl;
            }
        } else {
            std::cout << "\nNo forward references found for this symbol." << std::endl;
        }

        std::cout << "--------------------------------------------------------------\n";
    } else {
        std::cout << "Symbol " << *symbolToPrint << " not found!" << std::endl;
    }
}

string Assembler::intToLittleEndianHexString(int value) {
    std::stringstream ss;

    // Convert each byte to a 2-digit uppercase hexadecimal and append it to the stream
    unsigned char bytes[4];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;

    // Ensures uppercase hex digits
    ss << std::uppercase << std::hex << std::setfill('0');

    for (int i = 0; i < 4; ++i) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
    }

    return ss.str();
}

string Assembler::modifyCodeWithDisplacement(const std::string& code, int displacement){
  // Ensure the code string has at least 8 characters
  if (code.length() != 8) {
      std::cerr << "Error: Code string must be 8 characters long." << std::endl;
      return code;
  }

  // Convert displacement to a 3-character hexadecimal string
  std::stringstream ss;
  ss << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << displacement;
  std::string displacementHex = ss.str();

  // Check if the displacementHex is exactly 3 characters
  if (displacementHex.length() != 3) {
      std::cerr << "Error: Displacement must fit within 3 hexadecimal characters." << std::endl;
      return code;
  }

  // Reorder the displacement from 'abc' to 'cab'
  std::string reorderedDisplacement = displacementHex.substr(2, 1) + displacementHex.substr(0, 1) + displacementHex.substr(1, 1);

  // Replace the last three characters of the code with the reordered displacement
  std::string modifiedCode = code.substr(0, 5) + reorderedDisplacement;

  return modifiedCode;
}

void Assembler::printTablesToFile(const std::string& filename) {
    std::ofstream outFile(filename);

    if (!outFile) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // Print Symbol Table
    outFile << "Symbol Table:\n";
    outFile << "--------------------------------------------------------------\n";
    outFile << std::setw(15) << "Symbol"
            << std::setw(15) << "Section Num"
            << std::setw(10) << "ID"
            << std::setw(15) << "Section Name"
            << std::setw(10) << "Offset"
            << std::setw(10) << "Defined"
            << std::setw(10) << "Scope" << std::endl;
    outFile << "--------------------------------------------------------------\n";

    for (const auto& entry : symbolTable) {
        const std::string& symbol = entry.first;
        const SymbolTableEntry& entryValue = entry.second;

        outFile << std::setw(15) << symbol
                << std::setw(15) << entryValue.sectionNumber
                << std::setw(10) << entryValue.id
                << std::setw(15) << entryValue.sectionName
                << std::setw(10) << entryValue.offset
                << std::setw(10) << (entryValue.isDefined ? "Yes" : "No")
                << std::setw(10) << entryValue.scope << std::endl;
    }

    outFile << "--------------------------------------------------------------\n";

    // Print Section Table
    outFile << "\nSection Table:\n";
    outFile << std::setw(20) << "Section Name"
            << std::setw(15) << "ID Section"
            << std::setw(20) << "Location Counter"
            << std::setw(10) << "Size"
            << std::setw(15) << "Start Address"
            << std::setw(30) << "OpCode List" << std::endl;

    outFile << std::string(120, '-') << std::endl;

    for (const auto& entry : sectionTable) {
        const std::string& sectionName = entry.first;
        const SectionEntry& sectionData = entry.second;

        // Print section details in a formatted way
        outFile << std::setw(20) << sectionName
                << std::setw(15) << sectionData.idSection
                << std::setw(20) << sectionData.locationCounter
                << std::setw(10) << sectionData.size
                << std::setw(15) << sectionData.startAddr;

        // Print the opCodeList
        outFile << std::setw(30);
        for (const std::string& opCode : sectionData.opCodeList) {
            outFile << opCode << " ";
        }

        outFile << std::endl;
    }

    // Now print the literal pools for each section
    outFile << "\nLiteral Pools" << std::endl;
    outFile << std::setw(20) << "Section Name"
            << std::setw(15) << "Pool Location"
            << std::setw(30) << "Literals"
            << std::setw(30) << "Offset in Pool" << std::endl;

    outFile << std::string(120, '-') << std::endl;

    for (const auto& entry : sectionTable) {
        const std::string& sectionName = entry.first;
        const SectionEntry& sectionData = entry.second;

        // Print the literal pool details for this section
        outFile << std::setw(20) << sectionName
                << std::setw(15) << sectionData.poolLocation;

        std::string literals;
        for (size_t i = 0; i < sectionData.literalPool.literalList.size(); ++i) {
            literals += std::to_string(sectionData.literalPool.literalList[i]) + " ";
        }
        outFile << std::setw(30) << literals;

        std::string offsets;
        for (const auto& it : sectionData.literalPool.offsetInPoolForSymbol) {
            offsets += it.first + ": " + std::to_string(it.second) + " ";
        }
        outFile << std::setw(30) << offsets;

        outFile << std::endl;
    }

    outFile.close();  // Ensure to close the file stream when done
}

void Assembler::printTablesToFileProba(const std::string& filename) {
    std::ofstream outFile(filename);

    if (!outFile) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // Print Symbol Table
    outFile << "Symbol Table:\n";
    outFile << "--------------------------------------------------------------\n";
    outFile << std::setw(15) << "Symbol"
            << std::setw(15) << "Section Num"
            << std::setw(10) << "ID"
            << std::setw(15) << "Section Name"
            << std::setw(10) << "Offset"
            << std::setw(10) << "Defined"
            << std::setw(10) << "Scope" << std::endl;
    outFile << "--------------------------------------------------------------\n";

    for (const auto& entry : symbolTable) {
        const std::string& symbol = entry.first;
        const SymbolTableEntry& entryValue = entry.second;

        outFile << std::setw(15) << symbol
                << std::setw(15) << entryValue.sectionNumber
                << std::setw(10) << entryValue.id
                << std::setw(15) << entryValue.sectionName
                << std::setw(10) << entryValue.offset
                << std::setw(10) << (entryValue.isDefined ? "Yes" : "No")
                << std::setw(10) << entryValue.scope << std::endl;
    }

    outFile << "--------------------------------------------------------------\n";

    // Print Section Table
    outFile << "\nSection Table:\n";
    outFile << std::string(50, '-') << std::endl;

    for (const auto& entry : sectionTable) {
        const std::string& sectionName = entry.first;
        const SectionEntry& sectionData = entry.second;

        // Print section header
        outFile << "#data." << sectionName << std::endl;

        // Print the opCodeList with 2 opcodes per line
        std::string line;
        for (size_t i = 0; i < sectionData.opCodeList.size(); ++i) {
            line += sectionData.opCodeList[i] + " ";
            if ((i + 1) % 2 == 0) { // Print 2 opcodes per line
                outFile << line << std::endl;
                line.clear();
            }
        }
        if (!line.empty()) {
            outFile << line << std::endl; // Print any remaining opcodes
        }

        outFile << std::endl;
    }

        // Print Relocation Table
    outFile << "\nRelocation Table:\n";

// Iterate through the relocation table
  for (const auto& entry : relocationTable) {
    const std::string& sectionName = entry.first;
    const std::vector<RelocationTableEntry>& relEntries = entry.second;

    // Print section header
    outFile << "#rela." << sectionName << std::endl;
    outFile << "Num |Offset| Type |   Symbol | Symbol ID | Section ID" << std::endl;
    outFile << "=============================================================" << std::endl;

    for (size_t i = 0; i < relEntries.size(); ++i) {
        const RelocationTableEntry& relEntry = relEntries[i];

        // Look up the section ID
        int sectionId = relEntry.sectionId;

        // Look up the symbol ID
        int symbolId = relEntry.symbolId;

        // Determine relocation type
        std::string relocationTypeStr = (relEntry.typeOfRelocation == local_absolute) ? "ABS_LOCAL_32" : "ABS_GLOBAL_32";

        // Print the relocation entry
        outFile << std::setw(2) << relEntry.id << " |"
                << std::setw(4)  << relEntry.sectionOffset << " |"
                << std::setw(12) << std::left << relocationTypeStr << " |"
                << std::setw(10) << std::left << relEntry.symbolName << " |"
                << std::setw(10) << std::left << symbolId << " |"
                << std::setw(10) << std::left << sectionId << std::endl;
    }
    outFile << "=============================================================" << std::endl;
  } 

    outFile.close();  // Ensure to close the file stream when done
}



void Assembler::printOutputForLinker(const std::string& filename) {
    std::ofstream outFile(filename, std::ios::binary);

    if (!outFile) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

        // Write the number of sections
    size_t size = sectionTable.size();
    outFile.write(reinterpret_cast<char*>(&size), sizeof(size_t));

    // Write each section entry
    for (auto& entry : sectionTable) {
        const std::string& sectionName = entry.first;
        SectionEntry& sectionData = entry.second;

        size = sectionName.size();
        outFile.write(reinterpret_cast<char*>(&size), sizeof(size_t));  // Write size of section name
        outFile.write(sectionName.data(), size);  // Write section name
        //mozda i da stavim koji indeks je u tabeli simbola?
        outFile.write(reinterpret_cast<char*>(&sectionData.idSection), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&sectionData.size), sizeof(int));
        std::vector<uint8_t> byteArray = hexStringToBytes(sectionData.opCodeList);
        //std::cout << "Byte Array: ";
        //int count = 0;  // Keep track of the number of bytes printed
        // for (uint8_t byte : byteArray) {
        //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";  // Print the byte in hex

        //     count++;
        //     if (count % 8 == 0) {
        //         std::cout << std::endl;  // After every 8 bytes, print a newline
        //     }
        // }
        outFile.write(reinterpret_cast<char*>(byteArray.data()), byteArray.size());
    }

    // Write the number of symbols
    size = symbolTable.size();
    outFile.write(reinterpret_cast<char*>(&size), sizeof(size_t));

    // Write each symbol entry
    for (auto& entry : symbolTable) {
        const std::string& symbolName = entry.first;
        SymbolTableEntry& symbolEntry = entry.second;

        size = symbolName.size();
        outFile.write(reinterpret_cast<char*>(&size), sizeof(size_t));  // Write size of symbol name
        outFile.write(symbolName.data(), size);  // Write symbol name
        
        outFile.write(reinterpret_cast<char*>(&symbolEntry.id), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&symbolEntry.sectionNumber), sizeof(int));
        size_t sectionNameSize = symbolEntry.sectionName.size();  // Store the size in a variable
        outFile.write(reinterpret_cast<char*>(&sectionNameSize), sizeof(size_t));  // Pass the variable to write
        // Size of section name
        outFile.write(symbolEntry.sectionName.data(), symbolEntry.sectionName.size()); // Section name
        outFile.write(reinterpret_cast<char*>(&symbolEntry.offset), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&symbolEntry.scope), sizeof(visibility));
    }

    
    size_t sectionCount = relocationTable.size();
    outFile.write(reinterpret_cast<const char*>(&sectionCount), sizeof(size_t)); // Write number of sections
    
    for (auto& section : relocationTable) {
        const std::string& sectionName = section.first;
        std::vector<RelocationTableEntry>& relocEntries = section.second;
        
        // Write the section name size and then the section name itself
        size_t sectionNameSize = sectionName.size();
        outFile.write(reinterpret_cast<char*>(&sectionNameSize), sizeof(size_t)); // Write section name size
        outFile.write(sectionName.data(), sectionNameSize);                             // Write section name
        
        // Write the number of relocation entries in this section
        size_t relocEntryCount = relocEntries.size();
        outFile.write(reinterpret_cast<char*>(&relocEntryCount), sizeof(size_t)); // Write number of relocation entries
        
        // Iterate over each RelocationTableEntry in the vector and write its content
        for (RelocationTableEntry& relocEntry : relocEntries) {
            // Write each RelocationTableEntry's fields

            // Write the symbolName (first the size, then the string itself)
            size_t symbolNameSize = relocEntry.symbolName.size();
            outFile.write(reinterpret_cast<char*>(&symbolNameSize), sizeof(size_t));  // Write symbol name size
            outFile.write(relocEntry.symbolName.data(), symbolNameSize);  
            outFile.write(reinterpret_cast<char*>(&relocEntry.symbolId), sizeof(int));

            // Write the int fields
            outFile.write(reinterpret_cast<char*>(&relocEntry.id), sizeof(int));
            outFile.write(reinterpret_cast<char*>(&relocEntry.sectionId), sizeof(int));

                        // Write the sectionName (first the size, then the string itself)
            size_t sectionNameSize = relocEntry.sectionName.size();
            outFile.write(reinterpret_cast<char*>(&sectionNameSize), sizeof(size_t));  // Write section name size
            outFile.write(relocEntry.sectionName.data(), sectionNameSize);    

            outFile.write(reinterpret_cast<char*>(&relocEntry.sectionOffset), sizeof(int));
            outFile.write(reinterpret_cast<char*>(&relocEntry.addend), sizeof(int));
            outFile.write(reinterpret_cast<char*>(&relocEntry.typeOfRelocation), sizeof(relocationType));
        }
    }
    outFile.close();  // Ensure to close the file stream when done
    //mozda treba da stavim da je current_section = 0
}


void Assembler::printJumpRelocationEntries() {
    std::cout << "Jump Relocation Entries:\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << std::setw(20) << "Section Name"
              << std::setw(20) << "Section Offset"
              << std::setw(20) << "Symbol Name" << std::endl;
    std::cout << "----------------------------------------------------------------------\n";

    // Iterate through all sections in the sectionTable
    for (const auto& sectionPair : sectionTable) {
        const std::string& sectionName = sectionPair.first;
        const SectionEntry& sectionData = sectionPair.second;

        // Iterate through all JumpRelocationEntry in the current section
        for (const auto& entry : sectionData.jumpRelocationVector) {
            std::cout << std::setw(20) << entry.sectionName
                      << std::setw(20) << entry.sectionOffset
                      << std::setw(20) << entry.symbolName << std::endl;
        }
    }

    std::cout << "----------------------------------------------------------------------\n";
}

void Assembler::printLiteralPool(const std::string& sectionName) {
    // Check if the section exists
    if (sectionTable.find(sectionName) == sectionTable.end()) {
        std::cerr << "Section " << sectionName << " not found!" << std::endl;
        return;
    }

    const poolOfLiterals& literalPool = sectionTable[sectionName].literalPool;

    std::cout << "Literal Pool for Section: " << sectionName << std::endl;
    std::cout << "-------------------------------------------------------------\n";
    
    // Print the pool location counter
    std::cout << "Pool Location Counter: " << literalPool.poolLC << std::endl;
    
    // Print the literal list
    std::cout << "Literals in Pool:\n";
    for (size_t i = 0; i < literalPool.literalList.size(); ++i) {
        std::cout << "  Literal[" << i << "]: " << literalPool.literalList[i] << std::endl;
    }

    // Print the offsets in the pool for each symbol
    std::cout << "Symbol Offsets in Pool:\n";
    for (const auto& entry : literalPool.offsetInPoolForSymbol) {
        std::cout << "  Symbol: " << entry.first 
                  << " -> Offset: " << entry.second << std::endl;
    }

    std::cout << "-------------------------------------------------------------\n";
}

void Assembler::splitOpCodeListInPlace() {
    // Iterate over each opcode in the list
    for (auto it = sectionTable[current_section].opCodeList.begin(); it != sectionTable[current_section].opCodeList.end(); /* nothing here */) {
        std::string opcode = *it;

        // Ensure the opcode is 8 characters long
        if (opcode.length() == 8) {
            // Erase the original 8-character string and get the iterator pointing to the next element
            it = sectionTable[current_section].opCodeList.erase(it);

            // Insert the four 2-character strings from the original 8-character string
            for (size_t i = 0; i < opcode.length(); i += 2) {
                it = sectionTable[current_section].opCodeList.insert(it, opcode.substr(i, 2));
                ++it; // Move the iterator forward after each insertion
            }
        } else {
            // If the length isn't 8, just move the iterator forward
            ++it;
        }
    }

    // Optional: print the new opCodeList to verify
    for (const auto& opcode : sectionTable[current_section].opCodeList) {
        std::cout << opcode << std::endl;
    }
}
// Helper function to convert a hexadecimal string to a byte array
std::vector<uint8_t> Assembler::hexStringToBytes(const std::vector<std::string>& opCodeList) {
    std::vector<uint8_t> bytes;

    // Iterate through each string in the opCodeList
    for (const std::string& hexString : opCodeList) {
        // For each string, process it two characters at a time
        for (std::size_t i = 0; i < hexString.size(); i += 2) {
            std::string byteString = hexString.substr(i, 2);  // Take two characters (e.g., "12", "34")
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));  // Convert to byte
            //cout<<static_cast<int>(byte)<<endl;
            bytes.push_back(byte);  // Add the byte to the vector
        }
    }

    return bytes;
}