#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/Debug/GNU-Linux-x86

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/main.o

# C Compiler Flags
CFLAGS=-lmysqlclient -I/usr/include/mysql -DBIG_JOINS=1 -fPIC -Wl,-Bsymbolic-functions

# CC Compiler Flags
CCFLAGS=-lmysqlclient -lmysqlpp -I/usr/include/mysql -I/usr/include/mysql++ -DBIG_JOINS=1 -fPIC -Wl,-Bsymbolic-functions
CXXFLAGS=-lmysqlclient -lmysqlpp -I/usr/include/mysql -I/usr/include/mysql++ -DBIG_JOINS=1 -fPIC -Wl,-Bsymbolic-functions

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/GNU-Linux-x86/db_test

dist/Debug/GNU-Linux-x86/db_test: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/GNU-Linux-x86
	${LINK.cc} -o dist/Debug/GNU-Linux-x86/db_test ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/mysql++ -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Debug
	${RM} dist/Debug/GNU-Linux-x86/db_test

# Subprojects
.clean-subprojects:

# Enable dependency checking
include .dep.inc
