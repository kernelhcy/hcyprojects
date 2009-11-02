#!/bin/bash

#
# Several definitions in advance.
#
PROGRAM='xbindkeys'
BLACKHOLE='/dev/null'
KILLALL=`which killall`
PORT='/tmp/linux1g1g'
RCFILE="${HOME}/.xbindkeysrc"

#
# Key mappings.
#
NEXT='  m:0xc + c:57            # Ctrl + Alt + n'
PLAY='  m:0xc + c:33            # Ctrl + Alt + p'
MUTE='  m:0xc + c:32            # Ctrl + Alt + o'
DOWN='  m:0xc + c:31            # Ctrl + Alt + i'
UP='  m:0xc + c:30            # Ctrl + Alt + u'

#
# Remind users to install xbindkeys package first.
#
if [ ! `which ${PROGRAM}` ]; then
	echo "In order to use the global key bindings for linux1g1g,"
	echo "please install \"${PROGRAM}\" and run \"bindkeys.sh\" manually."
	exit 1
fi

#
# Kill all the running xbindkeys processes.
#
${KILLALL} ${PROGRAM} > ${BLACKHOLE} 2>&1

#
# Modify the xbindkeys RC file.
#
if [ ! -f ${RCFILE} ]; then
	${PROGRAM} -d > ${RCFILE}
fi

echo '# Following configurations are used by linux1g1g' >> ${RCFILE}

echo "\"echo next > ${PORT}\"" >> ${RCFILE}
echo "${NEXT}" >> ${RCFILE}

echo "\"echo playPause > ${PORT}\"" >> ${RCFILE}
echo "${PLAY}" >> ${RCFILE}

echo "\"echo volumeOnOff > ${PORT}\"" >> ${RCFILE}
echo "${MUTE}" >> ${RCFILE}

echo "\"echo volumeUp > ${PORT}\"" >> ${RCFILE}
echo "${UP}" >> ${RCFILE}

echo "\"echo volumeDown > ${PORT}\"" >> ${RCFILE}
echo "${DOWN}" >> ${RCFILE}

#
# Re-run the xbindkeys process.
#
${PROGRAM}
echo 'Default shortcuts are :'
echo 'NEXT= Ctrl + Alt + n'
echo 'PLAY= Ctrl + Alt + p'
echo 'MUTE= Ctrl + Alt + o'
echo 'DOWN= Ctrl + Alt + i'
echo 'UP= Ctrl + Alt + u'
