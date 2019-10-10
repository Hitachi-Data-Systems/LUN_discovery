#!/bin/bash

for D in $(find /dev -mindepth 1 -maxdepth 1 | grep -E \/dev\/sd[[:alpha:]]+\$) ; do
	lun2string -noseparator \"sd=ldev\" -ldev_no_colon \"_port\" -port %22%2Chost%3D%22 -hostname %22%2Clun%3D%22 -name %22%2Copenflags%3Do_direct%22 -space \"#\" -subsystemtype -serialnumber -port -pg -ldev $D
done | sort 
