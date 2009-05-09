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
	${OBJECTDIR}/Main_Window.o \
	${OBJECTDIR}/db_connection.o \
	${OBJECTDIR}/main.o

# C Compiler Flags
CFLAGS=`pkg-config --cflags --libs gtk+-2.0` -lmysqlclient -lmysqlpp -I/usr/include/mysql -I/usr/include/mysql++ -DBIG_JOINS=1 -fPIC -Wl,-Bsymbolic-functions 

# CC Compiler Flags
CCFLAGS=`pkg-config --cflags --libs gtk+-2.0` -lmysqlclient -lmysqlpp -I/usr/include/mysql -I/usr/include/mysql++ -DBIG_JOINS=1 -fPIC -Wl,-Bsymbolic-functions 
CXXFLAGS=`pkg-config --cflags --libs gtk+-2.0` -lmysqlclient -lmysqlpp -I/usr/include/mysql -I/usr/include/mysql++ -DBIG_JOINS=1 -fPIC -Wl,-Bsymbolic-functions 

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/include/gtk-2.0

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/GNU-Linux-x86/gtk_test

dist/Debug/GNU-Linux-x86/gtk_test: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/GNU-Linux-x86
	${LINK.cc} -o dist/Debug/GNU-Linux-x86/gtk_test ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/Main_Window.o: Main_Window.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/gtk-2.0 -I/usr/include/mysql++ -MMD -MP -MF $@.d -o ${OBJECTDIR}/Main_Window.o Main_Window.cpp

${OBJECTDIR}/db_connection.o: db_connection.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/gtk-2.0 -I/usr/include/mysql++ -MMD -MP -MF $@.d -o ${OBJECTDIR}/db_connection.o db_connection.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/gtk-2.0 -I/usr/include/mysql++ -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Debug
	${RM} dist/Debug/GNU-Linux-x86/gtk_test

# Subprojects
.clean-subprojects:

# Enable dependency checking
include .dep.inc
