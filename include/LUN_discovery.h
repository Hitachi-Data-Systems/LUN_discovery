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


#pragma once

#include <scsi/sg.h>
#include <scsi/scsi.h>

#define DEF_BUF_SIZE 255
#define SI_CMD 0x12

#define VPD_BUF_SIZE 255

#define BYTES_PER_SECTOR 512

#include "Hitachi_RAID_SCSI_Inquiry_format.h"

class LUN_discovery {
private:
	std::string LUNname; // will be set by the constructor
	std::string hostname;
	std::string E0_product;
	int fd, k;
	// SCSI inquiry command format:  (6 bytes)
	// byte 0 - opcode 0x12
	// byte 1 - 0 or 1.  1 means enable Vital Product Data
	// byte 2 - page code (type) of the page you are asking for
	// byte 3 - 0 (reserved)
	// byte 4 - allocation length (Not to be confused with the "additional length" returned with SI command response
	//                             which is 4 bytes smaller than the total number of bytes returned by the SI command.)
	// byte 5 - two bits vendor unique (zero for Hitachi), three bits zeros (reserved), NACA bit, Flag bit, Link bit

	unsigned char def_buf[DEF_BUF_SIZE];
	unsigned char def_cmd[6] = {SI_CMD,    0,    0, 0, DEF_BUF_SIZE, 0};
	bool haveDefaultPage{false};
	std::string def_sense_bytes{""};

	//unsigned char def_buf_EVPD[DEF_BUF_SIZE];
	//unsigned char def_cmd_EVPD[6] = {SI_CMD,    0x01,    0, 0, DEF_BUF_SIZE, 0};
	//bool haveDefaultPage_EVPD{false};
	//std::string def_sense_bytes_EVPD{""};

	unsigned char vpd_buf[VPD_BUF_SIZE];
	unsigned char vpd_cmd[6] = {SI_CMD, 0x01, 0x83, 0,  VPD_BUF_SIZE, 0};
	std::string vpd_sense_bytes{""};
	bool haveVpdPage{false};

	//unsigned char vpd_buf_EVPD_off[VPD_BUF_SIZE];
	//unsigned char vpd_cmd_EVPD_off[6] = {SI_CMD, 0x00, 0x83, 0,  VPD_BUF_SIZE, 0};
	//std::string vpd_sense_bytes_EVPD_off{""};
	//bool haveVpdPage_EVPD_off{false};

	Hitachi_page_E0 E0_page;
	unsigned char E0_cmd[6] = {SI_CMD, 0x01, 0xE0, 0,  sizeof(Hitachi_page_E0), 0};
	std::string page_E0_sense_bytes{""};
	bool haveE0page{false};

	unsigned char E2_sub_0_page[255];
	unsigned char E2_sub_0_cmd[6] = {SI_CMD, 0x01, 0xE2, 0,  sizeof(E2_sub_0_page), 0};
	std::string page_E2_sub_0_sense_bytes;
	bool have_E2_sub_0_page{false};

	unsigned char E2_sub_1_page[255];
	unsigned char E2_sub_1_cmd[6] = {SI_CMD, 0x01, 0xE2, 1,  sizeof(E2_sub_1_page), 0};
	bool have_E2_sub_1_page{false};
	std::string page_E2_sub_1_sense_bytes;

	unsigned char E2_sub_2_page[255];
	unsigned char E2_sub_2_cmd[6] = {SI_CMD, 0x01, 0xE2, 2,  sizeof(E2_sub_2_page), 0};
	bool have_E2_sub_2_page{false};
	std::string page_E2_sub_2_sense_bytes;

	int bus_number;
	char PROBE_HOSTbuf[101];
	int vpd_ioctl_return_code;
	unsigned char sense_buffer[32];
	int sense_bytes_received{0};
	std::string sense_data_unprintable_as_dot_with_hex{""};
	sg_io_hdr_t scsi_header;
	std::string constructor_log;

