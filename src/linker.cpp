#include "../inc/linker.hpp"


Linker::Linker() {}

void Linker::processPlaceCommand(const std::string& placeCommand) {
    std::string sectionName, startAddrStr;
    
    // Find the positions of '=' and '@'
    size_t equalPos = placeCommand.find('=');
    size_t atPos = placeCommand.find('@');

    // Ensure both '=' and '@' exist in the placeCommand
    if (equalPos != std::string::npos && atPos != std::string::npos && atPos > equalPos) {
        // Extract the section name and start address string
        sectionName = placeCommand.substr(equalPos + 1, atPos - equalPos - 1);
        startAddrStr = placeCommand.substr(atPos + 1);

        // Convert the start address string to unsigned long
        unsigned long startAddr = convertHexToUL(startAddrStr);

        // Store the section name and address in the map
        sectionPlaceMap[sectionName] = startAddr;
    } else {
        std::cerr << "Error: Invalid place command format: " << placeCommand << std::endl;
    }
}

unsigned long Linker::convertHexToUL(const std::string& hexStr) {
    return std::stoul(hexStr, nullptr, 16); 
}



void Linker::readAssembly(int argc, char* argv[]) {
  string outputFile = "linkerOutput.hex";
  int inputIndexStart = 1;
  bool hexFlag = false;

  while (inputIndexStart < argc) {
    std::string arg = argv[inputIndexStart];

    if (arg == "-o") {
        outputFile = argv[++inputIndexStart];  // Move to the next argument and capture the output file name
        inputIndexStart++;
    } 
    else if (arg == "-hex") {
        hexFlag = true;  // Set the hex flag
        inputIndexStart++;
    } else if(string(argv[inputIndexStart]).find("place")!=std::string::npos){
        cout<< "Usao sam u place" << endl;
        processPlaceCommand(argv[inputIndexStart]);
        inputIndexStart++;
    }else if (arg[0] == '-') {
    // Handle other flags starting with '-'
    std::cout << "Unrecognized option: " << arg << std::endl;
    inputIndexStart++;
    }
    else {
        break;  // Exit the loop if the argument doesn't match any known flag
    }
  }
  if(hexFlag==false) {
    cerr<<"Nema hex opcije! " <<endl;
    exit(-1);
  } 

  vector<ifstream>* inputFiles = new vector<ifstream>();
  unsigned int startAddrUnplaced = 0;
    for(int i = 0; i<argc-inputIndexStart; i++){ //za svaki od fajlova...
      ifstream inputFile(argv[inputIndexStart + i], ios::binary);
      if(inputFile.is_open()){
        // Number of sections in the file
        size_t numSectionsInFile;
        inputFile.read(reinterpret_cast<char*>(&numSectionsInFile), sizeof(size_t)); // Read number of sections

        for (int sectionIndex = 0; sectionIndex < numSectionsInFile; sectionIndex++) {
            // Read the section name size and the section name itself
            size_t sectionNameLength;
            inputFile.read(reinterpret_cast<char*>(&sectionNameLength), sizeof(size_t)); // Read the size of the section name

            std::string sectionName;
            sectionName.resize(sectionNameLength); // Resize the string to fit the section name
            inputFile.read(&sectionName[0], sectionNameLength); // Read the section name

            // Read section ID and size
            int sectionID;
            int sectionByteSize; //ovo je zapravo samo size nego ne znam da pisem imena
            inputFile.read(reinterpret_cast<char*>(&sectionID), sizeof(int)); // Read section ID
            inputFile.read(reinterpret_cast<char*>(&sectionByteSize), sizeof(int)); // Read section size in bytes

            // // Process or store the data, for now we'll print it
            // cout << "Section Name: " << sectionName << endl;
            // cout << "Section ID: " << sectionID << endl;
            // cout << "Section Byte Size: " << sectionByteSize << endl;

            unsigned int startingAdressSection;
            if(sectionTableLinker.find(sectionName) == sectionTableLinker.end()){
              //entry doesn't exist
              SectionEntryLinker symbolTableRow = sectionTableLinker[sectionName];
              bool isPlacedTemp = false;
              if (sectionPlaceMap.find(sectionName) != sectionPlaceMap.end()) { 
                  isPlacedTemp = true;
                  startingAdressSection = sectionPlaceMap[sectionName];
                  // Iterate through all placed sections in the linker symbol table
                for (auto sectionIter = sectionTableLinker.begin(); sectionIter != sectionTableLinker.end(); sectionIter++) {

                    // Only check placed sections
                    if (sectionIter->second.isPlaced) {

                        unsigned int existingSectionStart = sectionIter->second.startAdress;
                        unsigned int existingSectionEnd = existingSectionStart + sectionIter->second.size;

                        unsigned int newSectionEnd = startingAdressSection + sectionByteSize;

                        // Check for overlap between the current section and the new section
                        bool overlapsWithExisting = 
                            (newSectionEnd > existingSectionStart && startingAdressSection <= existingSectionStart) ||  // New section ends after the start of an existing one
                            (existingSectionEnd > startingAdressSection && existingSectionStart <= startingAdressSection);    // New section starts within an existing one

                        // If overlap is detected, stop the linker and report an error
                        if (overlapsWithExisting) {
                            // Output an error message and exit
                            cerr << "Linking error: Sections " 
                                << sectionName << " and " 
                                << sectionIter->first << " overlap!\n";

                            exit(-1);
                        }
                    }
                }

                // Update the start address for unplaced sections if necessary
                if (startingAdressSection + sectionByteSize > startAddrUnplaced) {
                    startAddrUnplaced = startingAdressSection + sectionByteSize;
                    cout<<"unplaced adresa za sekciju " << sectionName << " je " << startAddrUnplaced << endl;
                } 
                //mozda da dodas za rezervisan prostor proveru (da nije vise od ffffffff)

              } else {
                  cout<<"usao sam u else granu tokom obradjivanja my_data"<<endl;
                  startingAdressSection = 0;
                  notPlacedSections.push_back(sectionName);
                } 
            // Read the opcode (bytecode) data for the section

            std::vector<uint8_t> opCodeData(sectionByteSize); // Prepare a buffer to store the opcode data
            inputFile.read(reinterpret_cast<char*>(opCodeData.data()), sectionByteSize); // Read the opcode byte array

            // cout << "OpCode Data: ";
            // for (auto byte : opCodeData) {
            //     printf("%02X ", byte);  // Print each byte in hexadecimal format
            // }
            cout << endl;
            SectionEntryLinker newSectionEntry;
            newSectionEntry.idSection = ++sectionIndexId;
            newSectionEntry.size = sectionByteSize;
            newSectionEntry.isPlaced = isPlacedTemp;
            newSectionEntry.startAdress = startingAdressSection;
            newSectionEntry.opCodeList = opCodeData;

            sectionTableLinker[sectionName] = newSectionEntry;

            SymbolTableEntryLinker newSymbolEntry;
            newSymbolEntry.sectionNumber = newSectionEntry.idSection;
            newSymbolEntry.idSymbol = ++symbolIndexId;
            newSymbolEntry.sectionName = sectionName;
            newSymbolEntry.offset = startingAdressSection;
            newSymbolEntry.scope = visibility::section;

            symbolTableLinker[sectionName] = newSymbolEntry;
            }else{
                // Entry exists
                std::cout << "Section exists: " << sectionName << std::endl;
                SectionEntryLinker& currentSection = sectionTableLinker[sectionName];
                startingAdressSection=currentSection.size + currentSection.startAdress;
                if (currentSection.isPlaced) {
                 for (auto sectionIter = sectionTableLinker.begin(); sectionIter != sectionTableLinker.end(); sectionIter++) {
                    if(sectionIter->first != sectionName){
                        // Only check placed sections
                        //ovo mozda treba da se permutuje sa forom

                            unsigned int existingSectionStart = sectionIter->second.startAdress;
                            unsigned int existingSectionEnd = existingSectionStart + sectionIter->second.size;

                            unsigned int newSectionEnd = startingAdressSection + sectionByteSize;

                            // Check for overlap between the current section and the new section
                            bool overlapsWithExisting = 
                                (newSectionEnd > existingSectionStart && startingAdressSection <= existingSectionStart) ||  // New section ends after the start of an existing one
                                (existingSectionEnd > startingAdressSection && existingSectionStart <= startingAdressSection);    // New section starts within an existing one

                            // If overlap is detected, stop the linker and report an error
                            if (overlapsWithExisting) {
                                // Output an error message and exit
                                cerr << "Linking error: Sections " 
                                    << sectionName << " and " 
                                    << sectionIter->first << " overlap!\n";

                                exit(-1);
                            }
                        }
                    }
                    if (startingAdressSection + sectionByteSize > startAddrUnplaced) {
                        startAddrUnplaced = startingAdressSection + sectionByteSize;
                    }
                }
                currentSection.size += sectionByteSize;
                std::vector<uint8_t> opCodeData(sectionByteSize); // Prepare a buffer to store the opcode data
                inputFile.read(reinterpret_cast<char*>(opCodeData.data()), sectionByteSize); // Read the opcode byte array
                currentSection.opCodeList.insert(currentSection.opCodeList.end(), opCodeData.begin(), opCodeData.end());
            }

            symTableFileTracker newTracker;
            newTracker.inputFile = argv[i];
            newTracker.inputFileIndex = i;
            newTracker.sectionName = sectionName;

            descriptorTracker.push_back(newTracker);
            //onda ovde ide neki file tracker????
        }

        inputFiles->push_back(move(inputFile));
      }
    }
    
    unsigned int currentStartAdress = startAddrUnplaced;
    for(auto unplacedSectionName : notPlacedSections){
        cout<< "obradjujem unplaced sekciju " << unplacedSectionName << " i njen kod je "<<currentStartAdress<<endl;
        SectionEntryLinker& unplacedSection = sectionTableLinker[unplacedSectionName];
        unplacedSection.startAdress = currentStartAdress;
        currentStartAdress += unplacedSection.size;

        SymbolTableEntryLinker& newSymbolEntry = symbolTableLinker[unplacedSectionName];
        newSymbolEntry.offset = currentStartAdress;
        //TODO: OVDE TREBA FILE TRACKER KOJI NE RAZUMEM
    }

    //TODO sada radim sa tabelom simbola
    for(int i = 0; i < inputFiles->size(); i++) {
        if((*inputFiles)[i].is_open()) {
            //cout<<"trenutno obradjujem fajl " << argv[inputIndexStart + i]<<endl;
            // Read the number of symbols
            size_t symbsNum;
            (*inputFiles)[i].read(reinterpret_cast<char*>(&symbsNum), sizeof(size_t));  // Number of symbols

            for (size_t j = 0; j < symbsNum; j++) {
                // Read the symbol name
                std::string symbolName;
                size_t symbolNameSize;
                (*inputFiles)[i].read(reinterpret_cast<char*>(&symbolNameSize), sizeof(size_t));  // Size of symbol name
                symbolName.resize(symbolNameSize);
                (*inputFiles)[i].read(&symbolName[0], symbolNameSize);  // Symbol name

                // Read the symbol entry details
                int id, sectionNumber, offset;
                size_t sectionNameSize;
                visibility scope;
                std::string sectionNameForSymbol;

                (*inputFiles)[i].read(reinterpret_cast<char*>(&id), sizeof(int));  // Read symbol ID
                (*inputFiles)[i].read(reinterpret_cast<char*>(&sectionNumber), sizeof(int));  // Read section number
                (*inputFiles)[i].read(reinterpret_cast<char*>(&sectionNameSize), sizeof(size_t));  // Read size of section name

                sectionNameForSymbol.resize(sectionNameSize);
                (*inputFiles)[i].read(&sectionNameForSymbol[0], sectionNameSize);  // Read section name

                (*inputFiles)[i].read(reinterpret_cast<char*>(&offset), sizeof(int));  // Read offset
                (*inputFiles)[i].read(reinterpret_cast<char*>(&scope), sizeof(visibility));  // Read scope

                

                if (scope == global) {
                    // Check if the symbol is already defined globally
                    if (definedGlobalSymbols.find(symbolName) != definedGlobalSymbols.end()) {
                        std::cerr << "Error: Symbol '" << symbolName << "' is defined more than once as global!" << std::endl;
                    } else {
                        definedGlobalSymbols.insert(symbolName);
                    }
                    

                    // If it's defined as global, remove it from the list of external symbols
                    undefinedExternalSymbols.erase(symbolName);
                } else if (scope == external) {
                    // Track external symbols
                    
                    if (definedGlobalSymbols.find(symbolName) == definedGlobalSymbols.end()) {
                        undefinedExternalSymbols.insert(symbolName);
                    }
                }
                int newAdress = 0;
                if(scope == global) {
                    int newSectionIndex;
                    string newSectionName;
                    for(auto descriptor : descriptorTracker){
                        if(descriptor.inputFileIndex == i && descriptor.sectionName == sectionNameForSymbol){ //ovaj drugi uslov mozda treba da promenim?
                            //formula je pocetak sekcije + offset datog simbola
                            newAdress = offset + sectionTableLinker[sectionNameForSymbol].startAdress;
                            cout<< " za simbol " << symbolName << " nova adresa u linkeru je " << newAdress << endl;

                            newSectionName = sectionNameForSymbol;
                            newSectionIndex = sectionTableLinker[sectionNameForSymbol].idSection;
                        }
                    }
                    // Add the symbol to the symbol table
                    SymbolTableEntryLinker symbolEntry;
                    symbolEntry.idSymbol = ++symbolIndexId;
                    symbolEntry.sectionNumber = sectionNumber;
                    symbolEntry.sectionName = sectionNameForSymbol;
                    symbolEntry.offset = newAdress;
                    symbolEntry.scope = scope;

                    // Insert the symbol into the symbolTable (assuming symbolTable is a map<string, SymbolTableEntry>)
                    symbolTableLinker[symbolName] = symbolEntry;
                }
                cout<<"dosao sam do ovde pre vremena" << endl;

            }

        } else {
            std::cerr << "Error: Could not open input file " << i << std::endl;
        }
    }


    // Check if any external symbols are still undefined
    for (const auto& symbol : undefinedExternalSymbols) {
        std::cerr << "Error: External symbol '" << symbol << "' is never defined as global in any file!" << std::endl;
        exit(-1);
    }


    //ostaje tabela relokacija

    // Iterate through all the input files
    std::ofstream outFile("reloc_entries.txt", std::ios::out);
    for (int i = 0; i < inputFiles->size(); i++) {
        
        if ((*inputFiles)[i].is_open()) {
            // Read the number of sections in the relocation table
            size_t sectionCount;
            (*inputFiles)[i].read(reinterpret_cast<char*>(&sectionCount), sizeof(size_t));

            // Loop through each section
            for (size_t j = 0; j < sectionCount; j++) {
                // Read section name
                size_t sectionNameSize;
                (*inputFiles)[i].read(reinterpret_cast<char*>(&sectionNameSize), sizeof(size_t));
                
                std::string sectionName;
                sectionName.resize(sectionNameSize);
                (*inputFiles)[i].read(&sectionName[0], sectionNameSize);

                // Read the number of relocation entries for this section
                size_t relocEntryCount;
                (*inputFiles)[i].read(reinterpret_cast<char*>(&relocEntryCount), sizeof(size_t));

                // Loop through each relocation entry
                for (size_t k = 0; k < relocEntryCount; k++) {
                    // Local variables to hold relocation entry details
                    std::string symbolName;
                    int symbolId, RelocId, sectionId, sectionOffset, addend;
                    relocationType typeOfRelocation;

                    // Read the symbol name size and symbol name
                    size_t symbolNameSize;
                    (*inputFiles)[i].read(reinterpret_cast<char*>(&symbolNameSize), sizeof(size_t));
                    
                    symbolName.resize(symbolNameSize);
                    (*inputFiles)[i].read(&symbolName[0], symbolNameSize);

                    // Read symbolId, id, sectionId, sectionOffset, addend
                    (*inputFiles)[i].read(reinterpret_cast<char*>(&symbolId), sizeof(int));
                    (*inputFiles)[i].read(reinterpret_cast<char*>(&RelocId), sizeof(int));
                    (*inputFiles)[i].read(reinterpret_cast<char*>(&sectionId), sizeof(int));

                    // Read section name size and section name
                    size_t relocSectionNameSize;
                    std::string relocSectionName;
                    (*inputFiles)[i].read(reinterpret_cast<char*>(&relocSectionNameSize), sizeof(size_t));
                    relocSectionName.resize(relocSectionNameSize);
                    (*inputFiles)[i].read(&relocSectionName[0], relocSectionNameSize);

                    // Read sectionOffset, addend, and typeOfRelocation
                    (*inputFiles)[i].read(reinterpret_cast<char*>(&sectionOffset), sizeof(int));
                    (*inputFiles)[i].read(reinterpret_cast<char*>(&addend), sizeof(int));
                    (*inputFiles)[i].read(reinterpret_cast<char*>(&typeOfRelocation), sizeof(relocationType));

                    // // Print the read data to verify it
                    // std::cout << "Relocation Entry #" << k + 1 << " for Section: " << sectionName << std::endl;
                    // std::cout << "Symbol Name: " << symbolName << ", Symbol ID: " << symbolId << std::endl;
                    // std::cout << "ID: " << RelocId << ", Section ID: " << sectionId << std::endl;
                    // std::cout << "Relocation Section Name: " << relocSectionName << std::endl;
                    // std::cout << "Section Offset: " << sectionOffset << ", Addend: " << addend << std::endl;
                    // std::cout << "Relocation Type: " << typeOfRelocation << std::endl;
                    // std::cout << "----------------------------------------" << std::endl;

                    
                     // Write the relocation entry details to the file
                    outFile << "Relocation Entry #" << k + 1 << " for Section: " << sectionName << std::endl;
                    outFile << "Symbol Name: " << symbolName << ", Symbol ID: " <<symbolId << std::endl;
                    outFile << "ID: " << RelocId << ", Section ID: " << sectionId << std::endl;
                    outFile << "Relocation Section Name: " << sectionName << std::endl;
                    outFile << "Section Offset: " << sectionOffset << ", Addend: " << addend << std::endl;
                    outFile << "Relocation Type: " << typeOfRelocation << std::endl;
                    
                    int sectionStartAddr;
                    int patchValue;
                    for(auto descriptor : descriptorTracker){
                        if(descriptor.inputFileIndex == i && descriptor.sectionName == relocSectionName){ //ovaj drugi uslov mozda treba da promenim?
                            sectionStartAddr = sectionTableLinker[relocSectionName].startAdress;
                        }
                    }
                    int patchAddress = sectionStartAddr + sectionOffset; //ovo je pomeraj od samog pocetka, ovo je APSOLUTNA
                    //ADRESA SIMBOLA
                    //MENI ocigledno treba opet relativna vrednost od pocetka sekcije,zar to nije samo section offset?
                    //nadjes sekciju koja ti treba, i uzmes elemnt koji ti treba...
                    cout<<relocSectionName<<endl;
                    cout<<std::hex << std::uppercase << std::setw(2) << std::setfill('0') <<static_cast<int>(sectionTableLinker[relocSectionName].opCodeList[sectionOffset])<<endl;
                    if(typeOfRelocation = global_absolute) {
                        patchValue = symbolTableLinker[symbolName].offset;
                    }else{
                        patchValue = pocetnaVrednostSekcije("moras da odlucis da li je sekcija u symbol ili section name") + addend;
                    }
                    //proveri da li ovo funkcionise kada imas place
                    outFile << "Patch Adress for symbol: "<< symbolName << " is " << sectionStartAddr + sectionOffset << std::endl;
                    //dodaj i sectionOffset da mogu lako da izbrojim gde je patch
                    outFile << "And it's patching byte" <<std::hex << std::uppercase << std::setw(2) << std::setfill('0') <<static_cast<int>(sectionTableLinker[relocSectionName].opCodeList[sectionOffset])<<endl;
                    outFile << "Patch Value for symbol: "<< symbolName << " is " << patchValue << std::endl;
                    outFile << "----------------------------------------" << std::endl;
                }
            }
        } else {
            std::cerr << "Error: Could not open input file " << i << std::endl;
        }
    }
    outFile.close();
    
}

