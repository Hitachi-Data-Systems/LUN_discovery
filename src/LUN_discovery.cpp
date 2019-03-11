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

// LUN_discovery.cpp

#define _LARGE_FILE_API
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <scsi/sg.h>

using namespace std;

#define VENDOR 8
#define VENDOR_LEN 8
#define VENDORHITACHI "HITACHI "

#define PRODUCT 16
#define PRODUCT_LEN 16
#define PRODUCTDF600F "DF600F          "
#define PRODUCTDISKSUBSYSTEM "DISK-SUBSYSTEM  "
#define PRODUCTOPENVCM "OPEN-V-CM       "
#define PRODUCTOPENV "OPEN-V          "
#define PRODUCTOPENVA "OPEN-V-A        "
#define CMD_DEV "cmd dev"

#define REVISION 32
#define REVISION_LEN 4
#define REVISION0000 "0000"
#define REVISION5001 "5001"

#define VENDOR_SPECIFIC 36
#define VENDOR_SPECIFIC_LEN 20
#define PORTWITHINVENDORSPECIFIC 13
#define LDEVWITHINVENDORSPECIFIC 8

#include "printableAndHex.h"
#include "LUN_discovery.h"

uint16_t load_16bit(const unsigned char* p)
{
    uint16_t r = 0;
    for (unsigned int i=0; i<2; i++)
    {
        r = r << 8;
        r += ((uint16_t) (*p++));
    }
    return r;
}

uint32_t load_32bit(const unsigned char* p)
{
    uint32_t r = 0;
    for (unsigned int i=0; i<4; i++)
    {
        r = r << 8;
        r += ((uint32_t) (*p++));
    }
    return r;
}

uint64_t load_64bit(const unsigned char* p)
{
    uint64_t r = 0;
    for (unsigned int i=0; i<8; i++)
    {
        r = r << 8;
        r += ((uint64_t) (*p++));
    }
    return r;
}

//#define DEBUG ON

std::string decode_hex_serial_number(const unsigned char* p);

void LUN_discovery::printheaders(ostream& o)
{
    o << printheaders() << std::endl;
}

std::string LUN_discovery::printheaders()
{
    return std::string("hostname,SCSI Bus Number (HBA),LUN Name,Hitachi Product,HDS Product,Product Revision,Serial Number,Port,LUN 0-255,LDEV,Nickname,LDEV type")
           + std::string(",notes")
           + std::string(",RAID level,Parity Group,Pool ID,CLPR,Max LBA")
           + std::string(",Size MB,Size MiB,Size GB,Size GiB,Size TB,Size TiB")
           + std::string(",Vendor,Product,Product Revision,port_wwn,node_wwn,protect")
           + std::string(",SSID,CU,consistency_group")
           + std::string(",TC")
           + std::string(",MRCF_M0,MRCF_M1,MRCF_M2")
           + std::string(",HORC_M0,HORC_M1,HORC_M2,HORC_M3")
           + std::string(",HUVM_role,physical_product,physical_product_revision,physical_product_ID,physical_submodel,physical_serial,physical_LDEV,physical_LDEV_type"
            ",physical_PG,physical_RAID_level, physical_Pool_ID,physical_nickname,physical_SSID,physical_CU,physical_consistency_group"
            ",GAD_remote_serial,GAD_remote_LDEV"
            ",write_same,page_size_sectors,zero_reclaim_starting_sector"
            ",pages_in_use,pool_threshold,available_threshold"
            ",pool_usage,pool_remaining_MiB,total_pool_MiB,optimal_write_same_granularity_sectors"
            ",physical_pool_usage,pool_saving_rate,pool_physical_remaining_MiB,pool_total_physical_MiB")
           + std::string(",SCSI_IOCTL_PROBE_HOST");
}

void LUN_discovery::printdata(ostream& o)
{
    o << printdata() << std::endl;
}

std::string LUN_discovery::printdata()
{
    ostringstream os;
    os << hostname << ',' << bus_number << ',' << LUNname << ',' << HitachiProduct << ',' << HDSProduct
       << ',' << ProductRevisionUnprintableAsDot
       << ',' << SerialNumber
       << ',' << Port << ',' << LU_Number << ',' << LDEV << ',' << nickname << ',' << LDEV_type << ',' << notes << ',' << RAIDlevel << ','
       << PG << ',' << poolID << ',' << CLPR << ',' << std::fixed << maxLBA
       << ',' << SizeMB << ',' << SizeMiB << ',' << SizeGB << ',' << SizeGiB << ',' << SizeTB << ',' << SizeTiB
       //<< ',' << ((openROgood) ? (openRWgood) ? std::string("O_RDWR") : std::string("O_RDONLY") : std::string("") )
       << ',' << VendorUnprintableAsDot << ',' << ProductUnprintableAsDot << ',' << ProductRevisionUnprintableAsDot
       << ',' << port_wwn << ',' << node_wwn
       << ',' << protect
       << ',' << SSID << ',' << CU << ',' << consistency_group
       << ',' << TC_volstat
       << ',' << SI_M0_volstat << ',' << SI_M1_volstat << ',' << SI_M2_volstat
       << ',' << UR_M0_volstat << ',' << UR_M1_volstat << ',' << UR_M2_volstat << ',' << UR_M3_volstat

        << ',' << HUVM_role
        << ',' << physical_product    // e.g. OPEN-V
        << ',' << physical_product_revision
        << ',' << physical_product_ID   // 4 character, e.g. R800
        << ',' << physical_submodel
        << ',' << physical_serial
        << ',' << physical_LDEV
        << ',' << physical_LDEV_type
        << ',' << physical_PG
        << ',' << physical_RAID_level
        << ',' << physical_Pool_ID
        << ',' << physical_nickname
        << ',' << physical_SSID
        << ',' << physical_CU
        << ',' << physical_consistency_group
        << ',' << GAD_remote_serial
        << ',' << GAD_remote_LDEV

        << ',' << write_same
        << ',' << page_size_sectors
        << ',' << zero_reclaim_starting_sector
        << ',' << pages_in_use
        << ',' << pool_threshold
        << ',' << available_threshold
        << ',' << pool_usage
        << ',' << pool_remaining_MiB
        << ',' << total_pool_MiB
        << ',' << optimal_write_same_granularity_sectors
        << ',' << physical_pool_usage
        << ',' << pool_saving_rate
        << ',' << pool_physical_remaining_MiB
        << ',' << pool_total_physical_MiB

        << ',' << ((char*)PROBE_HOSTbuf);
    return os.str();
}

std::string showRAIDspecific ()
{
    std::string s = std::string("");
    return s;
}

