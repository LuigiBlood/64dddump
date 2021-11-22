//
// ci.h
//

#ifndef _CI_H
#define _CI_H

// prototypes
void	ciSectorsToRam(unsigned int ramaddr, unsigned int lba, unsigned int sectors);
void	ciRamToSectors(unsigned int ramaddr, unsigned int lba, unsigned int sectors);

void	ciReadSector(unsigned char *buf, unsigned int lba);
void	ciWriteSector(unsigned char *buf, unsigned int lba);
void	ciSetCycleTime(int cycletime);
int		ciOptimizeCycleTime();
void	ciSetByteSwap(int byteswap);
int		ciGetMagic();
int		ciGetRevision();
void	ciSetSave(int savetype);
void 	ciStartUpgrade();
int 	ciGetUpgradeStatus();
void	ciEnableRomWrites();
void	ciDisableRomWrites();

void	ciWriteEEPROMBuffer(unsigned char *buf, int start, int size);
void	ciWriteLBAWBBuffer(unsigned char *buf, int start, int size);

#endif