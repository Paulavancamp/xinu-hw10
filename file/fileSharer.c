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
	printf("In fishAsk function\r\n");
	/* Source of request becomes destination of reply. */
	memcpy(eg->dst, eg->src, ETH_ADDR_LEN);
	/* Source of reply becomes me. */
	memcpy(eg->src, myMAC, ETH_ADDR_LEN);
	/* Zero out payload. */
	bzero(eg->data, ETHER_MINPAYLOAD);
	/* FISH type becomes DIRLIST */
	printf("swapped addresses and zeroed payload\r\n");
	eg->data[0] = FISH_DIRLIST;
	printf("set fish type to dirlist\r\n");
	struct filenode *tempNode = supertab->sb_dirlst->db_fnodes;
	for(int i = 0;i<DIRENTRIES;i++){
		int offset = 1+ (i* (FNAMLEN+1));
		printf("%s\r\n",tempNode[i].fn_name);
		printf("%s\r\n",eg->data[i]);
		strncpy(&eg->data[offset],(tempNode[i].fn_name), MAXFILES);
		printf("%s\r\n",eg->data[i]);
	}
	write(ETH0, packet, ETHER_SIZE + ETHER_MINPAYLOAD);
}
/*------------------------------------------------------------------------
 * fishList - Reply to a fish list request.
 * Implement DIRLIST via the filetab
 *------------------------------------------------------------------------
 */
int fishList(uchar *packet)
{
	struct ethergram *eg = (struct ethergram *)packet;
	printf("In fishList func\r\n");
	/* Source of request becomes destination of reply. */
	memcpy(eg->dst, eg->src, ETH_ADDR_LEN);
	/* Source of reply becomes me. */
	memcpy(eg->src, myMAC, ETH_ADDR_LEN);
	printf("swapped addresses and about to zero the payload\r\n");
	/* Zero out payload. */
	bzero(eg->data, ETHER_MINPAYLOAD);
	/* FISH type becomes ?? */
//	eg->data[0] = FISH_DIRLIST;
	/*Test print*/
	printf("zero'd payload\r\n");
	/*initialize fishlist to all 0s*/
	int x,y;
	for(x=0;x<DIRENTRIES;x++){
		for(y=0;y<FNAMLEN;y++){
			fishlist[x][y]=0;
		}
	}
	/*Move dir entries into fishlist*/
	int i;
	for(i = 0; i<DIRENTRIES;i++){
		int offset = 1 + (i * (FNAMLEN+1));
	//we need to check if something is too small, we need to pad it
		memcpy(fishlist[i+offset], eg->data[i], MAXFILES);
	//access the file names the same way we got them before i forget how)
	// for each member of the fishlist, copy that to the eg for MAXFILES
	}
	if(i < ETHER_MINPAYLOAD){
//not exactly this, we need i to be the character count of things that are copied in the packet
		write(ETH0, packet, ETHER_MINPAYLOAD);// use this to pad if you're under the min character count
	}
	else{
		write(ETH0, packet, ETHER_SIZE);
	}
	/*TEST PRINT LOOP*/
	for (x=0;x<DIRENTRIES;x++){
		for(y=0;y<FNAMLEN;y++){
			printf("%c\r\n",fishlist[x][y]);
		}
	}
	return OK;
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

			case FISH_DIRASK: //what do we reply to a dirask with?
					  // we reply with a dirlist
				fishAsk(packet);			
				break;	

			case FISH_DIRLIST://what do we do when we get a dirlist?
					   // maybe just print it?
				fishList(packet);
				break;

			case FISH_GETFILE:
				break;

			case FISH_HAVEFILE:
				break;

			case FISH_NOFILE:
				break;

			default:
				printf("ERROR: Got unhandled FISH type %d\n", eg->data[0]);
			}
		}
	}

	return OK;
}