void LUN_discovery::showall(ostream& o)
{

    // print default SCSI Inquiry page
    o << std::endl;

    //if (haveDefaultPage) { display_memory_contents(o, def_buf, 4+def_buf[3], 16,"Page 0x00 with EVPD=0 (default_page): "); o << std::endl; }
    if (haveDefaultPage)
    {
        display_memory_contents(o, def_buf, sizeof(def_buf), 16,"Page 0x00 with EVPD=0 (default_page): ");
        o << std::endl;
    }
    if (def_sense_bytes.length()>0)
    {
        o << "Page 0x00 with EVPD=0 - " << def_sense_bytes;
        o << std::endl;
    }

    ////if (haveDefaultPage_EVPD) { display_memory_contents(o, def_buf_EVPD, 4+def_buf_EVPD[3], 16,"Page 0x00 with EVPD=1 (default_page): "); o << std::endl; }
    //if (haveDefaultPage_EVPD) { display_memory_contents(o, def_buf_EVPD, sizeof(def_buf_EVPD), 16,"Page 0x00 with EVPD=1 (default_page): "); o << std::endl; }
    //if (def_sense_bytes_EVPD.length()>0) { o << "Page 0x00 with EVPD=1 - " << def_sense_bytes_EVPD; o << std::endl; }

    //if (haveVpdPage) { display_memory_contents(o, buf_83_vpd, 4+buf_83_vpd[3], 16,"Page 0x83 with EVPD=1: "); o << std::endl; }
    if (haveVpdPage)
    {
        display_memory_contents(o, buf_83_vpd, sizeof(buf_83_vpd), 16,"Page 0x83 with EVPD=1: ");
        o << std::endl;
    }
    if (vpd_sense_bytes.length()>0)
    {
        o << "Page 0x83 with EVPD=1 sense bytes - " << vpd_sense_bytes;
        o << std::endl;
    }

    //if (haveVpdPage_EVPD_off) { display_memory_contents(o, vpd_buf_EVPD_off, 4+vpd_buf_EVPD_off[3], 16,"Page 0x83 with EVPD=0: "); o << std::endl; }
    //if (vpd_sense_bytes_EVPD_off.length()>0) { o << "Page 0x83 with EVPD=0 - " << vpd_sense_bytes_EVPD_off; o << std::endl; }

    if (haveE0page)
    {
        display_memory_contents(o, (unsigned char*)&E0_buf, E0_buf.additional_length, 16,"Page 0xE0 with EVPD=: ");
        o << std::endl;
    }
    if (page_E0_sense_bytes.length()>0)
    {
        o << "Page 0x83 with EVPD=1 - " <<page_E0_sense_bytes;
        o << std::endl;
    }

    if (have_E2_sub_0_HAM_buf)
    {
        display_memory_contents(o, E2_sub_0_HAM_buf, 4+E2_sub_0_HAM_buf[3], 16,"Page 0xE2 subpage 0 [HAM]: ");
        o << std::endl;
    }
    if (page_E2_sub_0_sense_bytes.length()>0)
    {
        o << "Page 0xE2 subpage 0 [HAM] - " <<page_E2_sub_0_sense_bytes;
        o << std::endl;
    }

    if (have_E2_sub_1_NDM_buf)
    {
        display_memory_contents(o, E2_sub_1_NDM_buf, 4+E2_sub_1_NDM_buf[3], 16,"Page 0xE2 subpage 1 [NDM]: ");
        o << std::endl;
    }
    if (page_E2_sub_1_sense_bytes.length()>0)
    {
        o << "Page 0xE2 subpage 1 [NDM] - " <<page_E2_sub_1_sense_bytes;
        o << std::endl;
    }

    if (have_E2_sub_2_GAD_buf)
    {
        display_memory_contents(o, E2_sub_2_GAD_buf, 4+E2_sub_2_GAD_buf[3], 16,"Page 0xE2 subpage 2 [HUVM]: ");
        o << std::endl;
    }
    if (page_E2_sub_2_sense_bytes.length()>0)
    {
        o << "Page 0xE2 subpage 2 [GAD] - " <<page_E2_sub_2_sense_bytes;
        o << std::endl;
    }

    if (have_E3_THP_buf)
    {
        display_memory_contents(o, E3_THP_buf, 4+E3_THP_buf[3], 16,"Page 0xE3 HDP/HDT: ");
        o << std::endl;
    }
    if (page_E3_THP_sense_bytes.length()>0)
    {
        o << "Page 0xE3 HDP/HDT - " << page_E3_THP_sense_bytes;
        o << std::endl;
    }

    o << "Constructor log:" << std::endl << constructor_log << std::endl;
}

