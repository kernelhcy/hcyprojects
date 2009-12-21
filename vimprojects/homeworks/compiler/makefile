main : main.o imp/FileAction_imp.o imp/LexicalAnalysis_imp.o imp/SyntaxAnalysis_imp.o headers/FileAction.h headers/LexicalAnalysis.h headers/SyntaxAnalysis.h
	g++ main.o imp/FileAction_imp.o imp/LexicalAnalysis_imp.o imp/SyntaxAnalysis_imp.o -o main

main.o : main.cpp headers/FileAction.h headers/LexicalAnalysis.h headers/SyntaxAnalysis.h
	g++ -c main.cpp -o main.o

FileAction_imp.o : imp/FileAction_imp.cpp headers/FileAciotn.h
	g++ -c imp/FileAction_imp.cpp -o imp/FileAction_imp.o
LexicalAnalysis_imp.o : headers/LexicalAnalysis.h imp/LexicalAnalysis_imp.cpp headers/FileAction.h
	g++ -c imp/LexicalAnalysis_imp.cpp -o imp/LexicalAnalysis_imp.o
SyntaxAnalysis_imp.o : headers/SyntaxAnalysis.h headers/LexicalAnalysis.h headers/FileAction.h imp/SyntaxAnalysis_imp.cpp
	g++ -c imp/SyntaxAnalysis_imp.cpp -o imp/SyntaxAnalysis_imp.o

clean:
	find . -name '*.o' -exec rm -f {} ';'
	rm main
