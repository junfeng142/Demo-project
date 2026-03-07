#!/bin/sh

JAVA_CMD=jvm/bin/cvm

if [ ! $MIDPATH_HOME ]; then
  MIDPATH_HOME=$(pwd)
fi

DIR=$HOME/.jvm
if [ ! -d "$DIR" ]; then
  mkdir -p $DIR
  mkdir -p $DIR/midlets
  mkdir -p $DIR/rms
  cp -a conf $DIR
fi

if [ $# -lt 2 ]; then
 echo "Usage :"
 echo "  $(basename $0) <classpath> <midlet-class> [midlet-name]" 
 echo "  $(basename $0) -jar <jar-file>"
 exit 1
fi

# Set the classpath
CP=$MIDPATH_HOME/dist/midpath.jar:$DIR/conf:$MIDPATH_HOME/dist/kxml2-2.3.0.jar:$MIDPATH_HOME/dist/microbackend.jar:$MIDPATH_HOME/dist/jlayerme-cldc.jar:$MIDPATH_HOME/dist/jorbis-cldc.jar:$MIDPATH_HOME/dist/escher-cldc.jar:$MIDPATH_HOME/dist/sdljava-cldc.jar:$MIDPATH_HOME/dist/avetanabt-cldc.jar:$MIDPATH_HOME/dist/jsr172-jaxp.jar:$MIDPATH_HOME/dist/jsr172-jaxrpc.jar:$MIDPATH_HOME/dist/jsr179-location.jar:$MIDPATH_HOME/dist/jsr184-m3g.jar:$MIDPATH_HOME/dist/jsr205-messaging.jar:$MIDPATH_HOME/dist/jsr226-svg-core.jar:$MIDPATH_HOME/dist/jsr226-svg-midp2.jar:$MIDPATH_HOME/dist/jsr239-opengles-core.jar:$MIDPATH_HOME/dist/jsr239-opengles-jgl.jar:$MIDPATH_HOME/dist/jsr239-opengles-nio.jar:$MIDPATH_HOME/dist/jgl-cldc.jar:$MIDPATH_HOME/dist/nokia.jar:$MIDPATH_HOME/dist/jsr226-svg-awt.jar:$MIDPATH_HOME/dist/cldc1.1.jar


# Parse the arguments
if [ $1 = "-jar" ]; then
 CP=$CP:$2
 ARGS="$1 $2"
else
 CP=$CP:$1
 ARGS="$2 $3"
fi

# Path of the native libraries
JLP=$MIDPATH_HOME/dist

CLASS=org.thenesis.midpath.main.MIDletLauncherSE


PIDS=`ps -ef |grep "$JAVA_CMD" |grep -v grep | awk '{print $2}'`

if [ "$PIDS" != "" ]; then
    pkill -9 $PIDS >/dev/null 2>&1 &
#    $JAVA_CMD -Dsun.boot.library.path=${JLP} -Xbootclasspath/p:${CP} -Xmx10M ${CLASS} ${ARGS} > $DIR/j2me_std_log.txt 2>$DIR/j2me_err_log.txt
    $JAVA_CMD -Dsun.boot.library.path=${JLP} -Xbootclasspath/p:${CP} -Xmx10M ${CLASS} ${ARGS}
else
#    $JAVA_CMD -Dsun.boot.library.path=${JLP} -Xbootclasspath/p:${CP} -Xmx10M ${CLASS} ${ARGS} > $DIR/j2me_std_log.txt 2>$DIR/j2me_err_log.txt
    $JAVA_CMD -Dsun.boot.library.path=${JLP} -Xbootclasspath/p:${CP} -Xmx10M ${CLASS} ${ARGS}
fi
