@echo off
set CC=C:\msys64\mingw64\bin\gcc.exe
set CXX=C:\msys64\mingw64\bin\g++.exe
set FLAGS=-I. -I../OpenJK/codemp -I../OpenJK/shared -I../OpenJK/lib/gsl-lite/include -I../OpenJK/lib/minizip/include -I C:/msys64/mingw64/include -DDEDICATED -DFINAL_BUILD -D_CONSOLE -std=c++11 -O3 -DNDEBUG
set LIBS=-L C:/msys64/mingw64/lib -lm -lstdc++ -ljansson

echo Compiling...
%CXX% %FLAGS% -c cl_parse.cpp -o cl_parse.o
%CXX% %FLAGS% -c cmd.cpp -o cmd.o
%CXX% %FLAGS% -c deps.cpp -o deps.o
%CXX% %FLAGS% -c main.cpp -o main.o
%CXX% %FLAGS% -c utils.cpp -o utils.o
%CXX% %FLAGS% -c ../OpenJK/codemp/qcommon/huffman.cpp -o huffman.o
%CXX% %FLAGS% -c ../OpenJK/codemp/qcommon/msg.cpp -o msg.o
%CC%  %FLAGS% -c ../OpenJK/shared/qcommon/q_math.c -o q_math.o
%CXX% %FLAGS% -c ../OpenJK/codemp/qcommon/q_shared.cpp -o q_shared.o
%CC%  %FLAGS% -c ../OpenJK/shared/qcommon/q_string.c -o q_string.o

echo Linking...
%CXX% -o jkdemometadata.exe cl_parse.o cmd.o deps.o main.o utils.o huffman.o msg.o q_math.o q_shared.o q_string.o %LIBS%
