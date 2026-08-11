TASK          = testboascript
DRIVER        = driver

CC            = g++
LINKER	      = g++
RM            = rm -rf

CCFLAGS	      = -O3 -DNDEBUG -DCALC_BATCH
#CCFLAGS        = -g -DVARSTR_DEBUG


SRC_DIR       = .
DIST_DIR      = ../distribution
EXT_DIR       = ../extras

LDFLAGS	      =

LIBS	      = -lBoascript

LIBDIR        = -L${DIST_DIR}

INCLUDES      = -I${DIST_DIR} \
                -I${EXT_DIR}


all: ${TASK} ${DRIVER}

${TASK}: $(TASK).o
	@echo -n "Linking $(TASK) ... "
	$(LINKER) $(LDFLAGS) -o $(TASK) $(TASK).o ${LIBDIR} ${LIBS}
	@echo "... well done"

${DRIVER}: $(DRIVER).o
	@echo -n "Linking $(DRIVER) ... "
	$(LINKER) $(LDFLAGS) -o $(DRIVER) $(DRIVER).o ${LIBDIR} ${LIBS}
	@echo "... well done"

%.o: %.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

# Run both test suites: the embedded assertion table in testboascript
# and the golden-file cases driven by run_tests.sh.
check: all
	@echo "== embedded assertion suite =="
	@./$(TASK) > /dev/null && echo "  testboascript: PASS" \
	    || { echo "  testboascript: FAIL"; exit 1; }
	@echo "== golden-file suite =="
	@bash run_tests.sh

test: check

clean:
	@echo "Cleaning..."
	$(RM) *.o $(TASK) $(DRIVER) build
	@echo "... done"

.PHONY: all check test clean
