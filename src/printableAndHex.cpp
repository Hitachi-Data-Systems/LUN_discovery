//Copyright (c) 2016, 2017, 2018 Hitachi Vantara Corporation
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
//Authors: Allart Ian Vogelesang <ian.vogelesang@hitachivantara.com>, Kumaran Subramaniam <kumaran.subramaniam@hitachivantara.com>
//
//Support:  "ivy" is not officially supported by Hitachi Vantara.
//          Contact one of the authors by email and as time permits, we'll help on a best efforts basis.

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

#include "../include/printableAndHex.h"

std::string printableAsDot (const unsigned char* p, int length) {
	// AIV 2013-04-24
	const unsigned char* q = p;
	std::stringstream os;
	for (int i=0;i<length;i++){
		if (isprint(*q)) os << *q;
		else os <<'.';
		q++;
	}
	return os.str();
}

std::string printAsHex (const unsigned char* p, int length){
	const unsigned char* q = p;
	std::ostringstream os;
		q=p;
		for (int i=0;i<length;i++){
			os << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)*q;
			q++;
			if ((i%4 == 3) && (i != (length-1))) os << ' ';
		}
	return os.str();
}

void display_memory_contents(ostream& o, const unsigned char*p, int length, int perlinemax, std::string eachlineprefix) {
	// AIV 2013-04-30
	if (length<=perlinemax) {
		o << eachlineprefix << " \"" << printableAsDot(p, length) << "\" (" << printAsHex(p, length) << ')';
	} else {
	// print a prefix string before each line, along with the offset from the start in hex and decimal.
		int current_offset=0;
		int bytes_to_go = length;
		int bytes_this_round;
		const unsigned char* q=p;
		while (bytes_to_go>0) {
			if (bytes_to_go > perlinemax) bytes_this_round = perlinemax;
			else bytes_this_round=bytes_to_go;
			o << eachlineprefix << "offset 0x" << std::hex << current_offset
			  << " (" << std::dec << current_offset << ")"
			  << " \"" << printableAsDot(q, bytes_this_round) << "\" ("
			  << printAsHex(q, bytes_this_round) << ')' << std::endl;
			q=q+bytes_this_round;
			current_offset+=bytes_this_round;
			bytes_to_go-=bytes_this_round;
		}
	}
}

std::string printableAndHex(const unsigned char*p, int length) {
	return std::string("\"") + printableAsDot(p,length) + std::string("\" (") + printAsHex(p,length) + std::string(")");
}
