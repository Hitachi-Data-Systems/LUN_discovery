#!/bin/bash

export PATH=$PATH:`dirname "$0"`

{
	InquireAboutHeaders

	for D in $(find /dev -mindepth 1 -maxdepth 1 | grep "/dev/sd[[:alpha:]]\+$") ; do
	    InquireAbout $D 2>/dev/null;
	done | sort -t, -k9 -k7
} | remove_empty_columns

# remove_empty_columns is part of "ivy" rather than part of LUN_discovery
