#!/bin/sh
# This is run by init
HOME=/; export HOME
PATH=/bin:/usr/bin:/usr/etc:/usr/lib ; export PATH

if [ -f /etc/ifconfig ] ; then
	/etc/ifconfig lo0 addr 127.0.0.1 up
	route add 127.0.0.0 lo0
fi

if [ -f /etc/fastboot ] ; then
	echo "Fast boot.. Skipping disk checks."
	/bin/rm /etc/fastboot
else
	if [ -f /etc/fsck ] ; then
		/etc/fsck -q
	fi
fi
