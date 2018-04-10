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

typedef struct Hitachi_RAID_SI_format {
	// byte 0
	unsigned int peripheral_qualifier   : 3;
	unsigned int peripheral_device_type : 5;

	// byte 1
	unsigned int RMB : 1; // value should be zero
	unsigned int     : 7; // reserved, value should be zero

	// byte 2
	char version; // The condition which has smaller number is prior to others
	              // (1) HMO68-on: 05h
	              // (2) HM=0x0f or 0x00 and HMO00=OFF: 03h
	              // (3) HMO18=ON: 04h
	              // (4) HMP63=ON: 04h

	// byte 3
	unsigned int : 2; // reserved, value should be zero
	unsigned int NACA : 1; // HM=0x0f: 1b otherwise 0b
	unsigned int HISUP : 1; // value should be 1 but says 0 in another place
	unsigned int response_data_format : 4; // value should be 2, but says 0 in another place

	// byte 4
	unsigned char additional_length; // HM=0x09 and HMO11=ON: 0x8F, otherwise 0xEF = 239

	// byte 5
	unsigned int SCCS : 1; // value should be 0
	unsigned int ACC : 1; // value should be 0
	unsigned int TPGS : 2; // value should be 0 meaning ALUA unavailable
	                       // 1 means ALUA available - but should not happen for RAID
	unsigned int ThreePC : 1; // HMO63=ON: 1b, else 0b
	unsigned int : 2; // reserved, value should be 0
	unsigned int protect : 1; // value should be 0

	// byte 6
	unsigned int BQUE : 1; // value should be 0
	unsigned int ENCSERV : 1; // value should be 0
	unsigned int VS : 1; // value shoudl be 0
	unsigned int MULTIP : 1; // value should be 0
	unsigned int MCHNGR : 1; // value should be 0
	unsigned int : 3; // reserved, value should be 0

	// byte 7
	unsigned int : 4; // reserved, value should be 0
	unsigned int linked : 1;
	unsigned int : 1; // reserved, value should be 0
	unsigned int CmdQue : 1; // value should be 1
	unsigned int VS_byte7 : 1; // value should be 0

	// bytes 8-15 (8 bytes)
	char VendorID[8];  // should be "HITACHI "

	// bytes 16-31 (16 bytes)
	char ProductID[16]; // e.g. "OPEN-V          "
		// If LDEV is not found (whatever that means!) "DISK-SUBSYSTEM"
		// Otherwise see product specification e.g. "OPEN-V"

	// bytes 32-35 (4 bytes)
	char ProductRevisionLevel[4]; // ASCII
		// HM=0x03 or 0x08 or 0x0c: "5001", else micro version e.g. "8001"
		// "5001" RAID 500
		// "6001" RAID 600
		// "7001" RAID 700
		// "8001" RAID 800
		// "7301" HM700
		// "8301" HM800
		// "8801" HM850

	// bytes 36-47 (12 bytes) LDEV identifier
	char fifty[2]; // "50"
	char three; // "3"
	char SN_each_hex_digit_as_char[5];
	char LDEV_each_hex_digit_as_char[4];

	// bytes 48-51 (4 bytes)
	char port[4]; // says 2 ASCII characters like " 1A ".

	// bytes 52-55 volume status 0x01 = SMPL, 0x02 = PVOL, 0x03 = SVOL
	char TC;
	char SI0; // MU#0
	char SI1; // MU#1
	char SI2; // MU#2

	// bytes 56-95 (40 bytes)
	char reserved_56_to_95[40];

	// bytes 96-103 (8 bytes) LDEV info
	unsigned char location_high;  // parity group name for internal LDEVs high-low
	unsigned char location_low;   // in case of DP-Vol, these two byes contain Pool ID high, Pool ID low
	                              // in case of external volume, external group 0x0001 to 0x4000)
	char RAID_level; // 1, 5, or 6.  External volume 16 (0x10).  DP-VOL: 18 (0x12).
	unsigned char SSIDhigh, SSIDlow;
	unsigned char CU;
	unsigned char number_of_CT_groups; // 0 to 15.  When LDEV is not paired for Asynch, this value is 0xFF.
	unsigned char reserved103;

	// bytes 104-119 (16 bytes)
	unsigned char PortWWN[8];
	unsigned char NodeWWN[8];

	// byte 120
	unsigned char VolumeProperty;
#define SCSI_VolumePropertyThinProvisioningInformationControl 0x20
#define SCSI_VolumePropertyCompareAndWriteATS 0x10
#define SCSI_VolumePropertyExtendedCopyFast 0x08
#define SCSI_VolumePropertyExtendedCopyFull 0x04
#define SCSI_VolumePropertyWriteSame 0x02

	// byte 121
	unsigned char LDEV_guard_status;
#define SCSI_LDEVguard_SVOLprotect 0x10
#define SCSI_LDEVguard_ReadCapacityInhibit 0x08
#define SCSI_LDEVguard_InquiryInhibit 0x04
#define SCSI_LDEVguard_WriteInhibit 0x02
#define SCSI_LDEVguard_ReadInhibit 0x01


	// bytes 122-123 (2 bytes)
	unsigned char hostgroupID[2];

	// byte 124  valid when LU is defined
	char drive_class;
#define SCSI_DriveClassFCorSAS 0x00
#define SCSI_DriveClassSATA 0x01
#define SCSI_DriveClassBD 0x02
#define SCSI_DriveClassNL_FC 0x08
#define SCSI_DriveClassSSDorFMD 0x10
		// says SATA, BD "in case of external"
		// says NL_FC "in case of external (not supported)"

	// byte 125
	char MF_retention; // 0 = not retained, 1 = retained
		// HM=0x4c: 1

	// byte 126
	char LDEV_guard_extent;

	// bytes 127-139 (13 bytes)
	unsigned char usedbyDF[13]; // reserved

	// bytes 140-143 (4 bytes)  logical partition number
	char SLPR;
	char CLPR;
	char reserved143[2];

	// bytes 144-151 (8 bytes) Volstat
	char TC_UR_status[8];
		// for MU#0-7, 1 means SMPL, 2 means PVOL, 3 means SVOL

	// bytes 152-183 (32 bytes) Volstat
	char SI_status[32]; // 1 means SMPL, 2 means PVOL, 3 means SVOL

	// bytes 184-191 (8 bytes)
	unsigned char CTGID[8]; // consistency group id for MU#0-7 for CAVOL
		// 0xFF simplex
		// TC/UR CTG#

	// bytes 192-211 (20 bytes)
	unsigned char copy_differential_bitmap[20];

	// bytes 212-243 (32 bytes)
	char LDEV_nickname[32];

} Hitachi_RAID_SI_format;

