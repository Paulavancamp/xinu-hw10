/**
 * @file     xsh_test.c
 * @provides xsh_test
 *
 */
/* Embedded XINU, Copyright (C) 2009.  All rights reserved. */

#include <xinu.h>

/**
 * Shell command (test) is testing hook.
 * @param args array of arguments
 * @return OK for success, SYSERR for syntax error
 */
command xsh_test(int nargs, char *args[])
{
    // Test your O/S.
	printf("This is where you should put some testing code.\n");
	printFreeList();
	printDirectory();
	for(int i = 0;i<100;i++){
		fileCreate("shmoop");
        }
	printFreeList();
 	printDirectory();
	fileDelete(5);
	fileDelete(4);
/*	for(int j=0;j<5;j++){
		fileDelete(j);
	}*/
	printFreeList();
	printDirectory();
  return OK;



}