LUN_discovery::LUN_discovery( std::string L) : LUNname(L)
{
    HitachiProduct = HDSProduct = SerialNumber = Port = LDEV
        = ProductRevisionUnprintableAsDot = VendorSpecificUnprintableAsDotWithHex = ProductUnprintableAsDot
        = std::string("");
    VendorUnprintableAsDot
        = std::string("<SCSI Inquiry failed for \"") + LUNname + std::string("\".  \"InquireAbout\" and \"lun2string\" must be owned by root and setuid to be authorized to issue SCSI Inquiry commands.>");


    char hostnamebuf[256];
    hostnamebuf[255]=0;
    if (gethostname(hostnamebuf,255))
    {
        hostname="gethostname()_failed";
    }
    else
    {
        hostname=hostnamebuf;
    }

    HitachiVPD = VendorUnprintableAsDot = ProductUnprintableAsDot = ProductRevisionUnprintableAsDot = "";
    std::string errmsg;
    if ((fd = open64(LUNname.c_str(), O_RDWR + O_LARGEFILE + O_DIRECT)) < 0)
    {
        errmsg="LUN_discovery::LUN_discovery(,\"" + LUNname + "\") - open O_RDWR failed";
        constructor_log = constructor_log + errmsg + '\n';
        openRWgood=false;
        if ((fd = open64(LUNname.c_str(), O_RDONLY + O_LARGEFILE + O_DIRECT)) < 0)
        {
            errmsg="LUN_discovery::LUN_discovery(,\"" + LUNname + "\") - open O_RDONLY failed";
            constructor_log = constructor_log + errmsg + '\n';
            openRWgood=false;
            openROgood=false;
            return;
        }
        else
        {
            openRWgood=false;
            openROgood=true;
        }
    }
    else
    {
        openRWgood=true;
        openROgood=true;
    }
    int rc;
    memset(&my_sg_scsi_id,0, sizeof(my_sg_scsi_id));
    if (0!=(rc=ioctl(fd,SG_GET_SCSI_ID, ((void*)(&my_sg_scsi_id)))))
    {
        ostringstream o;
        o <<"ioctl(fd,SG_GET_SCSI_ID,) return code = " << rc << ", errno = " << errno << " - " << strerror(errno) << " [note: seeing return code -1 errno=22 is actually normal.]";
        // fails rc=-1, errno = 22 - invalid argument.  Too bad.  This was the only way I could find to get the LU Number as an int instead of an unsigned char like in SCSI_IOCTL_GET_IDLUN
        //                                                        This is important because LU Numbers can be bigger than 255.
        SG_GET_SCSI_ID_string = o.str();
    }
    else
    {
        std::ostringstream o;
        o << "<host=" << my_sg_scsi_id.host_no
          << ",channel=" << my_sg_scsi_id.channel
          << ",scsi_id=" << my_sg_scsi_id.scsi_id
          << ",lun=" << my_sg_scsi_id.lun;
        switch (my_sg_scsi_id.scsi_type)
        {
        case (TYPE_DISK):
            o << "TYPE_DISK";
            break;
        case (TYPE_TAPE):
            o << "TYPE_TAPE";
            break;
        case (TYPE_PROCESSOR):
            o << "TYPE_PROCESSOR";
            break;
        case (TYPE_WORM):
            o << "TYPE_WORM";
            break;
        case (TYPE_ROM):
            o << "TYPE_ROM";
            break;
        case (TYPE_SCANNER):
            o << "TYPE_SCANNER";
            break;
        case (TYPE_MOD):
            o << "TYPE_MOD";
            break;
        case (TYPE_MEDIUM_CHANGER):
            o << "TYPE_MEDIUM_CHANGER";
            break;
        case (TYPE_ENCLOSURE):
            o << "TYPE_ENCLOSURE";
            break;
        case (TYPE_NO_LUN):
            o << "TYPE_NO_LUN";
            break;
        default:
            o << "unknown_type";
        }
        o << ",host_orHBA_max_cmds_per_LUN=" << my_sg_scsi_id.h_cmd_per_lun
          << ",device_or_HBA=" << my_sg_scsi_id.d_queue_depth
          << ">";
        SG_GET_SCSI_ID_string=o.str();
    }
    constructor_log=constructor_log + SG_GET_SCSI_ID_string + std::string("\n");

    bus_number=0;
    if (0!=(rc=ioctl(fd,SCSI_IOCTL_GET_BUS_NUMBER,&bus_number)))
    {
        std::ostringstream o;
        o << "ioctl(fd,SCSI_IOCTL_GET_BUS_NUMBER,&bus_number) return code " << rc << " - " << strerror(errno) << std::endl;
        display_memory_contents(o, ((unsigned char*) &bus_number), sizeof(bus_number), 16, "bus_number");
        o<< std::endl;
        constructor_log= constructor_log + o.str();
    }
    else
    {
        std::ostringstream o;
        o << "ioctl(fd,SCSI_IOCTL_GET_BUS_NUMBER,&bus_nubmer) return code == 0" << std::endl;
        display_memory_contents(o, ((unsigned char*) &bus_number), sizeof(bus_number), 16, "bus_number");
        o << "bus_number=" << bus_number << std::endl;
        o<< std::endl;
        constructor_log= constructor_log + o.str();
    }

    memset(&PROBE_HOSTbuf,0,sizeof(PROBE_HOSTbuf));
    int* p_PROBE_HOST = (int*)&PROBE_HOSTbuf;
    (*p_PROBE_HOST)=sizeof(PROBE_HOSTbuf)-1;
    if (-1==(rc=ioctl(fd,SCSI_IOCTL_PROBE_HOST,&PROBE_HOSTbuf)))
    {
        std::ostringstream o;
        o << "ioctl(fd,SCSI_IOCTL_PROBE_HOST,&PROBE_HOSTbuf) return code " << rc << " - " << strerror(errno) << std::endl;
        PROBE_HOSTbuf[0]=0;
        constructor_log= constructor_log + o.str();
    }
    else if (0==rc)
    {
        PROBE_HOSTbuf[0]=0;
    }
    else
    {
        std::ostringstream o;
        o << "SCSI_IOCTL_PROBE_HOST=" << PROBE_HOSTbuf << std::endl;
        constructor_log = constructor_log + o.str();
    }

    memset (&scsi_id,0,sizeof(scsi_id));
    if (0!=(rc=ioctl(fd,SCSI_IOCTL_GET_IDLUN, &scsi_id)))
    {
        std::ostringstream o;
        o <<"ioctl(fd,SCSI_IOCTL_GET_IDLUN,) return code = " << rc << ", errno = " << errno << " - " << strerror(errno);
        scsi_id_string = o.str();
        LU_Number=-1;
    }
    else
    {
        std::ostringstream o;
        o << '<' << ((scsi_id.dev_id >> 24) & 0x000000FF) << ':' << ((scsi_id.dev_id >> 16) & 0x000000FF)  << ':' << (scsi_id.dev_id & 0x000000FF) << ':' << ((scsi_id.dev_id >> 8) & 0x000000FF)  << '>' ;
        // Note that for some reason, although we write the fields in this order, "<host_no, channel, scsi_id, lun>,
        // the fields in the uint32_t are in a different order -  scsi_device_id | (lun << 8) | (channel << 16) | (host_no << 24))
        scsi_id_string = o.str();
        LU_Number =  ((scsi_id.dev_id >> 8) & 0x000000FF);
    }
    constructor_log = constructor_log + std::string("ioctl(fd,SCSI_IOCTL_GET_IDLUN,) yielded ") + scsi_id_string + std::string("\n");


    /* It is prudent to check we have a sg device by trying an ioctl */
    if ((ioctl(fd, SG_GET_VERSION_NUM, &k) < 0) || (k < 30000))
    {
        errmsg= "LUN_discovery::LUN_discovery(,\"" + LUNname + "\") - is not an sg (generic SCSI) device, or old sg driver.";
        constructor_log = constructor_log + errmsg + '\n';
        close(fd);
        return;
    }
    haveDefaultPage = get_page( (unsigned char*)&(def_buf[0]), sizeof(def_buf), def_cmd, "page 0x00 with EVPD=0", &def_sense_bytes);
    if (!haveDefaultPage)
    {
        close(fd);
        return;
    }
    //haveDefaultPage_EVPD = get_page( (unsigned char*)&(def_buf_EVPD[0]), sizeof(def_buf_EVPD), def_cmd_EVPD, "page 0x00 with EVPD=1", &def_sense_bytes_EVPD);

    unsigned char* d = (unsigned char *)def_buf;
    VendorUnprintableAsDot = printableAsDot(d+VENDOR,VENDOR_LEN);
    ProductUnprintableAsDot = printableAsDot(d+PRODUCT,PRODUCT_LEN);
    ProductRevisionUnprintableAsDot = printableAsDot(d+REVISION,REVISION_LEN);
    VendorSpecificUnprintableAsDotWithHex = printableAndHex(d+VENDOR_SPECIFIC,VENDOR_SPECIFIC_LEN);

    HitachiProduct=HDSProduct=SerialNumber=Port=LDEV=(std::string(""));

    // now get Vital Product Data page 0x83
    haveVpdPage = get_page( (unsigned char*)&(buf_83_vpd[0]), sizeof(buf_83_vpd), vpd_cmd, "page 0x83 with EVPD=1", &vpd_sense_bytes);

    ostringstream oHitachiVPD;
    display_memory_contents(oHitachiVPD, ((unsigned char*) buf_83_vpd)+38, 16, 16, "");
    HitachiVPD=oHitachiVPD.str();

    /* INQUIRY response is present */
    char * p = (char *)def_buf;
    unsigned char* q = (unsigned char*) p;


    if (VendorUnprintableAsDot!=std::string(VENDORHITACHI))
    {
        // vendor is not Hitachi
        HitachiProduct=HDSProduct=SerialNumber=Port=LDEV="";
    }
    else
    {
        // vendor is Hitachi
        isHitachi=true;
        HitachiProduct=HDSProduct=SerialNumber=Port=LDEV=(std::string("<Hitachi LUN \"") + LUNname + std::string("\" - not recognized Hitachi subsystem type>"));

        haveE0page = get_page( (unsigned char*)&E0_buf, sizeof(E0_buf), E0_cmd, "Hitachi-specific page 0xE0", &page_E0_sense_bytes);

        if (!haveE0page) return;

        E0_product   =  printableAsDot((unsigned char*)&E0_buf.ProductID, sizeof(E0_buf.ProductID));
        SerialNumber =  printableAsDot((unsigned char*)&E0_buf.NodeSN,    sizeof(E0_buf.NodeSN));

        have_E2_sub_0_HAM_buf = get_page( E2_sub_0_HAM_buf, sizeof(E2_sub_0_HAM_buf), E2_sub_0_cmd, "0xE2 sub page 0 (HAM): ", &page_E2_sub_0_sense_bytes );
        have_E2_sub_1_NDM_buf = get_page( E2_sub_1_NDM_buf, sizeof(E2_sub_1_NDM_buf), E2_sub_1_cmd, "0xE0 sub page 1 (NDM): ", &page_E2_sub_1_sense_bytes );
        have_E2_sub_2_GAD_buf = get_page( E2_sub_2_GAD_buf, sizeof(E2_sub_2_GAD_buf), E2_sub_2_cmd, "0xE0 sub page 2 (GAD): ", &page_E2_sub_2_sense_bytes );

#ifdef DEBUG
        std::cout << "sizeof EO page = " << sizeof(E0_buf) << std::endl;
        std::cout << "E0 microcode version = " << printableAndHex((unsigned char*)E0_buf.MicroVersion, sizeof(E0_buf.MicroVersion)) << std::endl;
        std::cout << "E0 vendor ID = " << printableAndHex((unsigned char*)&E0_buf.VendorID, sizeof(E0_buf.VendorID)) << std::endl;
        std::cout << "E0 local ID = " << printableAndHex((unsigned char*)&E0_buf.LocalID, sizeof(E0_buf.LocalID)) << std::endl;
        std::cout << "E0 product ID = " << printableAndHex((unsigned char*)&E0_buf.ProductID, sizeof(E0_buf.ProductID)) << std::endl;
        std::cout << "E0 host group = " << printableAndHex((unsigned char*)&E0_buf.HostGroup, sizeof(E0_buf.HostGroup)) << std::endl;
        std::cout << "E0 node S/N = " << printableAndHex((unsigned char*)&E0_buf.NodeSN, sizeof(E0_buf.NodeSN)) << std::endl;
        std::cout << "E0 command dev = " << printableAndHex((unsigned char*)&E0_buf.CmdDev, sizeof(E0_buf.CmdDev)) << std::endl;
        std::cout << "E0 LU number (ascii of hex value) = " << printableAndHex((unsigned char*)&E0_buf.LunNumberHexAsAscii, sizeof(E0_buf.LunNumberHexAsAscii)) << std::endl;
        std::cout << "E0 port WWN = " << printableAndHex((unsigned char*)&E0_buf.PortWWN, sizeof(E0_buf.PortWWN)) << std::endl;
        std::cout << "E0 port type ('5') = " << printableAndHex((unsigned char*)&E0_buf.PortType, sizeof(E0_buf.PortType)) << std::endl;
        std::cout << "E0 port high = " << printableAndHex((unsigned char*)&E0_buf.PortSerialNumber_high, sizeof(E0_buf.PortSerialNumber_high)) << std::endl;
        std::cout << "E0 port low = " << printableAndHex((unsigned char*)&E0_buf.PortSerialNumber_low, sizeof(E0_buf.PortSerialNumber_low)) << std::endl;
#endif//DEBUG
        // Port
        if ( (! isdigit( (int) *(q+VENDOR_SPECIFIC+PORTWITHINVENDORSPECIFIC)))
                || (! isalpha( (int) *(q+VENDOR_SPECIFIC+PORTWITHINVENDORSPECIFIC+1))) )
        {
            ostringstream os;
            os << "Invalid port name digit in position " << PORTWITHINVENDORSPECIFIC
               << " or port name alphabetic in position " << (PORTWITHINVENDORSPECIFIC+1) << " of vendor-specific data";
            Port=os.str();
        }
        else
        {
            Port=std::string(((const char*)q)+VENDOR_SPECIFIC+PORTWITHINVENDORSPECIFIC, 2);
        }

        // LDEV (later if it's RAID we'll stick a ':' in the middle.)
        bool LDEVlooksOK=true;
        for (int i=0; i<4; i++)
        {
            if(! isalnum((int)*(q+VENDOR_SPECIFIC+LDEVWITHINVENDORSPECIFIC+i)))
            {
                LDEVlooksOK=false;
                break;
            }
        }
        if ( ! LDEVlooksOK )
        {
            ostringstream os;
            os << "Invalid LDEV or LUN name characters in the four positions starting at offset "
               << LDEVWITHINVENDORSPECIFIC << " in vendor-specific data.";
            LDEV=os.str();
        }
        else
        {
            LDEV_no_colon = LDEV = std::string(((const char*)q)+VENDOR_SPECIFIC+LDEVWITHINVENDORSPECIFIC,4);
        }

        if (ProductUnprintableAsDot == std::string(PRODUCTDF600F))
        {
            // subsystem type DF

            bool DFSerialAllDigits=true;
            for (int i=0; i<8; i++)
            {
                if(! isdigit((int) *(q+VENDOR_SPECIFIC+i)))
                {
                    DFSerialAllDigits=false;
                    break;
                }
            }
            if (!DFSerialAllDigits)
            {
                SerialNumber="DF serial number in first 8 positions of vendor-specific data has non-numeric characters.";
            }
            else
            {
                SerialNumber = std::string(((const char*)q)+VENDOR_SPECIFIC,8);
            }

            std::string SNprefix = SerialNumber.substr(0,2);

            // DF700H	AMS1000	770
            // DF700M	AMS500	750
            // DF800H	AMS2500	870
            // DF800M	AMS2300	850
            // DF800S	AMS2100	830
            // DF850XS	HUS 110	91x
            // DF850S	HUS 130	92x
            // DF850MH	HUS 150	930

            if (SNprefix==std::string("93"))
            {

                HitachiProduct="DF850MH";
                HDSProduct="HUS150";

            }
            else if (SNprefix==std::string("92"))
            {

                HitachiProduct="DF850S";
                HDSProduct="HUS130";

            }
            else if (SNprefix==std::string("91"))
            {

                HitachiProduct="DF850XS";
                HDSProduct="HUS110";

            }
            else if (SNprefix==std::string("87"))
            {

                HitachiProduct="DF800H";
                HDSProduct="AMS2500";

            }
            else if (SNprefix==std::string("85"))
            {

                HitachiProduct="DF800M";
                HDSProduct="AMS2300";

            }
            else if (SNprefix==std::string("83"))
            {

                HitachiProduct="DF800S";
                HDSProduct="AMS2100";

            }
            else if (SNprefix==std::string("77"))
            {

                HitachiProduct="DF700H";
                HDSProduct="AMS1000";

            }
            else if (SNprefix==std::string("75"))
            {

                HitachiProduct="DF700M";
                HDSProduct="AMS500";


            }
            else
            {
                HitachiProduct="NotRecognized";
                HDSProduct="";
            }
            // subsystem type DF

        }
        else if ( (ProductUnprintableAsDot == std::string(PRODUCTDISKSUBSYSTEM))
               || (ProductUnprintableAsDot.substr(0,5) == std::string("OPEN-")) )
        {

            // subsystem type RAID

            std::string PrID = E0_buf.ProductID;

            if (PrID.substr(0,4) == std::string("R900"))
            {
                switch (E0_buf.sub_model_ID)
                {
                    case 0x00:
                        HitachiProduct = "Jupiter"; // "RAID900";
                        HDSProduct = "Europa"; // "VSP 5100 / VSP 5500"
                        break;
                    case 0x01:
                        HitachiProduct = "Jupiter"; // "RAID900";
                        HDSProduct = "Ganymede"; // Don't know Hitachi Vantara name yet
                        break;
                    default:
                    {
                        std::ostringstream o;
                        o << "Unknown submodel 0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) E0_buf.sub_model_ID
                            << " for Product ID \"R900\".";
                        HitachiProduct = HDSProduct = o.str();
                    }
                }
            }
            else if (PrID.substr(0,4) == std::string("R800"))
            {
                switch (E0_buf.sub_model_ID)
                {
                    case 0x00:
                        HitachiProduct = "RAID800";
                        HDSProduct = "VSP G1000";
                        break;
                    case 0x80:
                        HitachiProduct = "RAID800";
                        HDSProduct = "VSP G1500";
                        break;
                    case 0xC0:
                        HitachiProduct = "RAID800";
                        HDSProduct = "VSP F1500";
                        break;
                    default:
                    {
                        std::ostringstream o;
                        o << "Unknown submodel 0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) E0_buf.sub_model_ID
                            << " for Product ID \"R800\".";
                        HitachiProduct = HDSProduct = o.str();
                    }
                }
            }
            else if (PrID.substr(0,4) == std::string("HM86"))
            {
                switch (E0_buf.sub_model_ID)
                {
                    case 0x00:
                        HitachiProduct = "HM800H";
                        HDSProduct = "VSP G800";
                        break;
                    case 0x40:
                        HitachiProduct = "HM800H";
                        HDSProduct = "VSP F800";
                        break;
                    case 0x20:
                        HitachiProduct = "HM850H";
                        HDSProduct = "VSP G900";
                        break;
                    case 0x60:
                        HitachiProduct = "HM850H";
                        HDSProduct = "VSP F900";
                        break;
                    default:
                    {
                        std::ostringstream o;
                        o << "Unknown submodel 0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) E0_buf.sub_model_ID
                            << " for Product ID \"HM86\".";
                        HitachiProduct = HDSProduct = o.str();
                    }
                }
            }
            else if (PrID.substr(0,4) == std::string("HM84"))
            {
                switch (E0_buf.sub_model_ID)
                {
                    case 0x00:
                        HitachiProduct = "HM800M";
                        HDSProduct = "VSP G400/G600";
                        break;
                    case 0x40:
                        HitachiProduct = "HM800M";
                        HDSProduct = "VSP F400/F600";
                        break;
                    case 0x23:
                        HitachiProduct = "HM850M";
                        HDSProduct = "VSP G700";
                        break;
                    case 0x63:
                        HitachiProduct = "HM850M";
                        HDSProduct = "VSP F700";
                        break;
                    default:
                    {
                        std::ostringstream o;
                        o << "Unknown submodel 0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) E0_buf.sub_model_ID
                            << " for Product ID \"HM84\".";
                        HitachiProduct = HDSProduct = o.str();
                    }
                }
            }
            else if (PrID.substr(0,4) == std::string("HM82"))
            {
                switch (E0_buf.sub_model_ID)
                {
                    case 0x00:
                        HitachiProduct = "HM800S";
                        HDSProduct = "VSP G200";
                        break;
                    case 0x20:
                        HitachiProduct = "HM850S0";
                        HDSProduct = "VSP G150";
                        break;
                    case 0x21:
                        HitachiProduct = "HM850S1";
                        HDSProduct = "VSP G350";
                        break;
                    case 0x22:
                        HitachiProduct = "HM850S2";
                        HDSProduct = "VSP G370";
                        break;
                    case 0x24:
                        HitachiProduct = "HM850XS";
                        HDSProduct = "VSP G130";
                        break;
                    case 0x61:
                        HitachiProduct = "HM850S1";
                        HDSProduct = "VSP F350";
                        break;
                    case 0x62:
                        HitachiProduct = "HM850S2";
                        HDSProduct = "VSP F370";
                        break;
                    default:
                    {
                        std::ostringstream o;
                        o << "Unknown submodel 0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) E0_buf.sub_model_ID
                            << " for Product ID \"HM82\".";
                        HitachiProduct = HDSProduct = o.str();
                    }
                }
            }
            else if (    ((buf_83_vpd[38+4])==0x06)    // This different way of detecting RAID700 is because RAID700 is not documented in the SCSI Inquiry spec doc
                      || ((buf_83_vpd[38+4])==0x16)    // so I didn't know if the ProductID would be "R700".
                    )
            {
                HitachiProduct = "RAID700";
                HDSProduct = "VSP";
            }
            else
            {
                std::ostringstream o;
                o << "Unknown \"RAID\" (Hitachi enterprise family) subsystem Product ID \"" << (E0_buf.ProductID) << "\""
                    << " submodel 0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) E0_buf.sub_model_ID
                    << ".";
                HitachiProduct = HDSProduct = o.str();
            }

            if (LDEV.length() == 4)
            {
                LDEV=LDEV.substr(0,2) + ':' + LDEV.substr(2,2);
            }

            std::ostringstream pgstr,pg_no_hyphen_str;

            switch(def_buf[96+2])
            {
            case 0x01:
                LDEV_type="Internal";
                RAIDlevel="RAID-1";
                pgstr << /*std::setw(2) << std::setfill('0') << */ ((int) def_buf[96]) << '-' << /*std::setw(2) << std::setfill('0') << */ ((int)def_buf[97]);
                PG=pgstr.str();
                pg_no_hyphen_str << /*std::setw(2) << std::setfill('0') << */ ((int)def_buf[96]) << /* std::setw(2) << std::setfill('0') << */ ((int)def_buf[96]);
                PG_no_hyphen=pg_no_hyphen_str.str();
                break;
            case 0x05:
                LDEV_type="Internal";
                RAIDlevel="RAID-5";
                pgstr << /*std::setw(2) << std::setfill('0') << */ ((int) def_buf[96]) << '-' << /*std::setw(2) << std::setfill('0') << */ ((int)def_buf[97]);
                PG=pgstr.str();
                pg_no_hyphen_str << /*std::setw(2) << std::setfill('0') << */ ((int)def_buf[96]) << /* std::setw(2) << std::setfill('0') << */ ((int)def_buf[96]);
                PG_no_hyphen=pg_no_hyphen_str.str();
                break;
            case 0x06:
                LDEV_type="Internal";
                RAIDlevel="RAID-6";
                pgstr << /*std::setw(2) << std::setfill('0') << */ ((int) def_buf[96]) << '-' << /*std::setw(2) << std::setfill('0') << */ ((int)def_buf[97]);
                PG=pgstr.str();
                pg_no_hyphen_str << /*std::setw(2) << std::setfill('0') << */ ((int)def_buf[96]) << /* std::setw(2) << std::setfill('0') << */ ((int)def_buf[96]);
                PG_no_hyphen=pg_no_hyphen_str.str();
                break;
            case 0x10:
                LDEV_type="External";
                RAIDlevel="External";
                pgstr << "0x" << std::hex << std::setw(4) << std::setfill('0') << (   (((int)def_buf[96]) << 8)  +  ((int)def_buf[97])  );
                PG=PG_no_hyphen=pgstr.str();
                break;
            case 0x11:
                LDEV_type="TI_SVOL?_0x11";
                RAIDlevel="TI_SVOL?_0x11";
                pgstr << "0x" << std::hex << std::setw(4) << std::setfill('0') << (   (((int)def_buf[96]) << 8)  +  ((int)def_buf[97])  );
                PG=PG_no_hyphen=pgstr.str();
                break;
            case 0x12:
                LDEV_type="DP-Vol";
                RAIDlevel="DP-Vol";
                pgstr << ((((int)def_buf[96]) << 8) + ((int)def_buf[97]));
                poolID = pgstr.str();

                // We get the HDP stats from page E3
                have_E3_THP_buf = get_page( E3_THP_buf, sizeof(E3_THP_buf), E3_THP_cmd, "page 0xE3 HDP/HDT with EVPD=1", &page_E3_THP_sense_bytes);
                if (have_E3_THP_buf)
                {
                    unsigned int additional_length = (unsigned int) E3_THP_buf[3];
                    uint32_t vol_type {0};
                    if (additional_length >= 0x2E)
                    {
                        vol_type = load_32bit(E3_THP_buf+4);
                        if (vol_type == 0)
                        {
                            std::ostringstream o;
                            o << "<Error> Had already determined that a DP-Vol is being processed, but page 0xE3 vol type says 0x00000000 - not DP-Vol"
                            << " at line " << __LINE__ << " of " << __FILE__;
                            write_same = o.str();
                        }
                        else if (vol_type == 0x01000000 || vol_type == 0x00000001) // documentation only talks about a value 0x01, so ambiguous if one byte field is looked at
                        {
                            write_same = "not supported";
                        }
                        else if (vol_type == 0x02000000 || vol_type == 0x00000002) // documentation only talks about a value 0x01, so ambiguous if one byte field is looked at
                        {
                            write_same = "supported";
                        }
                        else
                        {
                            std::ostringstream o;
                            o << "<Error> page 0xE3 vol_type 0x" << std::hex << std::setw(8) << std::setfill('0') << vol_type << " not recognized"
                            << " at line " << __LINE__ << " of " << __FILE__;
                            write_same = o.str();
                        }

                        {
                            auto s = load_32bit(E3_THP_buf+8);
                            std::ostringstream o;
                            o << s;
                            page_size_sectors = o.str();
                        }

                        {
                            auto s = load_64bit(E3_THP_buf+12);
                            std::ostringstream o;
                            o << s;
                            zero_reclaim_starting_sector = o.str();
                        }

                        {
                            auto s = load_64bit(E3_THP_buf+20);
                            std::ostringstream o;
                            o << s;
                            pages_in_use = o.str();
                        }

                        {
                            unsigned int s = E3_THP_buf[30];
                            std::ostringstream o;
                            o << s << '%';
                            pool_threshold = o.str();
                        }

                        {
                            unsigned int s = E3_THP_buf[31];
                            std::ostringstream o;
                            o << s << '%';
                            available_threshold = o.str();
                        }

                        {
                            unsigned int s = E3_THP_buf[32];
                            std::ostringstream o;
                            o << s << '%';
                            pool_usage = o.str();
                        }

                        {
                            auto s = load_64bit(E3_THP_buf+34);
                            std::ostringstream o;
                            o << s;
                            pool_remaining_MiB = o.str();
                        }

                        {
                            auto s = load_64bit(E3_THP_buf+42);
                            std::ostringstream o;
                            o << s;
                            total_pool_MiB = o.str();
                        }
                    }
                    if (additional_length >= 0x32)
                    {
                        auto s = load_32bit(E3_THP_buf+50);
                        std::ostringstream o;
                        o << s;
                        optimal_write_same_granularity_sectors = o.str();
                    }
                    if (additional_length >= 0x44)
                    {

                        {
                            unsigned int s = E3_THP_buf[54];
                            std::ostringstream o;
                            o << s << '%';
                            physical_pool_usage = o.str();
                        }

                        {
                            unsigned int s = E3_THP_buf[55];
                            std::ostringstream o;
                            o << s << '%';
                            pool_saving_rate = o.str();
                        }

                        {
                            auto s = load_64bit(E3_THP_buf+56);
                            std::ostringstream o;
                            o << s;
                            pool_physical_remaining_MiB = o.str();
                        }

                        {
                            auto s = load_64bit(E3_THP_buf+64);
                            std::ostringstream o;
                            o << s;
                            pool_total_physical_MiB = o.str();
                        }
                    }
                }


                break;
            default:
//                pgstr << "RAID subsystem - unknown RAID level = 0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[96+2] << std::endl;
                pgstr << "phantom LUN" << std::endl;
                constructor_log += pgstr.str();
                LDEV_type=pgstr.str();
                RAIDlevel=pgstr.str();
            }

            {
                int nickname_length=0;
                for (unsigned char* p = &(def_buf[212]); (*p)!=0 && (nickname_length<31); p++) nickname_length++;
                if (nickname_length>0) nickname = printableAsDot((unsigned char*)&(def_buf[212]), nickname_length);
            }

            {
                std::ostringstream o;
                o << "CLPR" << ((int)def_buf[140+1]);
                CLPR=o.str();
            }

            if (ProductUnprintableAsDot == std::string(PRODUCTOPENVCM))
            {
                LDEV_type=CMD_DEV;
            }

            SerialNumber = decode_hex_serial_number(def_buf+36);

            if (haveDefaultPage)
            {
                port_wwn = printAsHex(def_buf+104,8);
                node_wwn = printAsHex(def_buf+104+8,8);

                {
                    std::ostringstream o;
                    o << "0x"
                        << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (unsigned int) def_buf[96+8+2+1]
                        << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (unsigned int) def_buf[96+8+2+1+1];
                    SSID = o.str();
                }

                {
                    std::ostringstream o;
                    o << "0x"
                        << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (unsigned int) def_buf[96+8+2+1+2];
                    CU = o.str();
                }

                unsigned char ctg = def_buf[96+8+2+1+2+1];
                if ( 0xFF != ctg)
                {
                    std::ostringstream o;
                    o << "0x"
                        << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (unsigned int) ctg;
                    consistency_group = o.str();
                }

                if (def_buf[5] && 1) { protect = std::string("T10_PI"); }

                switch (def_buf[52])
                {
                    case 1: TC_volstat = std::string("SMPL"); break;
                    case 2: TC_volstat = std::string("PVOL"); break;
                    case 3: TC_volstat = std::string("SVOL"); break;
                    default:
                    {
                        std::ostringstream o;
                        o << "unknown 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[52];
                        TC_volstat = o.str();
                    }
                }

                switch (def_buf[53])
                {
                    case 1: SI_M0_volstat = std::string("SMPL"); break;
                    case 2: SI_M0_volstat = std::string("PVOL"); break;
                    case 3: SI_M0_volstat = std::string("SVOL"); break;
                    default:
                    {
                        std::ostringstream o;
                        o << "unknown 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[53];
                        SI_M0_volstat = o.str();
                    }
                }

                switch (def_buf[54])
                {
                    case 1: SI_M1_volstat = std::string("SMPL"); break;
                    case 2: SI_M1_volstat = std::string("PVOL"); break;
                    case 3: SI_M1_volstat = std::string("SVOL"); break;
                    default:
                    {
                        std::ostringstream o;
                        o << "unknown 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[54];
                        SI_M0_volstat = o.str();
                    }
                }

                switch (def_buf[55])
                {
                    case 1: SI_M2_volstat = std::string("SMPL"); break;
                    case 2: SI_M2_volstat = std::string("PVOL"); break;
                    case 3: SI_M2_volstat = std::string("SVOL"); break;
                    default:
                    {
                        std::ostringstream o;
                        o << "unknown 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[55];
                        SI_M2_volstat = o.str();
                    }
                }

                switch (def_buf[144])
                {
                    case 1: UR_M0_volstat = std::string("SMPL"); break;
                    case 2: UR_M0_volstat = std::string("PVOL"); break;
                    case 3: UR_M0_volstat = std::string("SVOL"); break;
                    default:
                    {
                        std::ostringstream o;
                        o << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[144];
                        UR_M0_volstat = o.str();
                    }
                }

                switch (def_buf[144+1])
                {
                    case 1: UR_M1_volstat = std::string("SMPL"); break;
                    case 2: UR_M1_volstat = std::string("PVOL"); break;
                    case 3: UR_M1_volstat = std::string("SVOL"); break;
                    default:
                    {
                        std::ostringstream o;
                        o << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[144+1];
                        UR_M1_volstat = o.str();
                    }
                }

                switch (def_buf[144+2])
                {
                    case 1: UR_M2_volstat = std::string("SMPL"); break;
                    case 2: UR_M2_volstat = std::string("PVOL"); break;
                    case 3: UR_M2_volstat = std::string("SVOL"); break;
                    default:
                    {
                        std::ostringstream o;
                        o << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[144+2];
                        UR_M2_volstat = o.str();
                    }
                }

                switch (def_buf[144+3])
                {
                    case 1: UR_M3_volstat = std::string("SMPL"); break;
                    case 2: UR_M3_volstat = std::string("PVOL"); break;
                    case 3: UR_M3_volstat = std::string("SVOL"); break;
                    default:
                    {
                        std::ostringstream o;
                        o << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int) def_buf[144+3];
                        UR_M3_volstat = o.str();
                    }
                }
            }

            if (have_E2_sub_2_GAD_buf)
            {
                switch (E2_sub_2_GAD_buf[4])
                {
                    case (1):
                        HUVM_role = std::string("Migration Source");  // does not have remote DKC
                        break;
                    case (2):
                        HUVM_role = std::string("Migration Target");  // does not have remote DKC
                        break;
                    case (4):
                        HUVM_role = std::string("GAD PVOL");
                        break;
                    case (8):
                        HUVM_role = std::string("GAD SVOL");
                        break;
                    default:;
                }
                std::ostringstream pgstr;
                switch (E2_sub_2_GAD_buf[4])  // the second time, we get the info on physical DKC (and in the case of GAD, the remote physical DKC)
                {
                    case (4): case (8):
                        // info on remote physical DKC
                        GAD_remote_serial = decode_hex_serial_number(E2_sub_2_GAD_buf+200);
                        GAD_remote_LDEV.push_back(E2_sub_2_GAD_buf[200+8]);
                        GAD_remote_LDEV.push_back(E2_sub_2_GAD_buf[200+8+1]);
                        GAD_remote_LDEV.push_back(':');
                        GAD_remote_LDEV.push_back(E2_sub_2_GAD_buf[200+8+2]);
                        GAD_remote_LDEV.push_back(E2_sub_2_GAD_buf[200+8+3]);
                        // fallthrough to next bit without "break" is deliberate.
                    case (1): case (2):
                        // info on local physical DKC
                        physical_product = printableAsDot(E2_sub_2_GAD_buf+8,16);
                        physical_product_revision = printableAsDot(E2_sub_2_GAD_buf+24,4);
                        physical_serial = decode_hex_serial_number(E2_sub_2_GAD_buf+28);
                        physical_LDEV.push_back(E2_sub_2_GAD_buf[28+8]);
                        physical_LDEV.push_back(E2_sub_2_GAD_buf[28+8+1]);
                        physical_LDEV.push_back(':');
                        physical_LDEV.push_back(E2_sub_2_GAD_buf[28+8+2]);
                        physical_LDEV.push_back(E2_sub_2_GAD_buf[28+8+3]);

                        switch(E2_sub_2_GAD_buf[40+2])
                        {
                        case 0x01:
                            physical_LDEV_type="Internal";
                            physical_RAID_level="RAID-1";
                            pgstr << ((unsigned int) E2_sub_2_GAD_buf[40]) << '-' << ((unsigned int)E2_sub_2_GAD_buf[40+1]);
                            physical_PG=pgstr.str();
                            break;
                        case 0x05:
                            physical_LDEV_type="Internal";
                            physical_RAID_level="RAID-5";
                            pgstr << ((unsigned int) E2_sub_2_GAD_buf[40]) << '-' << ((unsigned int)E2_sub_2_GAD_buf[40+1]);
                            physical_PG=pgstr.str();
                            break;
                        case 0x06:
                            physical_LDEV_type="Internal";
                            physical_RAID_level="RAID-6";
                            pgstr << ((unsigned int) E2_sub_2_GAD_buf[40]) << '-' << ((unsigned int)E2_sub_2_GAD_buf[40+1]);
                            physical_PG=pgstr.str();
                            break;
                        case 0x10:
                            physical_LDEV_type="External";
                            break;
                        case 0x12:
                            physical_LDEV_type="DP-Vol";
                            pgstr << ((((unsigned int)E2_sub_2_GAD_buf[40]) << 8) + ((unsigned int)E2_sub_2_GAD_buf[40+1]));
                            physical_Pool_ID = pgstr.str();
                            break;
                        default:;
                        }

                        {
                            int nickname_length=0;
                            for (unsigned char* p = E2_sub_2_GAD_buf+40; (*p)!=0 && (nickname_length<31); p++) nickname_length++;
                            if (nickname_length>0) physical_nickname = printableAsDot(E2_sub_2_GAD_buf+40, nickname_length);
                        }

                        physical_SSID = std::string("0x")              + printAsHex(E2_sub_2_GAD_buf+40+2+1    ,2);
                        physical_CU = std::string("0x")                + printAsHex(E2_sub_2_GAD_buf+40+2+1+2  ,1);
                        physical_consistency_group = std::string("0x") + printAsHex(E2_sub_2_GAD_buf+40+2+1+2+1,1);

                        physical_product_ID = printableAsDot(E2_sub_2_GAD_buf+184,4);
                        physical_submodel = std::string("0x") + printAsHex(E2_sub_2_GAD_buf+183,1);
                    default:;
                }
            }

            if (TC_volstat.size() > 0 && TC_volstat != std::string("SMPL") )
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("TC ");
                notes += TC_volstat;
            }

            if (ProductUnprintableAsDot == std::string("OPEN-0V         "))
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("TI SVOL");
            }

            if (SI_M0_volstat.size() > 0 && SI_M0_volstat != std::string("SMPL") )
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("MRCF0 ");
                notes += SI_M0_volstat;
            }

            if (SI_M1_volstat.size() > 0 && SI_M1_volstat != std::string("SMPL") )
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("MRCF1 ");
                notes += SI_M1_volstat;
            }

            if (SI_M2_volstat.size() > 0 && SI_M2_volstat != std::string("SMPL") )
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("MRCF2 ");
                notes += SI_M2_volstat;
            }

            if (UR_M0_volstat.size() > 0 && UR_M0_volstat != std::string("SMPL") && TC_volstat != UR_M0_volstat)
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("UR0 ");
                notes += UR_M0_volstat;
            }

            if (UR_M1_volstat.size() > 0 && UR_M1_volstat != std::string("SMPL") )
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("UR1 ");
                notes += UR_M1_volstat;
            }

            if (UR_M2_volstat.size() > 0 && UR_M2_volstat != std::string("SMPL") )
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("UR2 ");
                notes += UR_M2_volstat;
            }

            if (UR_M3_volstat.size() > 0 && UR_M3_volstat != std::string("SMPL") )
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += std::string("UR3 ");
                notes += UR_M3_volstat;
            }

            if (HUVM_role.size() > 0)
            {
                if (notes.size() > 0 ) notes += " + ";
                notes += HUVM_role;
            }
        }
        else
        {
            HitachiProduct="Unknown Hitachi Product";
            HDSProduct="Unknown Hitachi product";
            SerialNumber="";
        }
    }

    if (LDEV_type!=CMD_DEV)
    {

        // find the size
        uint64_t lastgoodsector, lastbadsector, trynextsector;
        void* p_sector_buffer = memalign(BYTES_PER_SECTOR,BYTES_PER_SECTOR);

        rc=pread64(fd,p_sector_buffer,BYTES_PER_SECTOR,1*BYTES_PER_SECTOR);  // Read sector number 1 (not sector number 0)

        if (rc!=BYTES_PER_SECTOR)
        {
            errmsg = "LUN_discovery::LUN_discovery(,\"" + LUNname + "\") - pread64() for sector 1 failed - " + std::string(strerror(errno));
            constructor_log = constructor_log + errmsg + '\n';
            return;
        }
        // expansion phase
        lastgoodsector=1;
        while (true)
        {
            trynextsector=2*lastgoodsector;
            rc=pread64(fd,p_sector_buffer,BYTES_PER_SECTOR,trynextsector*BYTES_PER_SECTOR);
            if (rc!=BYTES_PER_SECTOR)
            {
                lastbadsector=trynextsector;
                break;
            }
            else
            {
                lastgoodsector=trynextsector;
            }
        }
        // contraction phase
        while (true)
        {
            if ((lastgoodsector+1)==lastbadsector) break;
            trynextsector=(lastgoodsector+lastbadsector)/2;
            rc=pread64(fd,p_sector_buffer,BYTES_PER_SECTOR,trynextsector*BYTES_PER_SECTOR);
            if (rc!=BYTES_PER_SECTOR)
            {
                lastbadsector=trynextsector;
            }
            else
            {
                lastgoodsector=trynextsector;
            }
        }

        maxLBA=lastgoodsector;

        if (maxLBA > 0)
        {
            SizeMB = ((long double) (1 + maxLBA)) * ((long double)BYTES_PER_SECTOR) / ((long double)1E6);
            SizeMiB = ((long double) (1 + maxLBA)) * ((long double)BYTES_PER_SECTOR) / ((long double)1024*1024);

            SizeGB = ((long double) (1 + maxLBA)) * ((long double)BYTES_PER_SECTOR) / ((long double)1E9);
            SizeGiB = ((long double) (1 + maxLBA)) * ((long double)BYTES_PER_SECTOR) / ((long double)1024*1024*1024);

            SizeTB = ((long double) (1 + maxLBA)) * ((long double)BYTES_PER_SECTOR) / ((long double)1E12);
            SizeTiB = ((long double) (1 + maxLBA)) * ((long double)BYTES_PER_SECTOR) / ((long double)1024*1024*1024*1024);
        }
    }
    constructor_log = constructor_log + printheaders() + '\n' + printdata() + '\n';

    close(fd);
    return;
}

