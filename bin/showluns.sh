#!/bin/bash

InquireAboutHeaders

for D in $(find /dev -mindepth 1 -maxdepth 1 | grep "/dev/sd[[:alpha:]]\+$") ; do
    InquireAbout $D 2>/dev/null;
done | grep -v "<no data>" | sort -t, -k9 -k7
