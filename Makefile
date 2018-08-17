PROJECT = Boascript

OOYACC = ${HOME}/bin/ooyacc

SH    = bash -f
RM    = rm -rf
MV    = mv
CP    = cp
CD    = cd
CC    = g++
AR    = ar rcs
MKDIR = mkdir -p

TEST_DIR  = test
SRC_DIR   = src
OBJ_DIR   = obj
BUILD_DIR = distribution
EXTRAS    = extras

SRCS     = ${SRC_DIR}/${PROJECT}.tab.cpp \
           ${EXTRAS}/DateTime.cpp       \

OBJS     = ${OBJ_DIR}/${PROJECT}.tab.o \
           ${OBJ_DIR}/DateTime.o      \

INCLUDES = -I ${EXTRAS}
 
all: clean directory ${PROJECT} 

directory:
	${MKDIR} ${BUILD_DIR}
	${MKDIR} ${OBJ_DIR}

${PROJECT}: ${OBJS}
	${AR} ${BUILD_DIR}/lib${PROJECT}.a ${OBJS}
	${CP} ${SRC_DIR}/${PROJECT}.tab.h ${BUILD_DIR}
	
clean: 
	${RM} ${BUILD_DIR} ${OBJ_DIR} $(SRC_DIR)/${PROJECT}.tab.*

test: ${PROJECT} 
	@for i in ${TEST_DIR}; do   \
	echo "Installing in $$i..."; \
	(cd $$i; $(MAKE) install); done

###

$(OBJ_DIR)/${PROJECT}.tab.o: ${SRC_DIR}/${PROJECT}.y
	${OOYACC} -b ${PROJECT} -d ${SRC_DIR}/${PROJECT}.y 
	${MV} ${PROJECT}.tab.* ${SRC_DIR}
	$(CC) $(CCFLAGS) $(INCLUDES) -c $(SRC_DIR)/${PROJECT}.tab.cpp -o $@

${OBJ_DIR}/DateTime.o: ${EXTRAS}/DateTime.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@


#${TASK}: $(OBJS)
#	@echo -n "Linking $(TASK) ... "
#	$(LINKER) $(LDFLAGS) -o $(INSTALL_DIR)/$(TASK) $(OBJS) $(LIBS)
#	@echo "... well done"

#clean:
#	@echo "Cleaning..."
#	$(RM) -f $(OBJ_DIR)/*.o $(TASK).exe $(SRC_DIR)/BoaScript.tab.*
#	@echo "... done"

#install:	$(PROGRAM)
#		@echo Installing $(PROGRAM) in $(DEST)
#		@install -s $(PROGRAM) $(DEST)
