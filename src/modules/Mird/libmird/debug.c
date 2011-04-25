/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
** libMird by Mirar <mirar@mirar.org>
** please submit bug reports and patches to the author
**
** also see http://www.mirar.org/mird/
*/

/* handles debug stuff */

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "internal.h"

#include "dmalloc.h"
#include <stdio.h>

static const char RCSID[]=
   "$Id$";

#define PRINT_BLOCK_FRAG(X) \
	CHUNK_ID_2_BLOCK(db,(X)), \
	CHUNK_ID_2_FRAG(db,(X))

#define P "l"

static void examine_chunk(struct mird *db,
			  unsigned char *data,UINT32 len)
{
   UINT32 i,j;
   if (len<4)
      fprintf(stderr,"type unknown (too short)\n");
   else
   {
      switch (READ_BLOCK_LONG(data,0))
      {
	 case CHUNK_CELL:
	    fprintf(stderr,"cell; key=%"P"xh size=%"P"db\n",
		    READ_BLOCK_LONG(data,1),
		    READ_BLOCK_LONG(data,2));
	    break;
	 case CHUNK_HASHTRIE:
	    fprintf(stderr,"hashtrie node; key=%"P"xh (%"P"db)\n",
		    READ_BLOCK_LONG(data,1),
		    len-4);
	    fprintf(stderr,"       |                   ");
	    for (j=i=0; i<(UINT32)(1<<db->hashtrie_bits); i++)
	       if (READ_BLOCK_LONG(data,i+2))
	       {
		  j++;
		  fprintf(stderr,"%"P"x:%"P"xh%"P"d ",
			  i,PRINT_BLOCK_FRAG(READ_BLOCK_LONG(data,i+2)));
	       }
	    if (!j) 
	       fprintf(stderr,"empty hashtrie node (?)\n");
	    else
	       fprintf(stderr,"\n");
	    break;
	 case CHUNK_CONT:
	    fprintf(stderr,"continued data; key=%"P"xh (%"P"db)\n",
		    READ_BLOCK_LONG(data,1),len-8);
	    break;
	 case CHUNK_ROOT:
	    fprintf(stderr,"table root; "
		    "id %"P"xh (%"P"x), root %"P"xh%"P"d,",
		    READ_BLOCK_LONG(data,1),
		    READ_BLOCK_LONG(data,1),
		    PRINT_BLOCK_FRAG(READ_BLOCK_LONG(data,2)));
	    switch (READ_BLOCK_LONG(data,3))
	    {
	       case MIRD_TABLE_HASHKEY:
		  fprintf(stderr," hashkey type\n");
		  break;
	       case MIRD_TABLE_STRINGKEY:
		  fprintf(stderr," stringkey type\n");
		  break;
	       default:
		  fprintf(stderr," illegal type (%08lxh)\n",
			  READ_BLOCK_LONG(data,3));
		  break;
	    }
	    break;
	 default:
	    fprintf(stderr,"type unknown (%08lxh)\n",
		    READ_BLOCK_LONG(data,0));
	    break;
      }
   }
	       
}

