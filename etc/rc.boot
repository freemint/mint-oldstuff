#!/bin/sh
# This is run by init

if [ -f /etc/fastboot ] ; then
	echo "Fast boot.. Skipping disk checks."
	/bin/rm /etc/fastboot
else
	if [ -f /etc/fsck ] ; then
		/etc/fsck -q
	fi
fi
