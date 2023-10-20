#!/bin/bash
# 
# maozongwu@dync.cc
# /etc/rc.d/init.d/anyxxx
# init script for anyxxx processes
#
# processname: anyxxx
# description: anyxxx is a media session daemon.
# chkconfig: 2345 99 10
#
if [ -f /etc/init.d/functions ]; then
	. /etc/init.d/functions
elif [ -f /etc/rc.d/init.d/functions ]; then
	. /etc/rc.d/init.d/functions
else
	echo -e "\aJnice: unable to locate functions lib. Cannot continue."
	exit -1
fi

ulimit -n 65000
echo 1024 65000 > /proc/sys/net/ipv4/ip_local_port_range

#---------------------------------------------------------------------------
# GLOBAL
#---------------------------------------------------------------------------

CurPath=$(pwd)/out/Linux_common
Bin=X2RtcStream
RT_LIB=${CurPath}
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${RT_LIB}
retval=0

#---------------------------------------------------------------------------
# START
#---------------------------------------------------------------------------
Start()
{
	if [ $( pidof -s ${Bin} ) ]; then
		echo -n "${Bin} process [${prog}] already running"
		echo_failure
		echo
	fi
		
	echo -n "Starting Dync Server(${Bin}): "
	${CurPath}/${Bin} $pwd/X2Rtc-Stream/x2rtc.conf & 2> /dev/null
	retval=$?
	if [ ${retval} == 0 ]; then
		echo_success
		echo
	else
		echo_failure
		echo
		break
	fi
	sleep 1
	
	#cd -
	return 0
}

#---------------------------------------------------------------------------
# STOP
#---------------------------------------------------------------------------
Stop()
{
	echo -n "Stopping Dync Server(${Bin}): "
	killproc ${Bin}
	echo
	return 0
}

#---------------------------------------------------------------------------
# MAIN
#---------------------------------------------------------------------------
case "$1" in
	start)
		Start
		;;
	stop)
		Stop
		;;
	restart)
		Stop
		#sleep 5
		Start
		;;
	*)
		echo "Usage: $0 {start|stop|restart}"
esac

exit 0