	///* Request information about a specific SG device, used by
	//   SG_GET_SCSI_ID ioctl ().  */
	//struct sg_scsi_id {
	//  /* Host number as in "scsi<n>" where 'n' is one of 0, 1, 2 etc.  */
	//  int host_no;
	//  int channel;
	//  /* SCSI id of target device.  */
	//  int scsi_id;
	//  int lun;
	//  /* TYPE_... defined in <scsi/scsi.h>.  */
	//  int scsi_type;
	//  /* Host (adapter) maximum commands per lun.  */
	//  short int h_cmd_per_lun;
	//  /* Device (or adapter) maximum queue length.  */
	//  short int d_queue_depth;
	//  /* Unused, set to 0 for now.  */
	//  int unused[2];
	//};
	struct sg_scsi_id my_sg_scsi_id;
	std::string SG_GET_SCSI_ID_string;

	struct {
		uint32_t dev_id;
		uint32_t host_unique_id;
	} scsi_id;
	int LU_Number;
	std::string scsi_id_string{""};
	std::string HitachiProduct, HDSProduct, SerialNumber, Port, LDEV, LDEV_no_colon;
	std::string VendorUnprintableAsDot , ProductUnprintableAsDot;
	std::string ProductRevisionUnprintableAsDot , VendorSpecificUnprintableAsDotWithHex;
	std::string HitachiVPD;
	std::string LDEV_type="", PG="", PG_no_hyphen="", CLPR="", poolID="", RAIDlevel="";
	std::string nickname{""};

	int64_t maxLBA{-1};
	int64_t SizeBytes{-1};

	bool openROgood{false};
	bool openRWgood{false};


	bool isHitachi{false};

	long double SizeMB{-1.}, SizeMiB{-1.}, SizeGB{-1.}, SizeGiB{-1.}, SizeTB{-1.}, SizeTiB{-1.};
	bool get_page(unsigned char* data_buffer, int data_buffer_size, unsigned char* SI_cmd, std::string description, std::string* sense_bytes);
	std::string showRAIDspecific();

public:
	int getSectorSize() { return 512; } // COME BACK AND UPDATE LATER  to recognize 4K sector size LUNs when we have them.
	LUN_discovery(std::string);
	inline int getLU_Number() {return LU_Number;}
	inline std::string getLUNname() {return LUNname;}
	inline std::string getHostname() {return hostname;}
	inline std::string getHDSProduct() {return HDSProduct;}
	inline std::string getHitachiProduct() {return HitachiProduct;}
	inline std::string getSerialNumber() {return SerialNumber;}
	inline std::string getPort() {return Port;}
	inline std::string getLDEV() {return LDEV;}
	inline std::string getLDEV_no_colon() {return LDEV_no_colon;}
	inline std::string getlog() {return constructor_log;}
	inline std::string getHitachiVPD() {return HitachiVPD;}
	inline std::string getSCSIID() { return SG_GET_SCSI_ID_string;}
	inline std::string get4in1id() { return scsi_id_string;}
	inline std::string getProbeHost() { return std::string(PROBE_HOSTbuf);}
	inline int getBusNumber() { return bus_number;} // usually HBA number, but several HBAs may be on a PCI bus
	inline int64_t getMaxLBA() { return maxLBA;}
	inline int64_t getSizeBytes() { return BYTES_PER_SECTOR*(1+maxLBA);}
	inline long double getSizeMB() { return SizeMB; }
	inline long double getSizeMiB() { return SizeMiB; }
	inline long double getSizeGB() { return SizeGB; }
	inline long double getSizeGiB() { return SizeGiB; }
	inline long double getSizeTB() { return SizeTB; }
	inline long double getSizeTiB() { return SizeTiB; }
	inline std::string getLDEV_type() { return LDEV_type; }
	inline std::string getPG() { return PG; }
	inline std::string getPG_no_hyphen() { return PG_no_hyphen; }
	inline std::string getCLPR() { return CLPR; }
	inline std::string getpoolID() { return poolID; }
	inline std::string getRAIDlevel() { return RAIDlevel; }
	inline std::string getnickname() { return nickname; }

	static std::string printheaders();
	std::string printdata();
	static void printheaders(std::ostream&);
	void printdata(std::ostream&);
	void showall(std::ostream&);
	inline bool IsHitachiSubsystemLUN() {return isHitachi;}
};

