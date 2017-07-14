/*
*********************************************************************************************************
*                                          The Real-Time Kernel
*                                            MEMORY MANAGEMENT
*
* File    : OS_MEM.C
* By      : Czhiming
* Version : V2.0
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE

#ifndef  OS_MASTER_FILE
#include <emos.h>
#endif 

#if (OS_MEM_EN > 0u) && (OS_MAX_MEM_PART > 0u)
/*
*********************************************************************************************************
*                                      CREATE A MEMORY PARTITION
* Description : Create a fixed-sized memory partition that will be managed by emos.
* Arguments   : addr     is the starting address of the memory partition
*               nblks    is the number of memory blocks to create from the partition.
*               blksize  is the size (in bytes) of each block in the memory partition.
*               perr     is a pointer to a variable containing an error message which will be set by
*                        this function to either:
*********************************************************************************************************
*/
OS_MEM  *OSMemCreate (void   *addr,
                      INT32U  nblks,
                      INT32U  blksize,
                      INT8U  *perr)
{
    OS_MEM    *pmem;
    INT8U     *pblk;
    void     **plink;
    INT32U     loops;
    INT32U     i;
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif

#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_MEM *)0);
    }
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_MEM *)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (addr == (void *)0) {                          /* Must pass a valid address for the memory part.*/
        *perr = OS_ERR_MEM_INVALID_ADDR;
        return ((OS_MEM *)0);
    }
    if (((INT32U)addr & (sizeof(void *) - 1u)) != 0u){  /* Must be pointer size aligned                */
        *perr = OS_ERR_MEM_INVALID_ADDR;
        return ((OS_MEM *)0);
    }
    if (nblks < 2u) {                                 /* Must have at least 2 blocks per partition     */
        *perr = OS_ERR_MEM_INVALID_BLKS;
        return ((OS_MEM *)0);
    }
    if (blksize < sizeof(void *)) {                   /* Must contain space for at least a pointer     */
        *perr = OS_ERR_MEM_INVALID_SIZE;
        return ((OS_MEM *)0);
    }
#endif
    OS_ENTER_CRITICAL();
    pmem = OSMemFreeList;                             /* Get next free memory partition                */
    if (OSMemFreeList != (OS_MEM *)0) {               /* See if pool of free partitions was empty      */
        OSMemFreeList = (OS_MEM *)OSMemFreeList->OSMemFreeList;
    }
    OS_EXIT_CRITICAL();
    if (pmem == (OS_MEM *)0) {                        /* See if we have a memory partition             */
        *perr = OS_ERR_MEM_INVALID_PART;
        return ((OS_MEM *)0);
    }
    plink = (void **)addr;                            /* Create linked list of free memory blocks      */
    pblk  = (INT8U *)addr;
    loops  = nblks - 1u;
    for (i = 0u; i < loops; i++) {
        pblk +=  blksize;                             /* Point to the FOLLOWING block                  */
       *plink = (void  *)pblk;                        /* Save pointer to NEXT block in CURRENT block   */
        plink = (void **)pblk;                        /* Position to  NEXT      block                  */
    }
    *plink              = (void *)0;                  /* Last memory block points to NULL              */
    pmem->OSMemAddr     = addr;                       /* Store start address of memory partition       */
    pmem->OSMemFreeList = addr;                       /* Initialize pointer to pool of free blocks     */
    pmem->OSMemNFree    = nblks;                      /* Store number of free blocks in MCB            */
    pmem->OSMemNBlks    = nblks;
    pmem->OSMemBlkSize  = blksize;                    /* Store block size of each memory blocks        */
    *perr               = OS_ERR_NONE;
    return (pmem);
}

/*
*********************************************************************************************************
*                                         GET A MEMORY BLOCK
* Description : Get a memory block from a partition
* Arguments   : pmem    is a pointer to the memory partition control block
*               perr    is a pointer to a variable containing an error message which will be set by this
*                       function to either:
*********************************************************************************************************
*/
void  *OSMemGet (OS_MEM  *pmem,
                 INT8U   *perr)
{
    void      *pblk;
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((void *)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                        /* Must point to a valid memory partition        */
        *perr = OS_ERR_MEM_INVALID_PMEM;
        return ((void *)0);
    }
#endif
    OS_ENTER_CRITICAL();
    if (pmem->OSMemNFree > 0u) {                      /* See if there are any free memory blocks       */
        pblk                = pmem->OSMemFreeList;    /* Yes, point to next free memory block          */
        pmem->OSMemFreeList = *(void **)pblk;         /*      Adjust pointer to new free list          */
        pmem->OSMemNFree--;                           /*      One less memory block in this partition  */
        OS_EXIT_CRITICAL();
        *perr = OS_ERR_NONE;                          /*      No error                                 */
        return (pblk);                                /*      Return memory block to caller            */
    }
    OS_EXIT_CRITICAL();
    *perr = OS_ERR_MEM_NO_FREE_BLKS;                  /* No,  Notify caller of empty memory partition  */
    return ((void *)0);                               /*      Return NULL pointer to caller            */
}

/*
*********************************************************************************************************
*                                 GET THE NAME OF A MEMORY PARTITION
* Description: This function is used to obtain the name assigned to a memory partition.
*********************************************************************************************************
*/

