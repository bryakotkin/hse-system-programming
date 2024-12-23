SRC_SERVER=server.cpp
SRC_CLIENT=client.cpp
EXE_SERVER=server.out
EXE_CLIENT=client.out

all:
	make bs bc
bs:
	g++ -o ${EXE_SERVER} ${SRC_SERVER} -std=c++17
bc:
	g++ -o ${EXE_CLIENT} ${SRC_CLIENT} -std=c++17
clean:
	rm ${EXE_PROJECT} ${EXE_CLIENT}