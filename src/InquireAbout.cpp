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
//Author: Allart Ian Vogelesang <ian.vogelesang@hitachivantara.com>
//
//Support:  "ivy" is not officially supported by Hitachi Data Systems.
//          Contact me (Ian) by email at ian.vogelesang@hitachivantara.com and as time permits, I'll help on a best efforts basis.

#define _LARGE_FILE_API
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/ioctl.h>
#include <scsi/sg.h>

using namespace std;

#include "printableAndHex.h"
#include "LUN_discovery.h"

int main(int argc, char * argv[]) {

	bool showall=false;
	std::string LUNname;

	if ( (3==argc) && ( std::string("-showall") == std::string(argv[1]) )) {showall=true;}
	else showall=false;

	if ( (!(2 == argc)) && (! ((argc==3) && showall) )) {
        	std::cout << "Usage: " << argv[0] << " [-showall] <device_name>" << std::endl;
        	return 1;
	}
	if (2==argc) LUNname=std::string(argv[1]);
	else LUNname=std::string(argv[2]);
	LUN_discovery* p_LUN = new LUN_discovery(LUNname);

	if (showall) p_LUN->showall(std::cout);
	else std::cout << p_LUN->printdata() << std::endl;

}
