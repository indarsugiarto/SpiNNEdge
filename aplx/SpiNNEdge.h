#ifndef SPINNEDGE_H
#define SPINNEDGE_H

#include <spin1_api.h>

/* 3x3 GX and GY Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
static const uint GX[3][3] = {{-1,0,1},
				 {-2,0,2},
				 {-1,0,1}};
static const uint GY[3][3] = {{1,2,1},
				 {0,0,0},
				 {-1,-2,-1}};

/* Laplace operator: 5x5 Laplace mask.  Ref: Myler Handbook p. 135 */
static const uint LAP[5][5] = {{-1,-1,-1,-1,-1},
				  {-1,-1,-1,-1,-1},
				  {-1,-1,24,-1,-1},
				  {-1,-1,-1,-1,-1},
				  {-1,-1,-1,-1,-1}};

/* Gaussian filter. Ref:  en.wikipedia.org/wiki/Canny_edge_detector */
static const uint FILT[5][5] = {{2,4,5,4,2},
				   {4,9,12,9,4},
				   {5,12,15,12,5},
				   {4,9,12,9,4},
				   {2,4,5,4,2}};
static const uint FILT_DENOM = 159;

#define TIMER_TICK_PERIOD_US 	1000000
#define PRIORITY_TIMER			4
#define PRIORITY_PROCESSING		3
#define PRIORITY_SDP			2
#define PRIORITY_MCPL			0
#define PRIORITY_DMA			1

#define SDP_TAG_REPLY			1
#define SDP_UDP_REPLY_PORT		20000

#define SDP_PORT_R_IMG_DATA		1
#define SDP_PORT_G_IMG_DATA		2
#define SDP_PORT_B_IMG_DATA		3
#define SDP_PORT_CONFIG			7
#define SDP_CMD_CONFIG			1	// will be sent via SDP_PORT_CONFIG
#define SDP_CMD_PROCESS			2	// will be sent via SDP_PORT_CONFIG
#define SDP_CMD_CLEAR			3	// will be sent via SDP_PORT_CONFIG
#define SDP_CMD_HOST_SEND_IMG	0x1234
#define SDP_CMD_REPLY_HOST_SEND_IMG	0x4321
#define IMG_OP_SOBEL_NO_FILT	1	// will be carried in arg2.low
#define IMG_OP_SOBEL_WITH_FILT	2	// will be carried in arg2.low
#define IMG_OP_LAP_NO_FILT		3	// will be carried in arg2.low
#define IMG_OP_LAP_WITH_FILT	4	// will be carried in arg2.low
#define IMG_NO_FILTERING		0
#define IMG_WITH_FILTERING		1
#define IMG_SOBEL				0
#define IMG_LAPLACE				1

#define IMG_R_BUFF0_BASE		0x61000000
#define IMG_G_BUFF0_BASE		0x62000000
#define IMG_B_BUFF0_BASE		0x63000000
#define IMG_R_BUFF1_BASE		0x64000000
#define IMG_G_BUFF1_BASE		0x65000000
#define IMG_B_BUFF1_BASE		0x66000000

//we also has direct key to each core (see initRouter())
#define MCPL_BCAST_CMD_KEY		0xbca5c00d  // command for edge detection
#define MCPL_BCAST_INFO_KEY		0xbca514f0	// for ping
#define MCPL_BCAST_SZIMG_KEY	0xbca52219	// for broadcasting image size
#define MCPL_BCAST_BLK_KEY		0xbca5b10c  // for broadcasting block info
#define MCPL_BCAST_FILT_KEY		0xbca5c44d	// command for filtering only
#define MCPL_INFO_TO_LEADER		0x14f01ead
#define MCPL_FLAG_TO_LEADER		0xf1a61ead	// worker send signal to leader
#define FLAG_FILTERING_DONE		0xf117
#define FLAG_DETECTION_DONE		0xde7c

#define CMD_FILTERING			0x12
#define CMD_DETECTION			0x21

typedef struct block_info {
	// part that will pre-collected by leadAp
	ushort wImg;
	ushort hImg;
	ushort isGrey;
	uchar opType;			// 0==sobel, 1==laplace
	uchar opFilter;			// 0==no filtering, 1==with filtering
	ushort nodeBlockID;		// will be send by host
	ushort maxBlock;		// will be send by host
	// part that will be filled by workers (including leadAp as worker-0)
	ushort subBlockID;		// closely related with wID in w_info_t
} block_info_t;

// worker info
typedef struct w_info {
	uchar wID[17];			// this is the coreID, maximum worker is 17
	//uchar subBlockID[17];	// just a helper, this maps subBlockID of workers
	uchar tAvailable;		// how many workers? should be initialized to 1
} w_info_t;


/* Due to filtering mechanism, the address of image might be changed.
 * Scenario:
 * 1. Without filtering:
 *		imgRIn will start from IMG_R_BUFF0_BASE and resulting in IMG_R_BUFF1_BASE
 *		imgGIn will start from IMG_G_BUFF0_BASE and resulting in IMG_G_BUFF1_BASE
 *		imgBIn will start from IMG_B_BUFF0_BASE and resulting in IMG_B_BUFF1_BASE
 * 2. Aftering filtering:
 *		imgRIn will start from IMG_R_BUFF1_BASE and resulting in IMG_R_BUFF0_BASE
 *		imgGIn will start from IMG_G_BUFF1_BASE and resulting in IMG_G_BUFF0_BASE
 *		imgBIn will start from IMG_B_BUFF1_BASE and resulting in IMG_B_BUFF0_BASE
 *
*/
uchar *imgRIn;
uchar *imgGIn;
uchar *imgBIn;
uchar *imgROut;
uchar *imgGOut;
uchar *imgBOut;

uchar imageInfoRetrieved;
uchar fullRImageRetrieved;
uchar fullGImageRetrieved;
uchar fullBImageRetrieved;

// for dma operation
#define DMA_FETCH_IMG_TAG		0x14
#define DMA_STORE_IMG_TAG		0x41
uint szDMA;
#define DMA_MOVE_IMG_R			0x52
#define DMA_MOVE_IMG_G			0x47
#define DMA_MOVE_IMG_B			0x42

uint myCoreID;
w_info_t workers;			// will be held by leadAp

block_info_t blkInfo;
uchar nJobDone;				// will be used to count how many workers have
							// finished their job in either filtering or edge detection

sdp_msg_t *replyMsg;

// forward declaration
void triggerWorker(uint arg0, uint arg1);
void triggerPreProcessing(uint arg0, uint arg1);	// after filterning, leadAp needs to copy
													// the content from FIL_IMG to ORG_IMG
void imgProcessing(uint arg0, uint arg1);	// this might be the most intense function
void imgFiltering(uint arg0, uint arg1);	// this is separate operation from edge detection
void initSDP();
void initRouter();
void initImage();
void initIPTag();
void hDMADone(uint tid, uint tag);
void hTimer(uint tick, uint Unused);
void hMCPL(uint key, uint payload);
void pingWorkers(uint arg0, uint arg1);
void infoSzImgWorkers(uint arg0, uint arg1);
void infoBlkImgWorkers(uint arg0, uint arg1);
void imgFiltering(uint arg0, uint arg1);
void imgProcessing(uint arg0, uint arg1);
void computeMyRegion(uint arg0, uint arg1);
void printImgInfo();
void printWorkerIDInfo(uint arg0, uint arg1);

/*--------------------------------------- IMPLEMENTATIONS --------------------------------------*/

// PrepareMemImage() will allocate and deallocate sdram.
// It should be handled by one core (ie. leadAp) in a chip.


#endif // SPINNEDGE_H

