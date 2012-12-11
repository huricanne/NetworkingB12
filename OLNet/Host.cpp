/*
 * Host.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: cs3516
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <cerrno>
#include <fstream>
#include <signal.h>
#include <sstream>

#include "Host.h"
#include "Packet.h"
//#include "Globals.h"
#include "cs3516sock.h"
#include "FileSender.h"

using namespace std;

int id = 0;
FILE* rFile;
ofstream stats;
int hostSock = -1;

static void terminate(int arg) {
	cout << endl << "Terminating host... " << endl;

	cout << "Closing receive file... ";
	if (rFile != NULL)
		fclose(rFile);
	cout << "Done." << endl;

	cout << "Closing receive file... ";
	stats.close();
	cout << "Done." << endl;

	cout << "Closing socket... ";
	close(hostSock);
	cout << "Done." << endl;

	exit(1);
}

int runHost(in_addr* _ip, int deviceID, int ttl, map<int, uint32_t>* idToRealIP,
		map<int, uint32_t>* idToOverlayIP, map<int, int>* hostToRouter,
		map<int, map<int, int> >* delayList) {
	char* file = "send_body";
	in_addr ip;
	memcpy(&ip, _ip, sizeof(in_addr));

	////////////////////////////////
	// DO NOT TOUCH THESE MEMCPYs //
	////////////////////////////////
	int routerID = (*hostToRouter)[deviceID];

	in_addr routerBinIP;
	routerBinIP.s_addr = (*idToRealIP)[routerID];
	char* routerRealIPString = (char*) malloc(16);
	memcpy(routerRealIPString, inet_ntoa(routerBinIP), 16);
	//temp3[15] = '\0';

	char* routerOverlayIPString = (char*) malloc(16);
	in_addr tmp;
	tmp.s_addr = (*idToOverlayIP)[routerID];
	memcpy(routerOverlayIPString, inet_ntoa(tmp), 16);
	//routerOverlayIPString[15] = '\0';

	char* hostOverlayIPString = (char*) malloc(16);
	tmp.s_addr = (*idToOverlayIP)[deviceID];
	memcpy(hostOverlayIPString, inet_ntoa(tmp), 16);
	//hostOverlayIPString[15] = '\0';

	char* hostRealIPString = (char*) malloc(16);
	memcpy(hostRealIPString, inet_ntoa(ip), 16);
	//temp1[15] = '\0';
	////////////////////////////////
	// DO NOT TOUCH THESE MEMCPYs //
	////////////////////////////////

	cout << "---------- Host Mode Started " << endl;
	cout << "File:       " << file << endl;
	cout << "Host ID:    " << deviceID << endl;
	cout << "Host IP:    " << hostRealIPString << endl;
	cout << "Overlay IP: " << hostOverlayIPString << endl;
	cout << "Router ID:  " << routerID << endl;
	cout << "Router IP:  " << routerRealIPString << endl;
	cout << "Overlay IP: " << routerOverlayIPString << endl;

	bool sending = true;

	signal(SIGINT, terminate);
	signal(SIGABRT, terminate);
	signal(SIGTERM, terminate);

	hostSock = create_cs3516_socket();
	//cout << hostSock << endl;

	ifstream fd("send_config.txt");
	if (!fd.good())
		sending = false;
	char* configBuf = new char[100];
	fd.getline(configBuf, 100);

	string line(configBuf);
	string param;
	string lineStr(line);
	stringstream lineStream(line);

	lineStream >> param;
	string destIP(param.c_str());
	char* destRealIPString = (char*) malloc(16);
	memcpy(destRealIPString, destIP.c_str(), 16);

	lineStream >> param;
	int sourcePort = atoi(param.c_str());

	lineStream >> param;
	int destPort = atoi(param.c_str());
	fd.close();

	//////////////////////////////// Select

	timeval t { 0 };
	t.tv_usec = 10;
	fd_set set;
	FD_SET(hostSock, &set);

	timeval t2;
	fd_set set2;

	if (sending) {
		//(char* file, int delay, char* hostOverlayIPString, char* destRealIPString, char* routerRealIPString, int ttl, int sourcePort, int destPort, int deviceID, int routerID)
		data d;
		d.delay = (*delayList)[deviceID][routerID];
		d.destPort = destPort;
		d.destRealIPString = destRealIPString;
		d.deviceID = deviceID;
		d.file = file;
		d.hostOverlayIPString = hostOverlayIPString;
		d.id = &id;
		d.routerID = routerID;
		d.routerRealIPString = routerRealIPString;
		d.sock = hostSock;
		d.sourcePort = sourcePort;
		d.ttl = ttl;

		FileSender* sender = new FileSender(d);
		sender->send();
	} else {
		cout << "No file specified. Waiting for data." << endl;
	}

	cout << "Opening receive file... ";
	rFile = fopen("received", "a+");
	if (rFile == NULL) {
		cout << "Receive file could not be opened. Exiting." << endl;
		abort();
	}
	cout << "Done." << endl;

	cout << "Opening receive_stats file... ";
	stats.open("received_stats.txt", ifstream::out | ifstream::app);
	if (!stats.good()) {
		cout << "Receive Stats file could not be opened. Exiting." << endl;
		abort();
	}
	cout << "Done." << endl;

	char* buf = (char*) malloc(MAX_PACKET_SIZE);
	cs3516_recv(hostSock, buf, MAX_PACKET_SIZE);
	int packetCount = 0;
	while (1) {

		buf = (char*) realloc(buf, MAX_PACKET_SIZE);
		memset(buf, 0, MAX_PACKET_SIZE);

		int recv = cs3516_recv(hostSock, buf, MAX_PACKET_SIZE);
		packetCount++;

		cout << "Recv: " << recv << " bytes" << endl;

		buf = (char*) realloc(buf, recv);

		packethdr* p = ((packethdr*) buf);
		char* data = (char*) (buf + sizeof(packethdr));

		//printPacket(p);
		cout << "Recieved " << packetCount << " packets." << endl;

		//printPacket(p);

		stats << inet_ntoa(p->ip_header.ip_src);
		stats << " " << inet_ntoa(p->ip_header.ip_dst);
		stats << " " << p->udp_header.source;
		stats << " " << p->udp_header.dest << endl;

		usleep(50);

		for (int i = 0; i < p->udp_header.len; i++) {
			fputc(data[i], rFile);
		}

		//char* data = buf + sizeof(packethdr);
		//cout << data << endl;
	}
	return 0;
}

int getFileSize(FILE* fd) {
	int size = -1;
	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	return size;
}
