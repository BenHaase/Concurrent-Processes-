/* Ben Haase
 * CS 4760
 * Assignment 2
 * $Author: o-haase $
 * $Date: 2016/02/24 22:49:00 $
 * $Log: memstrct.h,v $
 * Revision 1.2  2016/02/24 22:49:00  o-haase
 * Added some comments
 *
 * Revision 1.1  2016/02/20 00:42:24  o-haase
 * Initial revision
 *
 */
typedef enum {idle, want_in, in_cs} state;

typedef struct{
	int turn;
	state flag[20];
	}pt;

#define SHMKEY 56
#define BUFF_SZ sizeof(pt)
