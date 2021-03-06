/* fileSharer.c - fileSharer */
/* Copyright (C) 2007, Marquette University.  All rights reserved. */

#include <kernel.h>
#include <xc.h>
#include <file.h>
#include <fileshare.h>
#include <ether.h>
#include <network.h>
#include <nvram.h>

struct fishnode school[SCHOOLMAX];
char   fishlist[DIRENTRIES][FNAMLEN];

static uchar bcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uchar myMAC[ETH_ADDR_LEN];

int fishAnnounce(uchar *packet)
{
	struct ethergram *eg = (struct ethergram *)packet;
	int i = 0;

	for (i = 0; i < SCHOOLMAX; i++)
	{
		/* Check to see if node already known. */
		if (school[i].used && 
			(0 == memcmp(school[i].mac, eg->src, ETH_ADDR_LEN)))
			return OK;
	}
	for (i = 0; i < SCHOOLMAX; i++)
	{
		/* Else find an unused slot and store it. */
		if (!school[i].used)
		{
			school[i].used = 1;
			memcpy(school[i].mac, eg->src, ETH_ADDR_LEN);
			memcpy(school[i].name, eg->data + 1, FISH_MAXNAME);
			//printf("Node %s\n", school[i].name);
			return OK;
		}
	}
	return SYSERR;
}

/*------------------------------------------------------------------------
 * fishPing - Reply to a broadcast FISH request.
 *------------------------------------------------------------------------
 */
void fishPing(uchar *packet)
{
	uchar *ppkt = packet;
	struct ethergram *eg = (struct ethergram *)packet;

	/* Source of request becomes destination of reply. */
	memcpy(eg->dst, eg->src, ETH_ADDR_LEN);
	/* Source of reply becomes me. */
	memcpy(eg->src, myMAC, ETH_ADDR_LEN);
	/* Zero out payload. */
	bzero(eg->data, ETHER_MINPAYLOAD);
	/* FISH type becomes ANNOUNCE. */
	eg->data[0] = FISH_ANNOUNCE;
	strncpy(&eg->data[1], nvramGet("hostname\0"), FISH_MAXNAME-1);
	write(ETH0, packet, ETHER_SIZE + ETHER_MINPAYLOAD);
}
/*------------------------------------------------------------------------
 * dir ask - Reply to a fish DIRASK request.
 * should reply to a fish DIRASK with a DIRLIST?
 *------------------------------------------------------------------------
 */
void fishAsk(uchar *packet)
{
	uchar *ppkt = packet;
	struct ethergram *eg = (struct ethergram *)packet;
	/* Source of request becomes destination of reply. */
	memcpy(eg->dst, eg->src, ETH_ADDR_LEN);
	/* Source of reply becomes me. */
	memcpy(eg->src, myMAC, ETH_ADDR_LEN);
	/* Zero out payload. */
	bzero(eg->data, ETHER_MINPAYLOAD);
	/* FISH type becomes DIRLIST */
	eg->data[0] = FISH_DIRLIST;
	struct filenode *tempNode = supertab->sb_dirlst->db_fnodes;
	for(int i = 0;i<DIRENTRIES;i++){
		ppkt+=FNAMLEN;
		if(tempNode[i].fn_state){
			strncpy(&eg->data[1+i*FNAMLEN],tempNode[i].fn_name, FNAMLEN);
		}
		else{
			for(int j=0;j<FNAMLEN;j++){
				eg->data[j+1+i*FNAMLEN]=0;	
			}
		}
	}
	int packetSize;
	if((ppkt-packet)>(ETHER_SIZE+ETHER_MINPAYLOAD)){
		packetSize = ppkt - packet;
	}
	else{
		packetSize = ETHER_SIZE + ETHER_MINPAYLOAD;
	}
	write(ETH0, packet, packetSize);
}
/*------------------------------------------------------------------------
 * fishList - Reply to a fish list request.
 * Implement DIRLIST via the filetab
 *------------------------------------------------------------------------
 */
int fishList(uchar *packet)
{
	struct ethergram *eg = (struct ethergram *)packet;
/*	for(int c=0;c<DIRENTRIES;c++){
		for(int b =0;b<FISH_MAXNAME;b++){
			fishlist[c][b]=NULL;
		}
	}*/
	/*Move dir entries into fishlist*/
	for(int i = 0; i<DIRENTRIES;i++){
		int offset = 1 + (i*FNAMLEN);
		strncpy(fishlist[i],&eg->data[offset], FNAMLEN);
	}
	return OK;
}
/*------------------------------------------------------------------------
 * fish get file
 *------------------------------------------------------------------------
 */
