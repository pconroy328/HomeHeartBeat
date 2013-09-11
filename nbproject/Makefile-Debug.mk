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
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/database.o \
	${OBJECTDIR}/device.o \
	${OBJECTDIR}/helpers.o \
	${OBJECTDIR}/hhb_system.o \
	${OBJECTDIR}/inifile.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/mqtt.o \
	${OBJECTDIR}/openclose_sensor.o \
	${OBJECTDIR}/serialport.o \
	${OBJECTDIR}/waterleak_sensor.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lmosquitto ../IniFiler/dist/Debug/GNU-Linux-x86/libinifiler.a -lmysqlclient

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/homeheartbeat_1

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/homeheartbeat_1: ../IniFiler/dist/Debug/GNU-Linux-x86/libinifiler.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/homeheartbeat_1: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/homeheartbeat_1 ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/database.o: database.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/database.o database.c

${OBJECTDIR}/device.o: device.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/device.o device.c

${OBJECTDIR}/helpers.o: helpers.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/helpers.o helpers.c

${OBJECTDIR}/hhb_system.o: hhb_system.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/hhb_system.o hhb_system.c

${OBJECTDIR}/inifile.o: inifile.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/inifile.o inifile.c

${OBJECTDIR}/main.o: main.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.c

${OBJECTDIR}/mqtt.o: mqtt.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/mqtt.o mqtt.c

${OBJECTDIR}/openclose_sensor.o: openclose_sensor.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/openclose_sensor.o openclose_sensor.c

${OBJECTDIR}/serialport.o: serialport.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/serialport.o serialport.c

${OBJECTDIR}/waterleak_sensor.o: waterleak_sensor.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/waterleak_sensor.o waterleak_sensor.c

# Subprojects
.build-subprojects:
	cd ../IniFiler && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/homeheartbeat_1

# Subprojects
.clean-subprojects:
	cd ../IniFiler && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
