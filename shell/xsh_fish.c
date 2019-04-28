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
	int i = 0;

	// Zero out the packet buffer.
	bzero(packet, PKTSZ);

	for (i = 0; i < ETH_ADDR_LEN; i++)
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
		
	for (i = 1; i < ETHER_MINPAYLOAD; i++)
	{
		*ppkt++ = i;
	}
		
	write(ETH0, packet, ppkt - packet);
	
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
			//testprint
			printf("match found! \r\n");
			//wait 1s for response
			sleep(1000);	
			/*Print contents of fishlist table*/
			int count;
			for(count=0;count<DIRENTRIES;count++){
				printf("%c\r\n",fishlist[count][FISH_MAXNAME]);
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
	//Locate named node in school
	for(j=0;j<SCHOOLMAX;j++){
		if(school[j].name==args[2]){
			//send the GETFILE packet
			fishSend(school[j].mac,FISH_GETFILE);
			//wait 1s for reply	
			/*TO DO LATER... STORE THE FILE*/
			//fileSharer puts file in system when it arrives
			printf("File will be stored here");
		}
		return OK;
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
