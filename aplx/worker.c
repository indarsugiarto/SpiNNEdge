
#include "SpiNNEdge.h"

// After collecting block information, workers (including leadAp)
// will compute its working region.
void computeMyRegion(uint arg0, uint arg1)
{
	// for debugging
	printImgInfo();

	ushort numLines, remLines;
	ushort startLineBlock, endLineBlock;		// chip/block level
	ushort startLineSubBlock, endLineSubBlock;	// core level

	// first, compute chip/block-wide
	numLines = blkInfo.hImg / blkInfo.maxBlock;
	remLines = blkInfo.hImg % blkInfo.maxBlock;
	startLineBlock = blkInfo.nodeBlockID * numLines;
	// if my block is the last one, then I might have additional lines
	if(blkInfo.nodeBlockID==blkInfo.maxBlock-1)
		endLineBlock = blkInfo.hImg - 1;
	else
		endLineBlock = startLineBlock + numLines-1;

	// then, compute core-wide
	numLines = (endLineBlock+1 - startLineBlock) / workers.tAvailable;
	remLines = (endLineBlock+1 - startLineBlock) % workers.tAvailable;
	startLineSubBlock = blkInfo.subBlockID * numLines;
	endLineSubBlock = startLineSubBlock + numLines - 1;
	if(blkInfo.subBlockID==workers.tAvailable-1)
		endLineSubBlock += remLines;

	// for debugging
	io_printf(IO_BUF, "My startLine = %d, endLine = %d\n", startLineSubBlock, endLineSubBlock);
}

void printImgInfo()
{
	io_printf(IO_BUF, "Image w = %d, h = %d, ", blkInfo.wImg, blkInfo.hImg);
	if(blkInfo.isGrey==1)
		io_printf(IO_BUF, "grascale, ");
	else
		io_printf(IO_BUF, "color, ");
	switch(blkInfo.opType) {
	case IMG_SOBEL:		io_printf(IO_BUF, "for sobel "); break;
	case IMG_LAPLACE:	io_printf(IO_BUF, "for laplace "); break;
	}
	switch(blkInfo.opFilter) {
	case IMG_NO_FILTERING:		io_printf(IO_BUF, "without filtering\n"); break;
	case IMG_WITH_FILTERING:	io_printf(IO_BUF, "with filtering\n"); break;
	}
	io_printf(IO_BUF, "nodeBlockID = %d with maxBlock = %d\n",
			  blkInfo.nodeBlockID, blkInfo.maxBlock);
}

void printWorkerIDInfo(uint arg0, uint arg1)
{
	io_printf(IO_BUF, "My working block ID is %d\n", blkInfo.subBlockID);
}

//void printMCPLinfo(uint key, uint payload)
//{
//	io_printf(IO_BUF, "Got key-0x%x with payload-0x%x\n", blkInfo.subBlockID);
//}
