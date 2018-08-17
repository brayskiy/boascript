TASK = testboascript

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

OBJS          =  $(TASK).o    \

SRCS	      = $(TASK).cpp  \


all: ${TASK}

${TASK}: $(OBJS)
	@echo -n "Linking $(TASK) ... "
	$(LINKER) $(LDFLAGS) -o $(TASK) $(OBJS) ${LIBDIR} ${LIBS}
	@echo "... well done"

${TASK}.o: ${TASK}.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "Cleaning..."
	$(RM) *.o $(TASK)
	@echo "... done"
