/**
 * @file     xsh_fish.c
 * @provides xsh_fish
 * TA-BOT:MAILTO paula.vancamp@marquette.edu alexander.kosla@marquette.edu
 */
/* Embedded XINU, Copyright (C) 2013.  All rights reserved. */

#include <xinu.h>

/**
 * Local function for sending simply FiSh packets with empty payloads.
 * @param dst MAC address of destination
 * @param type FiSh protocal type (see fileshare.h)
 * @return OK for success, SYSERR otherwise.
 */
static int fishSend(uchar *dst, char fishtype)
{
	uchar packet[PKTSZ];
	uchar *ppkt = packet;
	// Zero out the packet buffer.
	bzero(packet, PKTSZ);

	for (int i = 0; i < ETH_ADDR_LEN; i++)
	{
		*ppkt++ = dst[i];
	}

	// Source: Get my MAC address from the Ethernet device
	control(ETH0, ETH_CTRL_GET_MAC, (long)ppkt, 0);
	ppkt += ETH_ADDR_LEN;
		
	// Type: Special "3250" packet protocol.
	*ppkt++ = ETYPE_FISH >> 8;
	*ppkt++ = ETYPE_FISH & 0xFF;
		
	*ppkt++ = fishtype;
		
	for (int i = 1; i < ETHER_MINPAYLOAD; i++)
	{
		*ppkt++ = i;
	}
		
	write(ETH0, packet, ppkt - packet);
	
	return OK;
}
	
/********************************************************************/
/*fish send payload*/
static int fishSendFile(uchar *dst, char fishtype, char *fileName){
	uchar packet[PKTSZ];
	uchar *ppkt = packet;
	struct ethergram *eg = (struct ethergram *)packet;
	bzero(packet, PKTSZ);
	for(int i = 0; i < ETH_ADDR_LEN; i++){
		*ppkt++ = dst[i];
	}
	control(ETH0, ETH_CTRL_GET_MAC, (long)ppkt, 0);
	ppkt += ETH_ADDR_LEN;
	*ppkt++ = ETYPE_FISH >> 8;
	*ppkt++ = ETYPE_FISH & 0xFF;
	*ppkt++ = fishtype;

	memcpy(ppkt, fileName, FNAMLEN);
	ppkt += FNAMLEN;
	eg->data[0] = FISH_GETFILE;

	for(int i=1;i<ETHER_MINPAYLOAD-FNAMLEN;i++){
		*ppkt++=0;
	}

	write(ETH0, packet,ppkt-packet);
	return OK;
}



	
/**
 * Shell command (fish) is file sharing client.
 * @param args array of arguments
 * @return OK for success, SYSERR for syntax error
 */
command xsh_fish(int nargs, char *args[])
{
	uchar bcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	int i = 0;
	int j = 0;

	if (nargs == 2 && strncmp(args[1], "ping", 4) == 0)
    {
		fishSend(bcast, FISH_PING);
		
		sleep(1000);

		printf("Known FISH nodes in school:\n");
		for (i = 0; i < SCHOOLMAX; i++)
		{
			if (school[i].used)
			{
				printf("\t%02X:%02X:%02X:%02X:%02X:%02X",
					   school[i].mac[0],
					   school[i].mac[1],
					   school[i].mac[2],
					   school[i].mac[3],
					   school[i].mac[4],
					   school[i].mac[5]);
				printf("\t%s\n", school[i].name);
			}
		}
		printf("\n");
		return OK;
	}

/********************************************************************/
/*                        if fish list _____                        */
	else if (nargs == 3 && strncmp(args[1], "list", 4) == 0)
	{
	//Locate named node in school and send a FISH_DIRASK packet to it.
	for(j=0;j<SCHOOLMAX;j++){
		//check if argument matches a machine in the system
		int same=strncmp(school[j].name,args[2],FISH_MAXNAME);
		if(same==0){
			//send a DIRASK to that node
			fishSend(school[j].mac,FISH_DIRASK);
			//wait 1s for response
			sleep(1000);	
			/*Print contents of fishlist table*/
			int i;
			for(i=0;i<DIRENTRIES;i++){
				if(fishlist[i]!=NULL){
				printf("%s\r\n",fishlist[i]);
				}
				else{ break;}
			}
		return OK;
		}
	}
	printf("No FiSh \"%s\" found in school.\n", args[2]);
	return OK;
	}

/********************************************************************/
/*                        if fish get ______                        */	
	else if (nargs == 4 && strncmp(args[1], "get", 4) == 0)
	{
	/*Traverse through school to find fish*/
	for(j=0;j<SCHOOLMAX;j++){
		if(strncmp(school[j].name,args[2],FISH_MAXNAME)==0){
			//send the GETFILE packet
			fishSendFile(school[j].mac,FISH_GETFILE,args[3]);
			return OK;
		}
	}
	printf("No FiSh \"%s\" found in school.\n", args[2]);
	return OK;
	}

/************************************************************************/
	else
    {
        fprintf(stdout, "Usage: fish [ping | list <node> | get <node> <filename>]\n");
        fprintf(stdout, "Simple file sharing protocol.\n");
        fprintf(stdout, "ping - lists available FISH nodes in school.\n");
        fprintf(stdout, "list <node> - lists directory contents at node.\n");
        fprintf(stdout, "get <node> <file> - requests a remote file.\n");
        fprintf(stdout, "\t--help\t display this help and exit\n");

        return 0;
    }

    return OK;
}
