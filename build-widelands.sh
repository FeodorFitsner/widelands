#!/bin/sh

if [ ! -e utils/scons.py -o -z "`utils/scons.py -v|grep 0.97`" ] ; then
	cd utils
	tar xzf scons-local-0.97.tar.gz
	cd -
fi

for i in $* ; do
	if [ "$i" = "--help" ] ; then
		echo syntax: $0 [OPTIONS]
		echo
		echo OPTIONS List - Use only absolute paths:
		python utils/scons.py -Q -h
		exit 0
	fi
done

python utils/scons.py $*