void Linker::printTablesToFile(const std::string& filename) {
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
            << std::setw(10) << "Scope" << std::endl;
    outFile << "--------------------------------------------------------------\n";

    for (const auto& entry : symbolTableLinker) {
        const std::string& symbol = entry.first;
        const SymbolTableEntryLinker& entryValue = entry.second;

        outFile << std::setw(15) << symbol
                << std::setw(15) << entryValue.sectionNumber
                << std::setw(10) << entryValue.idSymbol
                << std::setw(15) << entryValue.sectionName
                << std::setw(10) << std::hex << entryValue.offset
                << std::setw(10) << entryValue.scope << std::endl;
    }

    outFile << "--------------------------------------------------------------\n";

    // Print Section Table Header
    outFile << "\nSection Table:\n";
    outFile << std::string(50, '-') << std::endl;

    // Iterate through the sectionTableLinker and print details
    for (const auto& entry : sectionTableLinker) {
        const std::string& sectionName = entry.first;
        const SectionEntryLinker& sectionData = entry.second;

        // Print section name header in the desired format
        outFile << "#data." << sectionName << std::endl;

        // Print the additional section details: ID, Size, Is Placed, and Start Address
        outFile << std::setw(20) << "ID Section: " << sectionData.idSection << "\n"
                << std::setw(20) << "Size: " << std::hex <<sectionData.size << "\n"
                << std::setw(20) << "Is Placed: " << (sectionData.isPlaced ? "Yes" : "No") << "\n"
                << std::setw(20) << "Start Address: " << std::hex << sectionData.startAdress << std::dec << "\n";

        // Print the opCodeList (2 opcodes per line)
        std::string line;
        for (size_t i = 0; i < sectionData.opCodeList.size(); ++i) {
            // Convert each opcode byte into a two-character hex string
            std::stringstream opcodeStream;
            opcodeStream << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(sectionData.opCodeList[i]);
            line += opcodeStream.str() + " ";

            // After printing 2 opcodes, start a new line
            if ((i + 1) % 8 == 0) {
                outFile << line << std::endl;
                line.clear();
            }
        }
        if (!line.empty()) {
            outFile << line << std::endl; // Print remaining opcodes (if any)
        }

        outFile << std::endl; // Separate sections with a blank line
    }

    outFile.close();  // Ensure to close the file stream when done
}
