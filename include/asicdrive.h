#define USR_SECS_PER_BLK          85 /* Number of user sectors in a logical block */
#define C2_SECS_PER_BLK            4 /* Number of C2 sectors in a logical block */
#define GAP_SECS_PER_BLK           1 /* Number of GAP sectors in a logical block */
#define ALL_SECS_PER_BLK         (USR_SECS_PER_BLK + C2_SECS_PER_BLK + GAP_SECS_PER_BLK)

#define SEC_SIZE_ZONE0           232 /* Number of bytes in a Zone 0 sector */
#define SEC_SIZE_ZONE1           216 /* Number of bytes in a Zone 1 sector */
#define SEC_SIZE_ZONE2           208 /* Number of bytes in a Zone 2 sector */
#define SEC_SIZE_ZONE3           192 /* Number of bytes in a Zone 3 sector */
#define SEC_SIZE_ZONE4           176 /* Number of bytes in a Zone 4 sector */
#define SEC_SIZE_ZONE5           160 /* Number of bytes in a Zone 5 sector */
#define SEC_SIZE_ZONE6           144 /* Number of bytes in a Zone 6 sector */
#define SEC_SIZE_ZONE7           128 /* Number of bytes in a Zone 7 sector */
#define SEC_SIZE_ZONE8           112 /* Number of bytes in a Zone 8 sector */

/*************************************/
/* Definition of each ASIC register  */
/*************************************/
#define ASIC_IO_BASE       0xA5000000
#define ASIC_C2_BUFF       (ASIC_IO_BASE+0x0000) /* Buffer to store C2 data */
#define ASIC_SECTOR_BUFF   (ASIC_IO_BASE+0x0400) /* Buffer for C2 data to be corrected */
#define MSEQ_RAM_ADDR      (ASIC_IO_BASE+0x0580) /* Start address of the micro-sequencer */

#define ASIC_DATA          (ASIC_IO_BASE+0x0500) /* W / R */
#define ASIC_MISC_REG      (ASIC_IO_BASE+0x0504) /* R     */
#define ASIC_STATUS        (ASIC_IO_BASE+0x0508) /* R     */
#define ASIC_CMD           (ASIC_IO_BASE+0x0508) /* W     */
#define ASIC_CUR_TK        (ASIC_IO_BASE+0x050c) /* R     */
#define ASIC_BM_STATUS     (ASIC_IO_BASE+0x0510) /* R     */
#define ASIC_BM_CTL        (ASIC_IO_BASE+0x0510) /* W     */
#define ASIC_ERR_SECTOR    (ASIC_IO_BASE+0x0514) /* R     */
#define ASIC_SEQ_STATUS    (ASIC_IO_BASE+0x0518) /* R     */
#define ASIC_SEQ_CTL       (ASIC_IO_BASE+0x0518) /* W     */
#define ASIC_CUR_SECTOR    (ASIC_IO_BASE+0x051c) /* R     */
#define ASIC_HARD_RESET    (ASIC_IO_BASE+0x0520) /* W     */
#define ASIC_C1_S0         (ASIC_IO_BASE+0x0524) /* R     */
#define ASIC_HOST_SECBYTE  (ASIC_IO_BASE+0x0528) /* W / R */
#define ASIC_C1_S2         (ASIC_IO_BASE+0x052c) /* R     */
#define ASIC_SEC_BYTE      (ASIC_IO_BASE+0x0530) /* W / R */
#define ASIC_C1_S4         (ASIC_IO_BASE+0x0534) /* R     */
#define ASIC_C1_S6         (ASIC_IO_BASE+0x0538) /* R     */
#define ASIC_CUR_ADDR      (ASIC_IO_BASE+0x053c) /* R     */
#define ASIC_ID_REG        (ASIC_IO_BASE+0x0540) /* R     */
#define ASIC_TEST_REG      (ASIC_IO_BASE+0x0544) /* R     */
#define ASIC_TEST_PIN_SEL  (ASIC_IO_BASE+0x0548) /* W     */

/*-----------------------------------*/
/* ASIC status register(R)bit defs   */
/*-----------------------------------*/
#define LEO_STAT_DATA_REQ      0x40000000
#define LEO_STAT_C2_XFER       0x10000000
#define LEO_STAT_BM_ERROR      0x08000000
#define LEO_STAT_BM_INT        0x04000000
#define LEO_STAT_MECHA_INT     0x02000000
#define LEO_STAT_DISK          0x01000000
#define LEO_STAT_BUSY          0x00800000
#define LEO_STAT_RESET         0x00400000
#define LEO_STAT_SPM_OFF       0x00100000
#define LEO_STAT_HEAD_RETRACT  0x00080000
#define LEO_STAT_WPROTECT_ERR  0x00040000
#define LEO_STAT_MECHA_ERROR   0x00020000
#define LEO_STAT_DISK_CHANGE   0x00010000
#define LEO_STAT_MASK          0xffff0000

#define LEO_STAT_NC_CHK        0x0000ffff

#define LEO_STAT_WRITE_END     0x00000000

/*-----------------------------------*/
/* ASIC BM status reg(R) bit defs    */
/*-----------------------------------*/
#define LEO_BMST_RUNNING       0x80000000 /* bit 15 */
#define LEO_BMST_ERROR         0x04000000 /* bit 10 */
#define LEO_BMST_MICRO_STATUS  0x02000000 /* bit  9 */
#define LEO_BMST_BLOCKS        0x01000000 /* bit  8 */

#define LEO_BMST_C1_CORRECT    0x00800000 /* bit  7 */
#define LEO_BMST_C1_DOUBLE     0x00400000 /* bit  6 */
#define LEO_BMST_C1_SINGLE     0x00200000 /* bit  5 */
#define LEO_BMST_C1_ERROR      0x00010000 /* bit  0 */

/*-----------------------------------*/
/* ASIC BM control reg(W)bit defs    */
/*-----------------------------------*/
#define START_BM               0x80000000
#define BM_MODE                0x40000000
#define BM_INT_MASK            0x20000000
#define BM_RESET               0x10000000
#define BM_DISABLE_OR_CHK      0x08000000
#define BM_DISABLE_C1          0x04000000
#define BM_XFERBLKS            0x02000000
#define BM_MECHA_INT_RESET     0x01000000

/*----------------------------------------*/
/* ASIC ERROR/SECTOR register(W) bit defs */
/*----------------------------------------*/
#define BIT_AM_FAIL            0x80000000
#define BIT_MICRO_FAIL         0x40000000
#define BIT_SPINDLE_FAIL       0x20000000
#define BIT_OVER_RUN           0x10000000
#define BIT_OFFTRACK           0x08000000
#define BIT_NO_DISK            0x04000000
#define BIT_CLOCK_UNLOCK       0x02000000
#define BIT_SELF_STOP          0x01000000

/*------------------------------------*/
/* ASIC SEQ control reg(W)bit defs    */
/*------------------------------------*/
#define MICRO_INT_MASK         0x80000000
#define MICRO_PC_ENABLE        0x40000000

/*------------------------------------*/
/* ASIC CUR_TK register(R)bit defs    */
/*------------------------------------*/
#define BIT_INDEX_LOCK         0x60000000

