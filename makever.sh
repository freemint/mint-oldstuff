#! /bin/sh

if [ ! -f .version ]
then
  echo 0 > .version
fi

if [ -d /sys/c ]
then
  typeset -i cycle
  cycle=`/c/type .version`
  let cycle=$cycle+1
else
  cycle=`cat .version`
  cycle=`expr $cycle + 1`
fi

if [ $cycle -gt 99 ]
then
	cycle=0
fi
echo $cycle > .version