void fishGet(uchar *packet)
{
	uchar *ppkt = packet;
	struct ethergram *eg = (struct ethergram *)packet;
	/* Source of request becomes destination of reply. */
	memcpy(eg->dst, eg->src, ETH_ADDR_LEN);
	/* Source of reply becomes me. */
	memcpy(eg->src, myMAC, ETH_ADDR_LEN);
	/* Zero out payload. */
//	bzero(eg->data, DISKBLOCKLEN);
	int fileFound = 0;
	struct filenode *tempNode = supertab->sb_dirlst->db_fnodes;
	for(int i = 0;i<DIRENTRIES;i++){
		if(tempNode[i].fn_state){
			if(strncmp(tempNode[i].fn_name,&eg->data[1], FNAMLEN)==0){
				printf("sending have file\n\r");
				eg->data[0]=FISH_HAVEFILE;
				int payloadSize = DISKBLOCKLEN+FNAMLEN+1;
				int fd = fileOpen(&eg->data[1]);
				int i=FNAMLEN+1;
				char temp;
				while(i<payloadSize){
					if((temp=fileGetChar(fd)) != SYSERR){
						eg->data[i] = temp;
					}
					else{
						eg->data[i]=0;
					}
					i++;
				}
				fileClose(fd);
				write(ETH0, packet, payloadSize);
				return;
			}
		}
	}
	printf("sending nofile\n\r");
	eg->data[0]=FISH_NOFILE;
	write(ETH0, packet, ETHER_MINPAYLOAD+ETHER_SIZE);
}
/*------------------------------------------------------------------------
 * fish have file
 *------------------------------------------------------------------------
 */
void fishHave(uchar *packet)
{
	struct ethergram *eg = (struct ethergram *)packet;
	int packetSize = ETHER_SIZE + DISKBLOCKLEN + FNAMLEN + 1;
	int i, fd;
	char temp[FNAMLEN + 1];
	bzero(temp, FNAMLEN+1);
	memcpy(temp,&eg->data[1],FNAMLEN+1);
	if((fd=fileOpen(temp))!= SYSERR){
		if(fileDelete(fd) == SYSERR){
			printf("ERROR");
		}
	}
	if((fd=fileCreate(temp)) != SYSERR){
		for(i=FNAMLEN+1;i<(DISKBLOCKLEN+FNAMLEN+1);i++){
			filePutChar(fd,eg->data[i]);
		}
		fileClose(fd);
		printf("File Created \n\r");
	}
	else{
		printf("Unable to make file");
	}
}

void fishNoFile(uchar *packet){
	printf("File does not exist\n\r");
}


/*------------------------------------------------------------------------
 * fileSharer - Process that shares files over the network.
 *------------------------------------------------------------------------
 */
int fileSharer(int dev)
{
	uchar packet[PKTSZ];
	int size;
	struct ethergram *eg = (struct ethergram *)packet;

	enable();
	/* Zero out the packet buffer. */
	bzero(packet, PKTSZ);
	bzero(school, sizeof(school));
	bzero(fishlist, sizeof(fishlist));

	/* Lookup canonical MAC in NVRAM, and store in ether struct */
 	colon2mac(nvramGet("et0macaddr"), myMAC);


	while (SYSERR != (size = read(dev, packet, PKTSZ)))
	{
		/* Check packet to see if fileshare type with
		   destination broadcast or me. */
		if ((ntohs(eg->type) == ETYPE_FISH)
			&& ((0 == memcmp(eg->dst, bcast, ETH_ADDR_LEN))
				|| (0 == memcmp(eg->dst, myMAC, ETH_ADDR_LEN))))
		{
			switch (eg->data[0])
			{
			case FISH_ANNOUNCE:
				fishAnnounce(packet);
				break;

			case FISH_PING:
				fishPing(packet);
				break;

			case FISH_DIRASK:
				fishAsk(packet);			
				break;	

			case FISH_DIRLIST:
				fishList(packet);
				break;

			case FISH_GETFILE:
				fishGet(packet);
				break;

			case FISH_HAVEFILE:
				fishHave(packet);
				break;

			case FISH_NOFILE:
				fishNoFile(packet);
				break;

			default:
				printf("ERROR: Got unhandled FISH type %d\n", eg->data[0]);
			}
		}
	}

	return OK;
}
