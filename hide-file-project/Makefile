SRC_PROJECT=main.cpp
EXE_PROJECT=a.out

br: build run

build:
	g++ -o ${EXE_PROJECT} ${SRC_PROJECT} -std=c++17
run:
	./a.out $(FILE)
clean:
	rm -f ${EXE_PROJECT}