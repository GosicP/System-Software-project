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
    if(current_symbol.scope == global && current_symbol.scope == external){
      //odnosno samo ovaj deo se desi za global i extern, a za undefined i ostalo
      current_symbol.isDefined=true;
      current_symbol.sectionName = current_section;
      current_symbol.offset = location_counter;
    } 
    if(current_symbol.scope == undefined){
      current_symbol.isDefined=true;
      current_symbol.sectionName = current_section;
      current_symbol.offset = location_counter;
      current_symbol.scope = local;
      //MOZDA DA DODAS DA SE OVO RADI SAMO AKO SU SIMBOL I DIREKTIVA U ISTOJ SEKCIJI
      vector<ForwardReferencingTableEntry> current_flink = current_symbol.flink;
        for (const auto& entry : current_flink) {
          string newCode = intToLittleEndianHexString(current_symbol.offset);

          SectionEntry& sectionToBacktrack = sectionTable[entry.sectionName];
          if(sectionToBacktrack.opCodeList[entry.sectionOffset/4] != "38F00000" && !matches92xF0000(sectionToBacktrack.opCodeList[entry.sectionOffset/4])){
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
        
      }
      for (auto& entry : relocationTable) {
        const string& sectionName = entry.first;  // The section name (key)
        vector<RelocationTableEntry>& relocationEntries = entry.second;  // The vector of RelocationTableEntry objects

        // Iterate through all RelocationTableEntry objects in the vector
        for (auto& relocationEntry : relocationEntries) {
           if(relocationEntry.symbolName==*symbol)
            // Update the symbolName and addend as needed
            // zasto da updateujem symbolName na section name?
            relocationEntry.symbolName = current_symbol.sectionName;
            relocationEntry.addend += current_symbol.offset;
          }
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
    //newSymbol.sectionNumber = NULL;
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
    //newSymbol.sectionNumber = NULL;
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
    currentSection.locationCounter = location_counter;
    currentSection.size = location_counter;
    currentSection.poolLocation = location_counter; 
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
    //newSymbol.sectionNumber = NULL;
    //0 local, 1 global, 2 extern, 3 section, 4 undefined
    newSymbol.scope = section;
    newSymbol.offset = -1;
    newSymbol.id = ++symbolIndexId;
    newSymbol.isDefined = true; //mozda i treba da bude false?
    symbolTable[*sectionSymbol] = newSymbol;

    SectionEntry newSection;
    newSection.idSection = ++sectionIndexId;
    newSection.size = 0;
    newSection.locationCounter = 0;
    sectionTable[*sectionSymbol] = newSection;
    //FALI MAPA RELOKACIJA

    RelocationTableEntry newRelocationEntry;
    newRelocationEntry.id = ++relocationIndexId;  // Assuming relocationEntryId is an existing variable tracking IDs
    newRelocationEntry.sectionName = current_section;  // Replace with the appropriate section name or leave as default if not used
    newRelocationEntry.sectionOffset = 0;  // Initialize with 0 or another appropriate value
    newRelocationEntry.addend = 0;  // Initialize with 0 or another appropriate value

    relocationTable[current_section].push_back(newRelocationEntry);

    location_counter = 0;
    cout<<"zavrsio sam pravljenje nove sekcije"<<endl;
  }
}

void Assembler::wordAssemblyDirective(int literalParam, string* wordSymbol) {
  if(literalParam!=0){
    string hexLiteral = intToLittleEndianHexString(literalParam);
    sectionTable[current_section].opCodeList.push_back(hexLiteral);
    location_counter += 4;
  }else{
    if (symbolTable.find(*wordSymbol) == symbolTable.end()){
      //simbol ne postoji u tabeli simbola, znaci da mora da se referise unapred
      
      SymbolTableEntry newSymbol;
      newSymbol.sectionName = "NONE";
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

      RelocationTableEntry rtEntry;
      rtEntry.id = ++relocationIndexId;
      rtEntry.sectionName = current_section;
      rtEntry.sectionOffset = location_counter;
      rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
      rtEntry.addend = 0; //za lokalni treba offset simbola
      rtEntry.typeOfRelocation = absolute; /* set the appropriate relocation type */
      relocationTable[current_section].push_back(rtEntry);
      
      //treba dodati tabelu relokacija
    }else{
      SymbolTableEntry& definedSymbol = symbolTable[*wordSymbol];
      if(definedSymbol.scope == local){
        //PROVERI OVAJ DEO
        string newCode = intToLittleEndianHexString(definedSymbol.offset);

        SectionEntry& currentSection = sectionTable[current_section];
        currentSection.opCodeList.push_back(newCode);
        
        //Ne znam da li treba relokacija ali stavio sam je
        RelocationTableEntry rtEntry;
        rtEntry.id = ++relocationIndexId;
        rtEntry.sectionName = current_section;
        rtEntry.sectionOffset = location_counter;
        rtEntry.symbolName = definedSymbol.sectionName; // Assuming wordSymbol is the symbol name
        rtEntry.addend = definedSymbol.offset;
        rtEntry.typeOfRelocation = absolute; /* set the appropriate relocation type */
        relocationTable[current_section].push_back(rtEntry);

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
        rtEntry.id = ++relocationIndexId;
        rtEntry.sectionName = current_section;
        rtEntry.sectionOffset = location_counter;
        rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
        rtEntry.addend = 0;
        rtEntry.typeOfRelocation = absolute; /* set the appropriate relocation type */
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
        rtEntry.id = ++relocationIndexId;
        rtEntry.sectionName = current_section;
        rtEntry.sectionOffset = location_counter;
        rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
        rtEntry.addend = 0;
        rtEntry.typeOfRelocation = absolute; /* set the appropriate relocation type */
        relocationTable[current_section].push_back(rtEntry);
      }

    }
    location_counter += 4;
  }
}

void Assembler::endAssemblyDirective() {
  SectionEntry& currentSection = sectionTable[current_section];
  cout<<"ime trenutne sekcije" << currentSection.idSection <<endl;
  currentSection.locationCounter = location_counter;
  currentSection.size = location_counter;
  currentSection.poolLocation = location_counter; 
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
      int newDisplacement = section.poolLocation - entry.sectionOffset + poolOffset;



      // cout<< "pool location je " << section.poolLocation<< endl;
      // cout<< "offset instrukicje na pocetku sekcije je " << entry.sectionOffset << endl;
      // cout<< "pool offset je " << poolOffset <<endl;
      // cout<< "displacement za ovaj jump je " << newDisplacement << endl;
      // cout<< "vrednost koja se nalazi na toj adresi u displacementu je "<< section.literalPool.literalList[poolOffset/4] << endl;
      
      // // Print the header
      // std::cout << std::left << std::setw(35) << "Description"
      //           << std::setw(10) << "Value" << std::endl;
      // std::cout << std::string(45, '-') << std::endl;

      // // Print the values in table format

      // std::cout << std::left << std::setw(35) << "Pool location"
      //           << std::setw(10) << section.poolLocation << std::endl;

      std::cout << std::left << std::setw(35) << "Jump value"
                << std::setw(10) << jmpValue << std::endl;

      // std::cout << std::left << std::setw(35) << "Offset instruction at start of section"
      //           << std::setw(10) << entry.sectionOffset << std::endl;

      // std::cout << std::left << std::setw(35) << "Pool offset"
      //           << std::setw(10) << poolOffset << std::endl;

      std::cout << std::left << std::setw(35) << "Displacement for this jump"
                << std::setw(10) << newDisplacement << std::endl;

      // std::cout << std::left << std::setw(35) << "Value at displacement address"
      //           << std::setw(10) << section.literalPool.literalList[poolOffset/4] << std::endl;

      
      string newCode = section.opCodeList[entry.sectionOffset/4];
      string codeWithDisplacement = modifyCodeWithDisplacement(newCode, newDisplacement);
      cout<<codeWithDisplacement<<endl;

      SectionEntry& sectionToBacktrack = sectionTable[entry.sectionName];
      sectionToBacktrack.opCodeList[entry.sectionOffset/4] = codeWithDisplacement;
    }
  }
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

        RelocationTableEntry rtEntry;
        rtEntry.id = ++relocationIndexId;
        rtEntry.sectionName = current_section;
        rtEntry.sectionOffset = location_counter;
        rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
        rtEntry.addend = 0; //za lokalni treba offset simbola
        rtEntry.typeOfRelocation = absolute; /* set the appropriate relocation type */
        relocationTable[current_section].push_back(rtEntry);
        
    } else {
        SymbolTableEntry& definedSymbol = symbolTable[*wordSymbol];
        if (definedSymbol.scope == local) {
            
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
            
            RelocationTableEntry rtEntry;
            rtEntry.id = ++relocationIndexId;
            rtEntry.sectionName = current_section;
            rtEntry.sectionOffset = location_counter;
            rtEntry.symbolName = definedSymbol.sectionName; // Assuming wordSymbol is the symbol name
            rtEntry.addend = definedSymbol.offset;
            rtEntry.typeOfRelocation = absolute; /* set the appropriate relocation type */
            relocationTable[current_section].push_back(rtEntry);

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

            RelocationTableEntry rtEntry;
            rtEntry.id = ++relocationIndexId;
            rtEntry.sectionName = current_section;
            rtEntry.sectionOffset = location_counter;
            rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
            rtEntry.addend = 0;
            rtEntry.typeOfRelocation = absolute; /* set the appropriate relocation type */
            relocationTable[current_section].push_back(rtEntry);

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

            RelocationTableEntry rtEntry;
            rtEntry.id = ++relocationIndexId;
            rtEntry.sectionName = current_section;
            rtEntry.sectionOffset = location_counter;
            rtEntry.symbolName = *wordSymbol; // Assuming wordSymbol is the symbol name
            rtEntry.addend = 0;
            rtEntry.typeOfRelocation = absolute; /* set the appropriate relocation type */
            relocationTable[current_section].push_back(rtEntry);
        }
    }
    location_counter += 4;
}
void Assembler::storeData(int literal, string* symbol){
  current_data_literal = literal;
  current_data_symbol = symbol;
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
  if(current_data_symbol==nullptr){
    //u pitanju je literal
    std::string newHexCode = constructHexCodeForLD(0x92, registerDst);
    processLiteralForInstruction(current_data_literal, newHexCode);
  }else{
    //u pitanju je simbol
    std::string newHexCode = constructHexCodeForLD(0x92, registerDst);
    processSymbolForInstruction(current_data_symbol, newHexCode);
  }
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
    instruction |= (regDstNumber << 20); // Place regDstNumber in the next 4 bits
    instruction |= (regDstNumber << 16); // Place regDstNumber in the next 4 bits after that
    instruction |= (regSrcNumber << 12); // Place regSrcNumber in the next 4 bits

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
    instruction |= (regDstNumber << 20); // Place regDstNumber in the next 4 bits
    instruction |= (regDstNumber << 16); // Place regDstNumber in the next 4 bits after that
    instruction |= (regSrcNumber << 12); // Place regSrcNumber in the next 4 bits

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
    instruction |= (regDstNumber << 20); // Place regDstNumber in the next 4 bits
    instruction |= (regDstNumber << 16); // Place regDstNumber in the next 4 bits after that
    instruction |= (regSrcNumber << 12); // Place regSrcNumber in the next 4 bits

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
    instruction |= (regDstNumber << 20); // Place regDstNumber in the next 4 bits
    instruction |= (regDstNumber << 16); // Place regDstNumber in the next 4 bits after that
    instruction |= (regSrcNumber << 12); // Place regSrcNumber in the next 4 bits

    // Convert the instruction to a hex string manually
    char hexInstruction[9];
    snprintf(hexInstruction, sizeof(hexInstruction), "%08X", instruction);

    // Push the hex string to the opCodeList in the current section
    sectionTable[current_section].opCodeList.push_back(hexInstruction);

    location_counter += 4;
    
    // Output for debugging
    //cout << "Pushed instruction: " << hexInstruction << endl;
}



int Assembler::convertHexToInt(string* hexString){
  string hexWithoutPrefix = hexString->substr(2);
  int decimalValue = std::stoi(hexWithoutPrefix, nullptr, 16);
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

  // Replace the last three characters of the code with the displacement
  std::string modifiedCode = code.substr(0, 5) + displacementHex;

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
