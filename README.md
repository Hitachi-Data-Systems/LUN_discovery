# LUN_discovery

LUN discovery is a convenient Hitachi command line tool set for Linux to discover attributes of Hitachi storage LUNs.

### `showluns.sh`

Script to print a csv file like 

```
hostname,LUN name,HDS product,port,LDEV,parity group,CLPR
blade32,/dev/sdx,VSP G1000,1A,00:10,1-1,CLPR0
blade32,/dev/sdy,VSP G1000,2A,00:11,1-2,CLPR0
```

The header line defines the attribute names, and for each storage LUN, a detail line shows the attribute values.

### `InquireAboutHeaders`

Executable to print the header line.

### `InquireAbout`

Executable to issue SCSI Inquiry commands to a LUN, and to decode and print the attributes.

### `lun2string`

Executable to issue SCSI Inquiry commands to a LUN, and to build a custom string specified by command line parameters, incorporating decoded LUN attributes.

## binary executables
LUN_discovery is built on Linux using the Codeblocks IDE.  The binaries in the bin folder were built on a RHEL 6 system with g++ 4.92 and associated libstdc++.   They are statically linked with libstdc++ and dynamically linked with libc.

## License

[Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0)


## Contributors

ian.vogelesang@hds.com


## build with

[codeblocks](http://codeblocks.org)
