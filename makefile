lexer: misc/lexer.lpp
	flex -o build/lex.yy.cpp misc/lexer.lpp

parser: misc/parser.ypp
	bison  -o build/parser.tab.cpp -d misc/parser.ypp

assembler: parser lexer src/main.cpp src/assembler.cpp
	g++ -o assembler src/main.cpp build/parser.tab.cpp build/lex.yy.cpp src/assembler.cpp

linker: src/linkerMain.cpp src/linker.cpp
	g++ -o linker src/linkerMain.cpp src/linker.cpp

emulator: src/emulator.cpp inc/emulator.hpp
	g++ src/emulator.cpp inc/emulator.hpp -o emulator

clean:
	rm assembler

linker_init:
	./linker -hex -place=my_code@0x40000000 -place=math@0xF0000000 -o program.hex handler.o isr_software.o isr_terminal.o isr_timer.o math.o main.o
	./linker -hex -place=my_code@0x40000000 -place=math@0xF0000000 -o program.hex handler.o math.o main.o isr_terminal.o isr_timer.o isr_software.o

assembler_init:
	./assembler -o main.o test/main.s
	./assembler -o math.o test/math.s
	./assembler -o handler.o test/handler.s
	./assembler -o isr_software.o test/isr_software.s
	./assembler -o isr_terminal.o test/isr_terminal.s
	./assembler -o isr_timer.o test/isr_timer.s

emulator_init:
	./emulator program.hex