bool LUN_discovery::get_page(unsigned char* data_buffer, int data_buffer_size, unsigned char* SI_cmd, std::string description, std::string* sense_bytes)
{
    std::string errmsg;
    *sense_bytes = std::string("");

    memset(&scsi_header, 0, sizeof(sg_io_hdr_t));
    memset(data_buffer, 0, data_buffer_size);
    scsi_header.interface_id = 'S';
    scsi_header.cmd_len = 6;
    scsi_header.mx_sb_len = sizeof(sense_buffer);
    scsi_header.dxfer_direction = SG_DXFER_FROM_DEV;
    scsi_header.dxfer_len = data_buffer_size;
    scsi_header.dxferp = data_buffer;
    scsi_header.cmdp = SI_cmd;
    scsi_header.sbp = sense_buffer;
    scsi_header.timeout = 10000; // timeout 10 seconds
    vpd_ioctl_return_code = ioctl(fd, SG_IO, &scsi_header);

    if (vpd_ioctl_return_code < 0)
    {
        std::ostringstream msg;
        msg << "LUN_discovery(\"" << LUNname << "\") - SCSI Inquiry for " << description << " failed - SG_IO ioctl error.  Errno=" << errno << " (" << strerror(errno) << ")";
        *sense_bytes=msg.str();
        constructor_log += msg.str() + '\n';
        return false;
    }

    /* now for the error processing */
    if ((scsi_header.info & SG_INFO_OK_MASK) != SG_INFO_OK)
    {
        std::ostringstream msg;
        msg << "LUN_discovery(\"" << LUNname << "\") - SCSI Inquiry for " << description <<  ":" << std::endl;
        sense_bytes_received=scsi_header.sb_len_wr;
        if (scsi_header.masked_status || scsi_header.host_status || scsi_header.driver_status)
        {
            msg << "     "
                << "SCSI status=0x" << std::hex << std::setw(2) << std::setfill('0') << ((unsigned int)scsi_header.status)
                << ", host_status=0x" << std::hex << std::setw(2) << std::setfill('0') << ((unsigned int)scsi_header.host_status)
                << ", driver_status=0x" << std::hex << std::setw(2) << std::setfill('0') << ((unsigned int)scsi_header.driver_status)
                << std::endl;
        }
        if (sense_bytes_received > 0)
        {
            msg << "     Sense bytes = " << printableAndHex((unsigned char*)&sense_buffer, sense_bytes_received) << std::endl;
        }
        constructor_log += msg.str();
        *sense_bytes = msg.str();
        return false;
    }
    return true;
}

