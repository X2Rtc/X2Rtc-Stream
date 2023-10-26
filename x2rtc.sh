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
	${CurPath}/${Bin} $(pwd)/X2Rtc-Stream/x2rtc.conf & 2> /dev/null
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
	# Trying to get an external address
	external_ip=$(curl -s ifconfig.me)
	# Trying to get an internal address
	internal_ip=$(hostname -I | awk '{print $1}')
	if [ ${retval} == 0 ];then
           if [ -n "$external_ip" ]; then
               printf "********************************************************************************* \r\n"
	       printf "warn: External address please map the port or open the firewall... \r\n"
	       printf "********************************************************************************* \r\n"
               printf "Web platform External address：https://$external_ip:8080 \r\n"
               printf "Web platform Internal address：https://$internal_ip:8080 \r\n"
               printf "WebRTC External push-pull address：webrtc://$external_ip:10011/live/xxx \r\n" 
               printf "WebRTC Internal push-pull address：webrtc://$internal_ip:10011/live/xxx \r\n"
               printf "*********************************************************************************\r\n"
           else
               printf "*********************************************************************************\r\n"
               printf "* Failed to retrieve External IP. Trying internal IP... \r\n"
               printf "* Web platform Internal address：https://$internal_ip:8080 \r\n"
               printf "* WebRTC Intranet push-pull address：webrtc://$internal_ip:10011/live/xxx \r\n"
               printf "*********************************************************************************\r\n"
           fi
	fi
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

