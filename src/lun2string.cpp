//Copyright (c) 2016 Hitachi Data Systems, Inc.
//All Rights Reserved.
//
//   Licensed under the Apache License, Version 2.0 (the "License"); you may
//   not use this file except in compliance with the License. You may obtain
//   a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
//   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
//   License for the specific language governing permissions and limitations
//   under the License.
//
//Author: Allart Ian Vogelesang <ian.vogelesang@hds.com>
//
//Support:  "ivy" is not officially supported by Hitachi Data Systems.
//          Contact me (Ian) by email at ian.vogelesang@hds.com and as time permits, I'll help on a best efforts basis.

#define _LARGE_FILE_API
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <queue>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <type_traits>


using namespace std;

#include "printableAndHex.h"
#include "LUN_discovery.h"

std::string urlDecode(std::string &SRC) {
    string ret;
    char ch;
    unsigned int i, ii;
    for (i=0; i<SRC.length(); i++) {
        if (int(SRC[i])==37) {
            sscanf(SRC.substr(i+1,2).c_str(), "%x", &ii);
            ch=static_cast<char>(ii);
            ret+=ch;
            i=i+2;
        } else {
            ret+=SRC[i];
        }
    }
    return (ret);
}

int main(int argc, char * argv[]) {

	std::string LUNname;

	std::string options[] {
		"-comma" // use -comma as the first option to set the separator to comma
		,"-tab" // use -tab as the first option to set the separator to comma
		,"-space" // sets the separator back to a space
		,"-noseparator" // use -comma as the first option to set the separator to comma
		,"-hostname","-name", "-subsystemtype", "-hitachitype", "-serialnumber", "-ldev"
		,"-ldev_type", "-pg", "-pg_no_hyphen", "-clpr", "-poolID", "-RAIDlevel", "-nickname"
		,"-ldev_no_colon", "-port", "-lun", "-scsi_id"
		, "-maxLBA" , "-sizeBytes", "-sizeMB", "-sizeMiB"
		, "-sizeGB", "-sizeGiB", "-sizeTB", "-sizeTiB"
		, "-hba", "-probehost"

	};
	int number_of_options = extent<decltype(options)>::value; // new in -std=c++11
	std::queue<std::string> selected_options;

	bool badargs = false;

	int lastarg=argc-1;
	int LUNarg = 1;
	if (argc<2) {
		badargs=true;
	} else {
		bool goodarg;
		std::string argument;
		std::string URLdecoded;
		while (LUNarg < lastarg) {
			goodarg=false;
			argument=std::string(argv[LUNarg]);
			URLdecoded = urlDecode(argument);
			if ( (URLdecoded.length() > 2) && (URLdecoded.at(0)=='\"') && (URLdecoded.at(URLdecoded.length()-1)=='\"')) {
				goodarg=true;
				selected_options.push(URLdecoded);
			} else {

				// see if this is a valid option
				for (int i=0; i<number_of_options; i++) {
						if (argument==options[i]) {
						goodarg=true;
						selected_options.push(options[i]);
						break;
					}
				}
			}
			if (!goodarg) {badargs=true;break;}
			LUNarg++;
		}
	}

    if (badargs) {
        std::cout << std::endl << "Usage: " << argv[0] << " [any sequence of one or more of the options below] <device_name>" << std::endl <<std::endl;
	std::cout << "Options:" << std::endl;
	for (int i=0; i<number_of_options; i++) {
		if ((i>=5) && (i%5==0)) std::cout << std::endl;
		std::cout << " " << options[i];
	}
	std::cout << std::endl << std::endl << "-comma changes the separator between fields to comma." << std::endl;
	std::cout << "-tab changes the separator between fields to tab." << std::endl;
	std::cout << "-space changes it back to a space [default]." << std::endl;
	std::cout << "-noseparator sets the separator to the empty string." << std::endl << std::endl;

	std::cout << "A command line token that comes in starting and ending with double quotes" << std::endl
	<< "will have its contents between the double quotes echoed to output." << std::endl
	<< "This lets you build strings with LUN attributes inserted." << std::endl
	<< "In bash, you can back-quote a double quote this way \\\"" << std::endl
	<< "To make it easier to build strings with spaces, etc.  you can URLencode" << std::endl
	<< "the double-quoted token to make it breeze past the bash command line tokenizer." << std::endl << std::endl;

	std::cout << "For example, \"" << std::string(argv[0]) << " -noseparator %22port%3D%22 -port /dev/sdd\" could print \"port=1A\"." << std::endl << std::endl;

	std::cout << "Let\'s try a more ambitious vbench sd=,lun= statement. The following line" << std::endl <<std::endl;

	std::cout << argv[0] << " -noseparator \\\"sd=sd\\\" -ldev_no_colon \\\"_\\\" -port %22%2C%20lun%3D%22 -name -space \\\"#\\\" -subsystemtype -serialnumber -port -ldev /dev/sdl" << std::endl << std::endl;
	std::cout << "when used on 3 different luns generated the following output:" << std::endl

		<< "sd=sd0002_2A, lun=/dev/sdd # AMS2100 83011441 2A 0002" << std::endl
		<< "sd=sdFFFF_6D, lun=/dev/sdk # HUS VM 210056 6D FF:FF" << std::endl
		<< "sd=sd0009_1A, lun=/dev/sdl # VSP 65779 1A 00:09" <<std::endl << std::endl;
        return 1;
    }

	LUNname = std::string(argv[LUNarg]);
	LUN_discovery* p_LUN = new LUN_discovery(LUNname);

	std::string separator{" "};
	std::string o;
	bool first_option=true;
	while (selected_options.size()>0) {
		if (!first_option) std::cout << separator;
		o=selected_options.front(); selected_options.pop();
		if (o==std::string("-comma")) separator = ",";
		else if (o==std::string("-tab")) separator=std::string("\t");
		else if (o==std::string("-noseparator")) separator=std::string("");
		else if (o==std::string("-space")) separator=std::string(" ");
		else {	first_option=false;
			if ( (o.length() > 2) && (o.at(0)=='\"') && (o.at(o.length()-1)=='\"')) std::cout<<(o.erase(o.length()-1,1)).erase(0,1);
			else if (o==std::string("-subsystemtype")) std::cout << p_LUN->getHDSProduct();
			else if (o==std::string("-hitachitype")) std::cout << p_LUN->getHitachiProduct();
			else if (o==std::string("-serialnumber")) std::cout << p_LUN->getSerialNumber();
			else if (o==std::string("-port")) std::cout << p_LUN->getPort();
			else if (o==std::string("-lun")) std::cout << p_LUN->getLU_Number();
			else if (o==std::string("-scsi_id")) std::cout << p_LUN->get4in1id();
			else if (o==std::string("-ldev")) std::cout << p_LUN->getLDEV();
			else if (o==std::string("-ldev_type")) std::cout << p_LUN->getLDEV_type();
			else if (o==std::string("-pg")) std::cout << p_LUN->getPG();
			else if (o==std::string("-pg_no_hyphen")) std::cout << p_LUN->getPG_no_hyphen();
			else if (o==std::string("-clpr")) std::cout << p_LUN->getCLPR();
			else if (o==std::string("-poolID")) std::cout << p_LUN->getpoolID();
			else if (o==std::string("-RAIDlevel")) std::cout << p_LUN->getRAIDlevel();
			else if (o==std::string("-nickname")) std::cout << p_LUN->getnickname();
			else if (o==std::string("-ldev_no_colon")) std::cout << p_LUN->getLDEV_no_colon();
			else if (o==std::string("-maxLBA")) std::cout << p_LUN->getMaxLBA();
			else if (o==std::string("-sizeBytes")) std::cout << p_LUN->getSizeBytes();
			else if (o==std::string("-sizeMB")) std::cout << p_LUN->getSizeMB();
			else if (o==std::string("-sizeMiB")) std::cout << p_LUN->getSizeMiB();
			else if (o==std::string("-sizeGB")) std::cout << p_LUN->getSizeGB();
			else if (o==std::string("-sizeGiB")) std::cout << p_LUN->getSizeGiB();
			else if (o==std::string("-sizeTB")) std::cout << p_LUN->getSizeTB();
			else if (o==std::string("-sizeTiB")) std::cout << p_LUN->getSizeTiB();
			else if (o==std::string("-name")) std::cout << p_LUN->getLUNname();
			else if (o==std::string("-hostname")) std::cout << p_LUN->getHostname();
			else if (o==std::string("-hitachivpd")) std::cout << p_LUN->getHitachiVPD();
			else if (o==std::string("-hba")) std::cout << p_LUN->getBusNumber();
			else if (o==std::string("-probehost")) std::cout << p_LUN->getProbeHost();
			else std::cout << "Internal logic error - option \"" << o << "\" not found" << std::endl;
		}
	}
	std::cout << std::endl;

}
