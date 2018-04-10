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
		, "-port_wwn", "-node_wwn"
		, "-protect" // T10_PI
		, "-TC_volstat", "-SI_M0_volstat", "-SI_M1_volstat", "-SI_M2_volstat"
		, "-UR_M0_volstat" , "-UR_M1_volstat", "-UR_M2_volstat", "-UR_M3_volstat", "-UR_M4_volstat", "-UR_M5_volstat", "-UR_M6_volstat", "-UR_M7_volstat"
		, "-SSID", "-CU", "-consistency_group"
		, "-HUVM_role"
		, "-physical_product"
        , "-physical_product_revision"
        , "-physical_product_ID"  // 4 character, e.g. R800
        , "-physical_submodel"
        , "-physical_serial"
        , "-physical_LDEV"
        , "-physical_LDEV_type"
        , "-physical_PG"
        , "-physical_RAID_level"
        , "-physical_Pool_ID"
        , "-physical_nickname"
        , "-physical_SSID"
        , "-physical_CU"
        , "-physical_consistency_group"
        , "-GAD_remote_serial"
        , "-GAD_remote_LDEV"
        , "-write_same"
        , "-page_size_sectors"
        , "-zero_reclaim_starting_sector"
        , "-pages_in_use"
        , "-pool_threshold"
        , "-available_threshold"
        , "-pool_usage"
        , "-pool_remaining_MiB"
        , "-total_pool_MiB"
        , "-optimal_write_same_granularity_sectors"
        , "-physical_pool_usage"
        , "-pool_saving_rate"
        , "-pool_physical_remaining_MiB"
        , "-pool_total_physical_MiB"
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
			else if (o==std::string("-port_wwn")) std::cout << p_LUN->get_port_wwn();
			else if (o==std::string("-node_wwn")) std::cout << p_LUN->get_node_wwn();
			else if (o==std::string("-protect")) std::cout << p_LUN->get_protect();
			else if (o==std::string("-TC_volstat")) std::cout << p_LUN->get_TC_volstat();
			else if (o==std::string("-SI_M0_volstat")) std::cout << p_LUN->get_SI_M0_volstat();
			else if (o==std::string("-SI_M1_volstat")) std::cout << p_LUN->get_SI_M1_volstat();
			else if (o==std::string("-SI_M2_volstat")) std::cout << p_LUN->get_SI_M2_volstat();
			else if (o==std::string("-UR_M0_volstat")) std::cout << p_LUN->get_UR_M0_volstat();
			else if (o==std::string("-UR_M1_volstat")) std::cout << p_LUN->get_UR_M1_volstat();
			else if (o==std::string("-UR_M2_volstat")) std::cout << p_LUN->get_UR_M2_volstat();
			else if (o==std::string("-UR_M3_volstat")) std::cout << p_LUN->get_UR_M3_volstat();
			else if (o==std::string("-SSID")) std::cout << p_LUN->get_SSID();
			else if (o==std::string("-CU")) std::cout << p_LUN->get_CU();
			else if (o==std::string("-consistency_group")) std::cout << p_LUN->get_consistency_group();
			else if (o==std::string("-HUVM_role")) std::cout << p_LUN->get_HUVM_role();
			else if (o==std::string("-physical_product")) std::cout << p_LUN->get_physical_product();
			else if (o==std::string("-physical_product_revision")) std::cout << p_LUN->get_physical_product_revision();
			else if (o==std::string("-physical_product_ID")) std::cout << p_LUN->get_physical_product_ID();
			else if (o==std::string("-physical_submodel")) std::cout << p_LUN->get_physical_submodel();
			else if (o==std::string("-physical_serial")) std::cout << p_LUN->get_physical_serial();
			else if (o==std::string("-physical_LDEV")) std::cout << p_LUN->get_physical_LDEV();
			else if (o==std::string("-physical_LDEV_type")) std::cout << p_LUN->get_physical_LDEV_type();
			else if (o==std::string("-physical_PG")) std::cout << p_LUN->get_physical_PG();
			else if (o==std::string("-physical_RAID_level")) std::cout << p_LUN->get_physical_RAID_level();
			else if (o==std::string("-physical_Pool_ID")) std::cout << p_LUN->get_physical_Pool_ID();
			else if (o==std::string("-physical_nickname")) std::cout << p_LUN->get_physical_nickname();
			else if (o==std::string("-physical_SSID")) std::cout << p_LUN->get_physical_SSID();
			else if (o==std::string("-physical_CU")) std::cout << p_LUN->get_physical_CU();
			else if (o==std::string("-physical_consistency_group")) std::cout << p_LUN->get_physical_consistency_group();
			else if (o==std::string("-GAD_remote_serial")) std::cout << p_LUN->get_GAD_remote_serial();
			else if (o==std::string("-GAD_remote_LDEV")) std::cout << p_LUN->get_GAD_remote_LDEV();
			else if (o==std::string("-write_same")) std::cout << p_LUN->get_write_same();
			else if (o==std::string("-page_size_sectors")) std::cout << p_LUN->get_page_size_sectors();
			else if (o==std::string("-zero_reclaim_starting_sector")) std::cout << p_LUN->get_zero_reclaim_starting_sector();
			else if (o==std::string("-pages_in_use")) std::cout << p_LUN->get_pages_in_use();
			else if (o==std::string("-pool_threshold")) std::cout << p_LUN->get_pool_threshold();
			else if (o==std::string("-available_threshold")) std::cout << p_LUN->get_available_threshold();
			else if (o==std::string("-pool_usage")) std::cout << p_LUN->get_pool_usage();
			else if (o==std::string("-pool_remaining_MiB")) std::cout << p_LUN->get_pool_remaining_MiB();
			else if (o==std::string("-total_pool_MiB")) std::cout << p_LUN->get_total_pool_MiB();
			else if (o==std::string("-optimal_write_same_granularity_sectors")) std::cout << p_LUN->get_optimal_write_same_granularity_sectors();
			else if (o==std::string("-physical_pool_usage")) std::cout << p_LUN->get_physical_pool_usage();
			else if (o==std::string("-pool_saving_rate")) std::cout << p_LUN->get_pool_saving_rate();
			else if (o==std::string("-pool_physical_remaining_MiB")) std::cout << p_LUN->get_pool_physical_remaining_MiB();
			else if (o==std::string("-pool_total_physical_MiB")) std::cout << p_LUN->get_pool_total_physical_MiB();
			else std::cout << "Internal logic error - option \"" << o << "\" not found" << std::endl;
		}
	}
	std::cout << std::endl;

}
