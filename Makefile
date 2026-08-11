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

VERSION_H = ${SRC_DIR}/Version.h
DESC_FILE = DESCRIPTION

all: clean directory ${PROJECT}

directory:
	${MKDIR} ${BUILD_DIR}
	${MKDIR} ${OBJ_DIR}

# Generate Version.h from the latest release tag (vYY.WW.BB). Untagged dev
# builds fall back to the current year/ISO-week with patch 00. The app
# description comes from the DESCRIPTION file. Regenerated on every build so
# it tracks new tags.
version: ${VERSION_H}

${VERSION_H}: FORCE
	@ver=`git describe --tags --match 'v[0-9]*.[0-9]*.[0-9]*' --abbrev=0 2>/dev/null | sed 's/^v//'`; \
	if [ -z "$$ver" ]; then ver=`date -u +%g.%V.00`; fi; \
	desc=`head -n 1 ${DESC_FILE} 2>/dev/null`; \
	if [ -z "$$desc" ]; then desc="BoaScript"; fi; \
	{ echo "#ifndef _BOASCRIPT_VERSION_H_"; \
	  echo "#define _BOASCRIPT_VERSION_H_"; \
	  echo "#define BOASCRIPT_VERSION \"$$ver\""; \
	  echo "#define BOASCRIPT_DESCRIPTION \"$$desc\""; \
	  echo "#endif"; } > $@

${PROJECT}: ${OBJS}
	${AR} ${BUILD_DIR}/lib${PROJECT}.a ${OBJS}
	${CP} ${SRC_DIR}/${PROJECT}.tab.h ${BUILD_DIR}
	${CP} ${VERSION_H} ${BUILD_DIR}

clean:
	${RM} ${BUILD_DIR} ${OBJ_DIR} $(SRC_DIR)/${PROJECT}.tab.* ${VERSION_H}

test: ${PROJECT}
	@$(MAKE) -C ${TEST_DIR} check

FORCE:

.PHONY: all directory version clean test FORCE

###

$(OBJ_DIR)/${PROJECT}.tab.o: ${SRC_DIR}/${PROJECT}.y ${VERSION_H}
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
