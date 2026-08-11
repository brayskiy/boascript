TASK          = boa

CC            = g++
LINKER        = g++
RM            = rm -rf

CCFLAGS       = -O2 -DNDEBUG -DCALC_BATCH

DIST_DIR      = ../../distribution
EXT_DIR       = ../../extras

LDFLAGS       =
LIBS          = -lBoascript
LIBDIR        = -L${DIST_DIR}
INCLUDES      = -I${DIST_DIR} -I${EXT_DIR}


all: ${TASK}

${TASK}: ${TASK}.o
	@echo -n "Linking $(TASK) ... "
	$(LINKER) $(LDFLAGS) -o $(TASK) $(TASK).o ${LIBDIR} ${LIBS}
	@echo "... well done"

${TASK}.o: boa.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c boa.cpp -o $(TASK).o

run: ${TASK}
	./${TASK}

clean:
	@echo "Cleaning..."
	$(RM) *.o ${TASK}
	@echo "... done"

.PHONY: all run clean
