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

/*
** ----------------------------------------------------------------
** physical appearance
** ---------------------------------------------------------------- 
*/

/* superblock:
**   0 4b MIRD 
**   1 4b version == 1
**   2 4b supr
**   3 4b clean flag
**   4 4b block size
**   5 4b frag bits
**   6 4b hashtrie bits
**
**   9 4b last block used
**  10 4b clean last block used
**  11 4b tables hashtrie
**  12 4b last clean tables hashtrie
**  13 4b free block list start
**  14 4b last clean free block list start
**
**  20 8b next transaction
**  22 8b last clean transaction
**
** n-2 4b random
** n-1 4b MIRD
**   n 4b checksum 
*/

#define SUPERBLOCK_MIRD 1296650820

#define DB_VERSION 2

/* block types */

#define BLOCK_SUPER         0x53555052
#define BLOCK_FREELIST      0x46524545
#define BLOCK_FRAG          0x46524147
#define BLOCK_FRAG_PROGRESS 0x50524f46
#define BLOCK_BIG           0x42424947

#define CHUNK_HASHTRIE 0x68617368
#define CHUNK_CELL     0x63656c6c
#define CHUNK_CONT     0x636f6e74
#define CHUNK_ROOT     0x726f6f74

#define MASTER_MIRD_TABLE_ID 0

/*
** block structure:
** 8b id of transaction owner
** 4b block type
** data
** 4b checksum
*/

/* 
** frag block structure:
** 0 8b id of transaction owner       0
** 2 4b block type == BLOCK_FRAG      2
** 3 4b table identifier              
** 4 4b start of frag 1               3 (4+no-1)
** 5 4b start of frag 2               4 (4+no-1)
**      ...
** n 4b start of frag MAXFRAGS        n=5+MAXFRAGS-2
**   4b end of frag MAXFRAGS            5+MAXFRAGS-1
** m    frag                          m=5+MAXFRAGS
**      frag
**    ...
**
** frag:
** 4b type
**    data
**
** big chunk block structure:
** 0 8b id of transaction owner
** 2 4b block type == BLOCK_BIG
** 3 4b table identifier
** 4 4b chunk id of next or zero
**
*/

/*
** hashtrie chunk structure:
** 4b type == CHUNK_HASHTRIE
** 4b key (any key that lead here)
** 4b chunk id of hashvalue 0
** 4b chunk id of hashvalue 1
** ...
** 4b chunk id of hashvalue 2^HASHTRIE_BITS-1
*/

/* 
** cell chunk structure:
** 4b type == CHUNK_CELL
** 4b hashtrie key
** 4b size
**    data
*/

/*
** table root frag structure:
** 4b type == CHUNK_ROOT
** 4b table id (= key)
** 4b table root (hashtrie id)
*/

/*
** free list block structure:
** 0   4b MIRD
** 1   4b version
** 2   4b BLOCK_FREELIST
** 3   4b next free list block
** 4   4b number of entries
** 5   4b free block no
** 6   4b free block no
** ...
*/

/*
** journal file chunk
** 0 4b type
** 1 8b transaction id
** 3 4b a
** 4 4b b
** 5 4b c
** ---
** 24 bytes

** journal entries:        
** type____________________trans id___a___________b________c______________
** new transaction         trans id   -           -        -
** transaction cancelled   trans id   -           -        -
** transaction finished    trans id   new tables  -        -
** block allocated         trans id   block id    -        -
** dependencies on key     trans id   table id    key id   old chunk id
** wrote key               trans id   table id    key id   old chunk id
** rewrote key             trans id   table id    key id   old chunk id
** deleted key             trans id   table id    key id   old chunk id
** redeleted key           trans id   table id    key id   old chunk id
** block may be unused     trans id   block id    -        -
** key lock                trans id   table id    key id   -
*/

#define MIRD_JOURNAL_ENTRY_SIZE 24

#define MIRDJ_NEW_TRANSACTION       0x6e657774
#define MIRDJ_TRANSACTION_CANCEL    0x636e636c
#define MIRDJ_TRANSACTION_FINISHED  0x66696e69
#define MIRDJ_ALLOCATED_BLOCK       0x616c6c6f
#define MIRDJ_WROTE_KEY             0x77726974
#define MIRDJ_DELETE_KEY            0x64656c65
#define MIRDJ_REWROTE_KEY           0x7277726f
#define MIRDJ_REDELETE_KEY          0x7264656c
#define MIRDJ_BLOCK_UNUSED          0x66726565
#define MIRDJ_DEPEND_KEY            0x64657065
#define MIRDJ_KEY_LOCK              0x6c6f636b

/*
 * string key storage
 *
 * 8b transaction id
 * 4b length of first key (n)
 * 4b length of first data (m)
 *  n first key
 *  m first data
 * 8b transaction id
 * 4b length of second key
 * 4b length of second data
 *  ...
 */

/* usable macrons */

#define BLOCK_LONG(DATA,N) ( ((UINT32*)DATA)[N] )
#define READ_BLOCK_LONG(DATA,N) ( (UINT32)ntohl(((UINT32*)(DATA))[N]) )
#define WRITE_BLOCK_LONG(DATA,N,X) ( ((UINT32*)(DATA))[N]=(UINT32)htonl((UINT32)(X)) )
#define LONGS_IN_BLOCK(DB) ( (DB)->block_size>>2 )

#define CHUNK_ID_2_BLOCK(DB,ID) ( (ID)>>((DB)->frag_bits) )
#define CHUNK_ID_2_FRAG(DB,ID) ( (ID) & ( (1<<((DB)->frag_bits))-1) )
#define BLOCK_FRAG_2_CHUNK_ID(DB,B,F) ( ((B)<<(DB)->frag_bits) | (F) )
#define MAX_FRAGS(DB) ((UINT32)((1<<(DB)->frag_bits)-1))
