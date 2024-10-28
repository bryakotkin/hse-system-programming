SRC_PROJECT=main.cpp
EXE_PROJECT=a.out
SUB_PROGRAM=./b.out

all: build run
build:
	g++ -o ${EXE_PROJECT} ${SRC_PROJECT} -std=c++17
run:
	./${EXE_PROJECT} ${SUB_PROGRAM}
clean:
	rm -f ${EXE_PROJECT}