void mird_describe_block(struct mird *db,UINT32 no)
{
   unsigned char *data;
   MIRD_RES res;
   UINT32 i,n;
   UINT32 a,b,c;

   data=smalloc(db->block_size);
   if (!data)
   {
      fprintf(stderr,"mird_describe_block: out of memory\n");
      return;
   }

   res=mird_low_block_read(db,no,data,1);
   if (res)
   {
      char *err;
      mird_describe_error(res,&err);
      mird_free_error(res);
      fprintf(stderr,"%4lxh: ERROR: %s\n",no,err);
      sfree(err);
      return;
   }

   fprintf(stderr,"%4lxh: ",no);
   if (READ_BLOCK_LONG(data,0)==SUPERBLOCK_MIRD)
      fprintf(stderr,"special block; version: %"P"u\n",
	      READ_BLOCK_LONG(data,1));
   else
      if (READ_BLOCK_LONG(data,0)==0 &&
	  READ_BLOCK_LONG(data,1)==0 &&
	  READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1)==0)
      {
	 fprintf(stderr,"zero block (illegal)\n");
	 return;
      }
   else
      fprintf(stderr,"owner: transaction %"P"u:%"P"u\n",
	      READ_BLOCK_LONG(data,0),
	      READ_BLOCK_LONG(data,1));

   switch (READ_BLOCK_LONG(data,2))
   {
      case BLOCK_SUPER:
	 fprintf(stderr,"       type: superblock\n");
	 
	 fprintf(stderr,"       | clean flag....................%"P"u (%s)\n",
		 READ_BLOCK_LONG(data,3),
		 READ_BLOCK_LONG(data,3)?"clean":"dirty");
	 fprintf(stderr,"       | block size....................%"P"u\n",
		 READ_BLOCK_LONG(data,4));
	 fprintf(stderr,"       | frag bits.....................%"P"u (%u frags)\n",
		 READ_BLOCK_LONG(data,5),(1<<READ_BLOCK_LONG(data,5))-1);
	 fprintf(stderr,"       | hashtrie bits.................%"P"u (%u entries)\n",
		 READ_BLOCK_LONG(data,6),1<<READ_BLOCK_LONG(data,6));

	 fprintf(stderr,"       |\n");
	 fprintf(stderr,"       | last block used...............%"P"xh\n",
		 READ_BLOCK_LONG(data,9));
	 fprintf(stderr,"       | tables hashtrie...............%"P"xh %"P"u\n",
		 PRINT_BLOCK_FRAG(READ_BLOCK_LONG(data,11)));
	 fprintf(stderr,"       | free block list start.........%"P"xh\n",
		 READ_BLOCK_LONG(data,13));
	 fprintf(stderr,"       | next transaction..............%"P"u:%"P"u\n",
		 READ_BLOCK_LONG(data,20),
		 READ_BLOCK_LONG(data,21) );
	 fprintf(stderr,"       |\n");
	 fprintf(stderr,"       | last last block used..........%"P"xh\n",
		 READ_BLOCK_LONG(data,10));
	 fprintf(stderr,"       | last clean tables hashtrie....%"P"xh %"P"u\n",
		 PRINT_BLOCK_FRAG(READ_BLOCK_LONG(data,12)));
	 fprintf(stderr,"       | last clean free list start....%"P"xh\n",
		 READ_BLOCK_LONG(data,14));

	 fprintf(stderr,"       | last next transaction.........%"P"u:%"P"u\n",
		 READ_BLOCK_LONG(data,22),
		 READ_BLOCK_LONG(data,23) );
	 fprintf(stderr,"       |\n");
	 fprintf(stderr,"       | random value..................%08lxh\n",
		 READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-2));

	 break;

      case BLOCK_FREELIST:
	 fprintf(stderr,"       type: freelist\n");
	 fprintf(stderr,"       | next freelist block...........%"P"xh\n",
		 READ_BLOCK_LONG(data,3));
	 fprintf(stderr,"       | number of entries.............%"P"u %s\n",
		 READ_BLOCK_LONG(data,4),
		 READ_BLOCK_LONG(data,4)?"(below)":"");
	 n=READ_BLOCK_LONG(data,4);
	 if (n>db->block_size/4) n=0;
	 if (n)
	 {
#define COLUMNS_6 11
	    UINT32 m=(n+COLUMNS_6-1)/COLUMNS_6;
	    UINT32 j;
	    for (i=0; i*COLUMNS_6<n; i++)
	    {
	       fprintf(stderr,"       | ");
	       for (j=i; j<n; j+=m)
		  fprintf(stderr,"%5lxh",READ_BLOCK_LONG(data,5+j));
	       fprintf(stderr,"\n");
	    }
	 }
	 break;

      case BLOCK_FRAG:
      case BLOCK_FRAG_PROGRESS:

	 if (READ_BLOCK_LONG(data,2)==BLOCK_FRAG_PROGRESS)
	    fprintf(stderr,"       type: frag block (in progress!)\n");
	 else
	    fprintf(stderr,"       type: frag block\n");
	 fprintf(stderr,"       | table id......................%"P"xh (%"P"d)\n",
		 READ_BLOCK_LONG(data,3),READ_BLOCK_LONG(data,3));

	 if (READ_BLOCK_LONG(data,5)==0)
	    fprintf(stderr,"       | no frags in this block, though (?)\n");
	 else
	    fprintf(stderr,"       | frag offset   len \n");

	 c=READ_BLOCK_LONG(data,4);
	 for (i=1; i<=MAX_FRAGS(db); i++)
	 {
	    a=READ_BLOCK_LONG(data,3+i);
	    b=READ_BLOCK_LONG(data,4+i);
	    if (b==0) continue;
	    else c=b;
	    fprintf(stderr,"       | %4lu %6lu %5lu ",
		    i,a,b-a);
	    /* type or comment */

	    if (a==0 || b==0 || b>db->block_size || a>db->block_size)
	       fprintf(stderr,"illegal (out of block)\n");
	    else
	       examine_chunk(db,data+a,b-a);
	 }
	 fprintf(stderr,"       | unused bytes..................%"P"d bytes (%"P"d%% overhead)\n",
		 db->block_size-4-c,
		 (100*((db->block_size-4-c)+
		       READ_BLOCK_LONG(data,4)))/db->block_size);

	 break;

      case BLOCK_BIG:
	 fprintf(stderr,"       type: big block\n");
	 fprintf(stderr,"       | table id......................%"P"xh (%"P"d)\n",
		 READ_BLOCK_LONG(data,3),READ_BLOCK_LONG(data,3));
	 fprintf(stderr,"       | continued in..................%"P"xh %"P"u\n",
		 PRINT_BLOCK_FRAG(READ_BLOCK_LONG(data,4)));
	 fprintf(stderr,"       | contents (%lub): ",
		 db->block_size-24);
	 examine_chunk(db,data+20,db->block_size-24);
	 break;

      default:
	 fprintf(stderr,"       type: unknown (%"P"xh)\n",
		 READ_BLOCK_LONG(data,2));
	 break;
   }

   if (mird_checksum((UINT32*)data,(db->block_size>>2)-1)==
       READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1))
      fprintf(stderr,"       | checksum......................%08lxh (good)\n",
	      READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1));
   else
      fprintf(stderr,"       | checksum......................%08lxh (bad, expected %08lxh)\n",
	      READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1),
	      mird_checksum((UINT32*)data,(db->block_size>>2)-1));
	 

}

void mird_hexdump(unsigned char *s,UINT32 n)
{
   UINT32 i,j;
   for (j=0; j<n; j+=16)
   {
      for (i=0; i<16 && i+j<n; i++) fprintf(stderr,"%02x ",s[i+j]);
      for (; i<16; i++) fprintf(stderr,"   ");
      for (i=0; i<16 && i+j<n; i++)
	 fprintf(stderr,"%c",(s[i+j]>=' ' && s[i+j]<=126)?s[i+j]:'.');
      fprintf(stderr,"\n");
   }
}
