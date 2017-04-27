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


//#define DEBUG ON


void LUN_discovery::printheaders(ostream& o)
{
    o << printheaders() << std::endl;
}

std::string LUN_discovery::printheaders()
{
    return std::string("hostname,SCSI Bus Number (HBA),LUN Name,Hitachi Product,HDS Product,Serial Number,Port,LUN 0-255,LDEV,Nickname,LDEV type,RAID level,Parity Group,Pool ID,CLPR,Max LBA")
           + std::string(",Size MB,Size MiB,Size GB,Size GiB,Size TB,Size TiB")
           + std::string(",Vendor,Product,Product Revision,SCSI_IOCTL_PROBE_HOST");
}

void LUN_discovery::printdata(ostream& o)
{
    o << printdata() << std::endl;
}

std::string LUN_discovery::printdata()
{
    ostringstream os;
    os << hostname << ',' << bus_number << ',' << LUNname << ',' << HitachiProduct << ',' << HDSProduct << ',' << SerialNumber
       << ',' << Port << ',' << LU_Number << ',' << LDEV << ',' << nickname << ','<< LDEV_type << ',' << RAIDlevel << ','
       << PG << ',' << poolID << ',' << CLPR << ',' << std::fixed << maxLBA
       << ',' << SizeMB << ',' << SizeMiB << ',' << SizeGB << ',' << SizeGiB << ',' << SizeTB << ',' << SizeTiB
       //<< ',' << ((openROgood) ? (openRWgood) ? std::string("O_RDWR") : std::string("O_RDONLY") : std::string("") )
       << ',' << VendorUnprintableAsDot << ',' << ProductUnprintableAsDot << ',' << ProductRevisionUnprintableAsDot
       //<< ',' << HitachiVPD
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

    //if (haveVpdPage) { display_memory_contents(o, vpd_buf, 4+vpd_buf[3], 16,"Page 0x83 with EVPD=1: "); o << std::endl; }
    if (haveVpdPage)
    {
        display_memory_contents(o, vpd_buf, sizeof(vpd_buf), 16,"Page 0x83 with EVPD=1: ");
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
        display_memory_contents(o, (unsigned char*)&E0_page, E0_page.additional_length, 16,"Page 0xE0 with EVPD=: ");
        o << std::endl;
    }
    if (page_E0_sense_bytes.length()>0)
    {
        o << "Page 0x83 with EVPD=1 - " <<page_E0_sense_bytes;
        o << std::endl;
    }

    if (have_E2_sub_0_page)
    {
        display_memory_contents(o, E2_sub_0_page, 4+E2_sub_0_page[3], 16,"Page 0xE2 subpage 0 [HAM]: ");
        o << std::endl;
    }
    if (page_E2_sub_0_sense_bytes.length()>0)
    {
        o << "Page 0xE2 subpage 0 [HAM] - " <<page_E2_sub_0_sense_bytes;
        o << std::endl;
    }

    if (have_E2_sub_1_page)
    {
        display_memory_contents(o, E2_sub_1_page, 4+E2_sub_1_page[3], 16,"Page 0xE2 subpage 1 [NDM]: ");
        o << std::endl;
    }
    if (page_E2_sub_1_sense_bytes.length()>0)
    {
        o << "Page 0xE2 subpage 1 [NDM] - " <<page_E2_sub_1_sense_bytes;
        o << std::endl;
    }

    if (have_E2_sub_2_page)
    {
        display_memory_contents(o, E2_sub_2_page, 4+E2_sub_2_page[3], 16,"Page 0xE2 subpage 2 [HUVM]: ");
        o << std::endl;
    }
    if (page_E2_sub_2_sense_bytes.length()>0)
    {
        o << "Page 0xE2 subpage 2 [HUVM] - " <<page_E2_sub_2_sense_bytes;
        o << std::endl;
    }


    o << "Constructor log:" << std::endl << constructor_log << std::endl;
}

