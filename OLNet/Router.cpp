/*
 * Router.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: cs3516
 */

#include <iostream>
#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>
#include "LPMTree.h"
#include "Router.h"
#include "Packet.h"
#include "cs3516sock.h"

using namespace std;

#define VERBOSE (0)

int runRouter(char* configFile) {

	LPMTree* tree = new LPMTree();
	tree->insert(7, (uint32_t)0x30000000, 4);
	tree->insert(3, (uint32_t)0x80000000, 2);
	tree->insert(6, (uint32_t)0x70000000, 4);
	tree->insert(2, (uint32_t)0x40000000, 2);
	cout.flush();
	//tree->print();

	cout << "This should be a '7': " << tree->get((uint32_t)0x30000000, 4) << endl;


	cout << "---------- Router Mode Started" << endl;

	int sock = create_cs3516_socket();
	char* buf = new char[MAX_PACKET_SIZE];
	packet* p;
	char* data;

	while(1){
		bzero(buf, MAX_PACKET_SIZE);
		int recv = cs3516_recv(sock, buf, MAX_PACKET_SIZE);
		p = (packet*)buf;
		data = (char*)(p + sizeof(packethdr));

		if(VERBOSE){
			printPacket(p);
		} else {
			cout << "Routing packet of size " << recv << " bytes." << endl;
		}

//		unsigned int addr = 0;
//		inet_pton(AF_INET, "192.168.132.165", &addr);
		int send = cs3516_send(sock, buf, recv, p->header.ip_header.ip_dst.s_addr);



	}
	////////////////////////////////////

//	char* testWord = (char*) "0123456789";
//
//	packet* p = new packet;
//	p->header.ip_header.ip_ttl = 12;
//	p->header.ip_header.ip_v = 4;
//	p->header.ip_header.ip_hl = 5;
//
//
//	if (p->header.ip_header.ip_ttl != 0) {
//		p->header.ip_header.ip_ttl -= 1;
//	} else {
//		dropPacket(p);
//	}
//	p->data = testWord;
//	p->header.udp_header.len = strlen(p->data) + 1;
//	cout << endl;
//
//	unsigned int addr = 0;
//	inet_pton(AF_INET, "192.168.132.164", &addr);
//
//	int sent = sendPacket(sock, p, addr);
//
//	cout << "Sent: " << sent << " bytes" << endl;

	return 0;
}


void dropPacket(packet* p) {
	cout << "DROPPING PACKET" << endl;
	printPacket(p);
}