std::string decode_hex_serial_number(const unsigned char* p)
{
    if ( ((*p) != '5') || ((*(p+1)) != '0') )
    {
        std::ostringstream o;
        o << "<Error> LUN_discovery - Invalid RAID subsystem 8 hex character format serial number " + printableAndHex(p,8) + " - "
            << "did not start with \"50\"."
            << "  Problem occured at line " << __LINE__ << " of " << __FILE__ << std::endl;
        throw  std::runtime_error(o.str());
    }

    unsigned int s {0};

    for (unsigned int i = 3; i <= 7; i++)
    {
        s = s << 4;
        switch (*(p+i))
        {
            case '0': s = s + 0; break;
            case '1': s = s + 1; break;
            case '2': s = s + 2; break;
            case '3': s = s + 3; break;
            case '4': s = s + 4; break;
            case '5': s = s + 5; break;
            case '6': s = s + 6; break;
            case '7': s = s + 7; break;
            case '8': s = s + 8; break;
            case '9': s = s + 9; break;
            case 'a':
            case 'A': s = s + 10; break;
            case 'b':
            case 'B': s = s + 11; break;
            case 'c':
            case 'C': s = s + 12; break;
            case 'd':
            case 'D': s = s + 13; break;
            case 'e':
            case 'E': s = s + 14; break;
            case 'f':
            case 'F': s = s + 15; break;
            default:
            {
                std::ostringstream o;
                o << "<Error> LUN_discovery - Invalid RAID subsystem 8 hex character format serial number " + printableAndHex(p,8) + " - "
                    << "character number " << i << " (starting from zero) was not an ASCII representation of a hex digit."
                    << "  Problem occured at line " << __LINE__ << " of " << __FILE__ << std::endl;
                throw  std::runtime_error(o.str());
            }
        }
    }

    switch (*(p+2))
    {
        case ' ': break;               // RAID700 or earlier
        case '2': s += 200000; break;  // RAID800
        case '3': s += 300000; break;  // HM800
        case '4': s += 400000; break;
        case '5': s += 500000; break;
        case '6': s += 600000; break;
    }
    {
        std::ostringstream o;
        o << s;
        return (o.str());
    }
}