LUN_discovery::LUN_discovery( std::string L) : LUNname(L)
{
    HitachiProduct=HDSProduct=SerialNumber=Port=LDEV=VendorUnprintableAsDot=ProductUnprintableAsDot=ProductRevisionUnprintableAsDot=VendorSpecificUnprintableAsDotWithHex
                                           =(std::string("<SCSI Inquiry failed for \"") + LUNname + std::string("\">"));

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

    HitachiVPD = VendorUnprintableAsDot = ProductUnprintableAsDot = ProductRevisionUnprintableAsDot = "<no data>";
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

#ifdef DEBUG
    std::cout << "VendorUnprintableAsDot=\"" + VendorUnprintableAsDot + "\"" << std::endl;
    std::cout << "ProductUnprintableAsDot=\"" + ProductUnprintableAsDot + "\"" << std::endl;
    std::cout << "std::string(PRODUCTOPENV)) =\"" + std::string(PRODUCTOPENV) + "\"" << std::endl;
    std::cout << "ProductRevisionUnprintableAsDot=\"" + ProductRevisionUnprintableAsDot + "\"" << std::endl;
    std::cout << "VendorSpecificUnprintableAsDotWithHex=\"" + VendorSpecificUnprintableAsDotWithHex + "\"" << std::endl;
#endif//DEBUG

    HitachiProduct=HDSProduct=SerialNumber=Port=LDEV=(std::string("<not Hitachi LUN>"));

    // now get Vital Product Data page 0x83
    haveVpdPage = get_page( (unsigned char*)&(vpd_buf[0]), sizeof(vpd_buf), vpd_cmd, "page 0x83 with EVPD=1", &vpd_sense_bytes);
    //haveVpdPage_EVPD_off = get_page( (unsigned char*)&(vpd_buf_EVPD_off[0]), sizeof(vpd_buf_EVPD_off), vpd_cmd_EVPD_off, "page 0x83 with EVPD=0", &vpd_sense_bytes_EVPD_off);

    ostringstream oHitachiVPD;
    display_memory_contents(oHitachiVPD, ((unsigned char*) vpd_buf)+38, 16, 16, "");
    HitachiVPD=oHitachiVPD.str();

    /* INQUIRY response is present */
    char * p = (char *)def_buf;
    unsigned char* q = (unsigned char*) p;


    if (VendorUnprintableAsDot==std::string(VENDORHITACHI))
    {
        // vendor is Hitachi
        isHitachi=true;
        HitachiProduct=HDSProduct=SerialNumber=Port=LDEV=(std::string("<Hitachi LUN \"") + LUNname + std::string("\" - not recognized Hitachi subsystem type>"));

        haveE0page = get_page( (unsigned char*)&E0_page, sizeof(E0_page), E0_cmd, "Hitachi-specific page 0xE0", &page_E0_sense_bytes);

        if (!haveE0page) return;

        E0_product   =  printableAsDot((unsigned char*)&E0_page.ProductID, sizeof(E0_page.ProductID));
        SerialNumber =  printableAsDot((unsigned char*)&E0_page.NodeSN,    sizeof(E0_page.NodeSN));

        have_E2_sub_0_page = get_page( E2_sub_0_page, sizeof(E2_sub_0_page), E2_sub_0_cmd, "0xE2 sub page 0 (HAM): ", &page_E2_sub_0_sense_bytes );
        have_E2_sub_1_page = get_page( E2_sub_1_page, sizeof(E2_sub_1_page), E2_sub_1_cmd, "0xE0 sub page 1 (NDM): ", &page_E2_sub_1_sense_bytes );
        have_E2_sub_2_page = get_page( E2_sub_2_page, sizeof(E2_sub_2_page), E2_sub_2_cmd, "0xE0 sub page 2 (HUVM): ", &page_E2_sub_2_sense_bytes );

#ifdef DEBUG
        std::cout << "sizeof EO page = " << sizeof(E0_page) << std::endl;
        std::cout << "E0 microcode version = " << printableAndHex((unsigned char*)E0_page.MicroVersion, sizeof(E0_page.MicroVersion)) << std::endl;
        std::cout << "E0 vendor ID = " << printableAndHex((unsigned char*)&E0_page.VendorID, sizeof(E0_page.VendorID)) << std::endl;
        std::cout << "E0 local ID = " << printableAndHex((unsigned char*)&E0_page.LocalID, sizeof(E0_page.LocalID)) << std::endl;
        std::cout << "E0 product ID = " << printableAndHex((unsigned char*)&E0_page.ProductID, sizeof(E0_page.ProductID)) << std::endl;
        std::cout << "E0 host group = " << printableAndHex((unsigned char*)&E0_page.HostGroup, sizeof(E0_page.HostGroup)) << std::endl;
        std::cout << "E0 node S/N = " << printableAndHex((unsigned char*)&E0_page.NodeSN, sizeof(E0_page.NodeSN)) << std::endl;
        std::cout << "E0 command dev = " << printableAndHex((unsigned char*)&E0_page.CmdDev, sizeof(E0_page.CmdDev)) << std::endl;
        std::cout << "E0 LU number (ascii of hex value) = " << printableAndHex((unsigned char*)&E0_page.LunNumberHexAsAscii, sizeof(E0_page.LunNumberHexAsAscii)) << std::endl;
        std::cout << "E0 port WWN = " << printableAndHex((unsigned char*)&E0_page.PortWWN, sizeof(E0_page.PortWWN)) << std::endl;
        std::cout << "E0 port type ('5') = " << printableAndHex((unsigned char*)&E0_page.PortType, sizeof(E0_page.PortType)) << std::endl;
        std::cout << "E0 port high = " << printableAndHex((unsigned char*)&E0_page.PortSerialNumber_high, sizeof(E0_page.PortSerialNumber_high)) << std::endl;
        std::cout << "E0 port low = " << printableAndHex((unsigned char*)&E0_page.PortSerialNumber_low, sizeof(E0_page.PortSerialNumber_low)) << std::endl;
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
        else
        {
            if ( (ProductUnprintableAsDot == std::string(PRODUCTDISKSUBSYSTEM))
                    || (ProductUnprintableAsDot == std::string(PRODUCTOPENV))
                    || (ProductUnprintableAsDot == std::string(PRODUCTOPENVCM)) )
            {

                // subsystem type RAID

                std::string E0hm8 {""};
                for (int i=0; i<3; i++)
                {
                    E0hm8.push_back(E0_page.ProductID[i]);
                }

                if ( 0==std::string("HM8").compare(E0hm8) )
                {
                    // HM800

                    // The old code below that worked for some earlier subsystem generations didn't work with a pre-GA HM800,
                    // and I could see by running "InquireAbout -showall /dev/sdxx" that the info we wanted was available
                    // in the default SCSI Inquiry data.

                    // So the idea was that if you keep a separate piece of code for each subsystem type,
                    // then one you make it work, it pretty much stays working as long as the subsystem type itself changes its behavour.

                    // Default SCSI Inquiry page is "def_buf", which when I wrote this showed as 255 bytes long.

                    HitachiProduct="HM800";
                    HDSProduct="VSP Gx00";
                    LDEV = LDEV_no_colon.substr(0,2) + std::string(":") + LDEV_no_colon.substr(2,2);

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
                    case 0x12:
                        LDEV_type="DP-Vol";
                        RAIDlevel="DP-Vol";
                        pgstr << ((((int)def_buf[96]) << 8) + ((int)def_buf[97]));
                        poolID = pgstr.str();
                        break;
                    default:
                        pgstr << "RAID subsystem - unknown RAID level = 0x" << std::hex << std::setw(2) << std::setfill('0') << def_buf[96+2] << std::endl;
                        constructor_log += pgstr.str();
                    }


                    {
                        std::ostringstream sn;
                        sn << '4';
                        for (unsigned int i = 3; i < sizeof(E0_page.NodeSN); i++)  // NodeSN for serial number 410116, NodeSN contained "00010116"
                        {
                            sn << E0_page.NodeSN[i];
                        }
                        SerialNumber = sn.str();
                    }

                    int nickname_length=0;
                    for (unsigned char* p = &(def_buf[212]); (*p)!=0 && (nickname_length<31); p++) nickname_length++;
                    if (nickname_length>0) nickname = printableAsDot((unsigned char*)&(def_buf[212]), nickname_length);

                    {
                        std::ostringstream o;
                        o << "CLPR" << ((int)def_buf[140+1]);
                        CLPR=o.str();
                    }

                    if (ProductUnprintableAsDot == std::string(PRODUCTOPENVCM))
                    {
                        LDEV_type=CMD_DEV;
                    }
                }
                else	// not HM800
                {
                    Hitachi_RAID_SI_format* p_vpd = (Hitachi_RAID_SI_format*) &(vpd_buf[0]);

                    ostringstream pgstr;
                    ostringstream pg_no_hyphen_str;

                    switch (p_vpd->RAID_level)
                    {
                    case 0x01:
                        LDEV_type="Internal";
                        RAIDlevel="RAID-1";
                        pgstr << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_high) << '-' << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_low);
                        PG=pgstr.str();
                        pg_no_hyphen_str << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_high) << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_low);
                        PG_no_hyphen=pg_no_hyphen_str.str();
                        break;
                    case 0x05:
                        LDEV_type="Internal";
                        RAIDlevel="RAID-5";
                        pgstr << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_high) << '-' << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_low);
                        PG=pgstr.str();
                        pg_no_hyphen_str << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_high) << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_low);
                        PG_no_hyphen=pg_no_hyphen_str.str();
                        break;
                    case 0x06:
                        LDEV_type="Internal";
                        RAIDlevel="RAID-6";
                        pgstr << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_high) << '-' << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_low);
                        PG=pgstr.str();
                        pg_no_hyphen_str << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_high) << std::setw(2) << std::setfill('0') << ((int)p_vpd->location_low);
                        PG_no_hyphen=pg_no_hyphen_str.str();
                        break;
                    case 0x10:
                        LDEV_type="External";
                        pgstr << "0x" << std::hex << std::setw(4) << std::setfill('0') << (((p_vpd->location_high) << 8) + p_vpd->location_low);
                        PG=PG_no_hyphen=pgstr.str();
                        break;
                    case 0x12:
                        LDEV_type="DP-Vol";
                        pgstr << "pool=" << (((p_vpd->location_high) << 8) + p_vpd->location_low);
                        poolID = pgstr.str();
                        break;
                    default:
                        pgstr << "RAID subsystem - unknown RAID level = 0x" << std::hex << std::setw(2) << std::setfill('0') << p_vpd->RAID_level << std::endl;
                        constructor_log += pgstr.str();
                    }
                    pgstr.str("");
                    pgstr.clear();
                    pgstr << "CLPR" << (int)def_buf[140+1];
                    CLPR=pgstr.str();

                    unsigned int nickname_length=0;
                    for (char* p = &(p_vpd->LDEV_nickname[0]); (*p)!=0 && (nickname_length<sizeof(p_vpd->LDEV_nickname)); p++) nickname_length++;
                    if (nickname_length>0) nickname = printableAsDot((unsigned char*)&(p_vpd->LDEV_nickname[0]), nickname_length);

                    if (LDEV.length() == 4)
                    {
                        // might be an error message in there
                        LDEV=LDEV.substr(0,2) + ':' + LDEV.substr(2,2);
                    }


                    if (
                        (   (ProductUnprintableAsDot==std::string(PRODUCTOPENVCM))
                            || (ProductUnprintableAsDot==std::string(PRODUCTOPENV))
                        )
                        &&
                        (    ((vpd_buf[38+4])==0x06)
                             || ((vpd_buf[38+4])==0x16)
                        )
                    )
                    {

                        // VSP
                        // 00010203 04050607 08091011 12131415		Subsystem type	HM700 serial number part	Earlier RAID serial number	HM700 serial number part, decimal	Earlier RAID serial number, decimal
                        // 60060e80 1600f300 000100f3 00000009	"HITACHI ","OPEN-V-CM       ","5001"	16	100f3	00f3	65779	243

                        HitachiProduct="RAID700";
                        HDSProduct="VSP";
                        uint32_t RAIDserialnumber = (((uint32_t) (vpd_buf[38+9] % 16)) << 16) + (((uint32_t)vpd_buf[38+10]) << 8) + vpd_buf[38+11];
                        ostringstream o;
                        o << RAIDserialnumber;
                        SerialNumber=o.str();


                    }
                    else if (     (
                                      (ProductUnprintableAsDot==std::string(PRODUCTDISKSUBSYSTEM))
                                      || (ProductUnprintableAsDot==std::string(PRODUCTOPENVCM))
                                      || (ProductUnprintableAsDot==std::string(PRODUCTOPENV))
                                  )
                                  && ((vpd_buf[38+4])==0x13))
                    {
                        // HUS VM
                        // 00010203 04050607 08091011 12131415		Subsystem type	HM700 serial number part	Earlier RAID serial number	HM700 serial number part, decimal	Earlier RAID serial number, decimal
                        // 60060e80 13274800 50202748 0000ffff	"HITACHI ","DISK-SUBSYSTEM  ","5001"	13	02748	2748	10056	10056

                        HitachiProduct="HM700";
                        HDSProduct="HUS VM";
                        uint32_t RAIDserialnumber = 200000 + (((uint32_t) (vpd_buf[38+9] % 16)) << 16) + (((uint32_t)vpd_buf[38+10]) << 8) + vpd_buf[38+11];
                        ostringstream o;
                        o << RAIDserialnumber;
                        SerialNumber=o.str();

                    }
                    else if (E0_product == std::string("R800"))
                    {
                        HitachiProduct="RAID800";
                        HDSProduct="VSP G1000/F1500/G1500";
                        uint32_t RAIDserialnumber = 300000 + (((uint32_t) (vpd_buf[38+9] % 16)) << 16) + (((uint32_t)vpd_buf[38+10]) << 8) + vpd_buf[38+11];
                        ostringstream o;
                        o << RAIDserialnumber;
                        SerialNumber=o.str();
                    }
                    else
                    {
                        HitachiProduct="TBD";
                        HDSProduct="TBD";
                        SerialNumber="TBD";
                    }

                } // end not HM800 case

            }
            else
            {
                HitachiProduct="NotRecognized";
                HDSProduct="";
                SerialNumber="";
            }
        }
    }
    else
    {
        // vendor is not Hitachi
        HitachiProduct=HDSProduct=SerialNumber=Port=LDEV="";
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
                << "SCSI status=0x" << std::hex << std::setw(2) << std::setfill('0') << ((int)scsi_header.status)
                << ", host_status=0x" << std::hex << std::setw(2) << std::setfill('0') << ((int)scsi_header.host_status)
                << ", driver_status=0x" << std::hex << std::setw(2) << std::setfill('0') << ((int)scsi_header.driver_status)
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