typedef struct Hitachi_page_E0 {

// standard header bytes - 4 bytes
	unsigned int PeripheralQualifier : 3;
	unsigned int PeripheralDeviceType : 5;
	unsigned char PageCode; // 0xE0;
	unsigned char reserved2; // zeros
	unsigned char additional_length; // should be 0x90;
		// size should be standard header 4 bytes plus unique contents 0x90 or 144 decimal
		// for total size 0x94 or decimal 148 bytes

// payload bytes
	char reserved_byte_4; // zeros
	unsigned char MicroVersion[5]; // microcode version in binary
	char reserved_bytes_10_15[6];
	char VendorID[8]; // "HITACHI "
	char LocalID[8]; // "-HI-CMD-"
	char ProductID[4]; // e.g. "R800" for RAID800
	unsigned char sub_model_ID;
	char reserved_37_to_39[3];
	unsigned char HostGroup[2]; // binary info
	char reserved_42_to_55[14];
	char NodeSN[8]; // ASCII representation of decimal value, serial number
	char reserved_64_to71[8];
	unsigned char CmdDev;  // 0 means not command device, 0x80 (128) means command device
	char reserved_73_to75[3];
	char LunNumberHexAsAscii[4];
	unsigned char PortWWN[8];
		// 0x60
		// 0x060E80 (Hitachi company code)
		// 07 - model code
		// 2 bytes of serial number
		// 1 byte Port
	char reserved_88;
	char PortType; // ASCII '5'
	char reserved_90_to_92[3];
	unsigned char PortSerialNumber_low; // binary data
	char reserved_94_to_103[10];
	unsigned char PortSerialNumber_high; // binary data
	char reserved_105_to_147[43];

} Hitachi_page_E0;
