lexer: misc/lexer.lpp
	flex -o build/lex.yy.cpp misc/lexer.lpp

parser: misc/parser.ypp
	bison  -o build/parser.tab.cpp -d misc/parser.ypp

assembler: parser lexer src/main.cpp src/assembler.cpp
	g++ -o assembler src/main.cpp build/parser.tab.cpp build/lex.yy.cpp src/assembler.cpp

linker: src/linkerMain.cpp src/linker.cpp
	g++ -o linker src/linkerMain.cpp src/linker.cpp

clean:
	rm assembler

linker_init:
	./linker -hex -o program.hex handler.o isr_software.o isr_terminal.o isr_timer.o math.o main.o
	./linker -hex -o program.hex main.o math.o handler.o isr_terminal.o isr_software.o isr_timer.o

assembler_init:
	./assembler -o main.o test/main.s
	./assembler -o math.o test/math.s
	./assembler -o handler.o test/handler.s
	./assembler -o isr_software.o test/isr_software.s
	./assembler -o isr_terminal.o test/isr_terminal.s
	./assembler -o isr_timer.o test/isr_timer.s