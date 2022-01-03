EXECS=tp2
MPICC?=mpicxx
BUILD=./build/
SRC=./src/
INCLUDE=./include/

all: ${EXECS}

${EXECS}: $(BUILD)main.o
	${MPICC} -o ${EXECS} $(BUILD)main.o

$(BUILD)main.o: $(SRC)main.cpp
	${MPICC} -c $(SRC)main.cpp -o $(BUILD)main.o

run: ${EXECS}
	mpiexec -n 5 ./${EXECS}

clean:
	rm ${EXECS} -f $(BUILD)*/*.o $(BUILD)*.o