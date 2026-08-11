TASK          = testboascript
DRIVER        = driver
UNIT          = unittests

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


all: ${TASK} ${DRIVER} ${UNIT}

${TASK}: $(TASK).o
	@echo -n "Linking $(TASK) ... "
	$(LINKER) $(LDFLAGS) -o $(TASK) $(TASK).o ${LIBDIR} ${LIBS}
	@echo "... well done"

${DRIVER}: $(DRIVER).o
	@echo -n "Linking $(DRIVER) ... "
	$(LINKER) $(LDFLAGS) -o $(DRIVER) $(DRIVER).o ${LIBDIR} ${LIBS}
	@echo "... well done"

${UNIT}: $(UNIT).o
	@echo -n "Linking $(UNIT) ... "
	$(LINKER) $(LDFLAGS) -o $(UNIT) $(UNIT).o ${LIBDIR} ${LIBS}
	@echo "... well done"

%.o: %.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

# Run all three test suites: the embedded assertion table (testboascript),
# the focused unit tests (unittests), and the golden-file cases (run_tests.sh).
check: all
	@echo "== embedded assertion suite =="
	@./$(TASK) > /dev/null && echo "  testboascript: PASS" \
	    || { echo "  testboascript: FAIL"; exit 1; }
	@echo "== unit tests =="
	@./$(UNIT)
	@echo "== golden-file suite =="
	@bash run_tests.sh

test: check

# Measure line and branch coverage of the interpreter (needs gcov). The
# generated parser must already exist (build it with 'make all' from the
# repo root).
coverage:
	@bash coverage.sh

clean:
	@echo "Cleaning..."
	$(RM) *.o $(TASK) $(DRIVER) $(UNIT) build coverage
	@echo "... done"

.PHONY: all check test coverage clean