#if OS_MEM_NAME_EN > 0u
INT8U  OSMemNameGet (OS_MEM   *pmem,
                     INT8U   **pname,
                     INT8U    *perr)
{
    INT8U      len;
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                   /* Is 'pmem' a NULL pointer?                          */
        *perr = OS_ERR_MEM_INVALID_PMEM;
        return (0u);
    }
    if (pname == (INT8U **)0) {                  /* Is 'pname' a NULL pointer?                         */
        *perr = OS_ERR_PNAME_NULL;
        return (0u);
    }
#endif
    if (OSIntNesting > 0u) {                     /* See if trying to call from an ISR                  */
        *perr = OS_ERR_NAME_GET_ISR;
        return (0u);
    }
    OS_ENTER_CRITICAL();
    *pname = pmem->OSMemName;
    len    = OS_StrLen(*pname);
    OS_EXIT_CRITICAL();
    *perr  = OS_ERR_NONE;
    return (len);
}
#endif

/*
*********************************************************************************************************
*                                       RELEASE A MEMORY BLOCK
* Description : Returns a memory block to a partition
* Arguments   : pmem    is a pointer to the memory partition control block
*               pblk    is a pointer to the memory block being released.
*********************************************************************************************************
*/
INT8U  OSMemPut (OS_MEM  *pmem,
                 void    *pblk)
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif


#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                   /* Must point to a valid memory partition             */
        return (OS_ERR_MEM_INVALID_PMEM);
    }
    if (pblk == (void *)0) {                     /* Must release a valid block                         */
        return (OS_ERR_MEM_INVALID_PBLK);
    }
#endif
    OS_ENTER_CRITICAL();
    if (pmem->OSMemNFree >= pmem->OSMemNBlks) {  /* Make sure all blocks not already returned          */
        OS_EXIT_CRITICAL();
        return (OS_ERR_MEM_FULL);
    }
    *(void **)pblk      = pmem->OSMemFreeList;   /* Insert released block into free block list         */
    pmem->OSMemFreeList = pblk;
    pmem->OSMemNFree++;                          /* One more memory block in this partition            */
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);                        /* Notify caller that memory block was released       */
}

/*
*********************************************************************************************************
*                                       QUERY MEMORY PARTITION
* Description : This function is used to determine the number of free memory blocks and the number of
*               used memory blocks from a memory partition.
*********************************************************************************************************
*/
#if OS_MEM_QUERY_EN > 0u
INT8U  OSMemQuery (OS_MEM       *pmem,
                   OS_MEM_DATA  *p_mem_data)
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                   /* Must point to a valid memory partition             */
        return (OS_ERR_MEM_INVALID_PMEM);
    }
    if (p_mem_data == (OS_MEM_DATA *)0) {        /* Must release a valid storage area for the data     */
        return (OS_ERR_MEM_INVALID_PDATA);
    }
#endif
    OS_ENTER_CRITICAL();
    p_mem_data->OSAddr     = pmem->OSMemAddr;
    p_mem_data->OSFreeList = pmem->OSMemFreeList;
    p_mem_data->OSBlkSize  = pmem->OSMemBlkSize;
    p_mem_data->OSNBlks    = pmem->OSMemNBlks;
    p_mem_data->OSNFree    = pmem->OSMemNFree;
    OS_EXIT_CRITICAL();
    p_mem_data->OSNUsed    = p_mem_data->OSNBlks - p_mem_data->OSNFree;
    return (OS_ERR_NONE);
}
#endif                                           /* OS_MEM_QUERY_EN                                    */

/*
*********************************************************************************************************
*                                 INITIALIZE MEMORY PARTITION MANAGER
* Description : This function is called by emos to initialize the memory partition manager.  Your
*               application MUST NOT call this function.
*********************************************************************************************************
*/
void  OS_MemInit (void)
{
#if OS_MAX_MEM_PART == 1u
    OS_MemClr((INT8U *)&OSMemTbl[0], sizeof(OSMemTbl));   /* Clear the memory partition table          */
    OSMemFreeList               = (OS_MEM *)&OSMemTbl[0]; /* Point to beginning of free list           */
#if OS_MEM_NAME_EN > 0u
    OSMemFreeList->OSMemName    = (INT8U *)"?";           /* Unknown name                              */
#endif
#endif

#if OS_MAX_MEM_PART >= 2u
    OS_MEM  *pmem;
    INT16U   i;


    OS_MemClr((INT8U *)&OSMemTbl[0], sizeof(OSMemTbl));   /* Clear the memory partition table          */
    for (i = 0u; i < (OS_MAX_MEM_PART - 1u); i++) {       /* Init. list of free memory partitions      */
        pmem                = &OSMemTbl[i];               /* Point to memory control block (MCB)       */
        pmem->OSMemFreeList = (void *)&OSMemTbl[i + 1u];  /* Chain list of free partitions             */
#if OS_MEM_NAME_EN > 0u
        pmem->OSMemName  = (INT8U *)(void *)"?";
#endif
    }
    pmem                = &OSMemTbl[i];
    pmem->OSMemFreeList = (void *)0;                      /* Initialize last node                      */
#if OS_MEM_NAME_EN > 0u
    pmem->OSMemName = (INT8U *)(void *)"?";
#endif

    OSMemFreeList   = &OSMemTbl[0];                       /* Point to beginning of free list           */
#endif
}
#endif                                                    /* OS_MEM_EN                                 */
