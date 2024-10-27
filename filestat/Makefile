SRC_PROJECT=main.cpp
EXE_PROJECT=a.out

all: build run
build:
	g++ -o ${EXE_PROJECT} ${SRC_PROJECT} -std=c++17 -lssl -lcrypto
run:
	./${EXE_PROJECT}
clean:
	rm -f ${EXE_PROJECT}