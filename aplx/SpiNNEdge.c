#include <spin1_api.h>
#include "SpiNNEdge.h"

void hDMADone(uint tid, uint tag)
{

}


void hMCPL(uint key, uint payload)
{
/*
	// this is worker's part
	if(key==MCPL_BCAST_INFO_KEY) {
		// leadAp sends "0" for ping, worker replys with its core
		// withouth any additional info in the payload part
		// the additional info can be used, eg., notifying leadAp
		// that the worker has finished its task
		if(payload==0) {
			spin1_send_mc_packet(MCPL_INFO_TO_LEADER, myCoreID, WITH_PAYLOAD);
		}
		else {
			blkInfo.wImg = payload >> 16;
			blkInfo.hImg = payload && 0xFFFF;
		}
	}
	// myCoreID is the key-signal to start either imgFiltering() or imgProcessing()
	else if(key==myCoreID) {
		//fetch this: [cmd_Type(4), isGray(4), opType(8), subBlockID(8), mainBlockID(8)]
		//first, check if this is only for filtering
		blkInfo.nodeBlockID = payload & 0xFF;
		blkInfo.subBlockID = (payload >> 8) & 0xFF;
		if((payload >> 28) == CMD_FILTERING) {
			spin1_schedule_callback(imgFiltering,0,0,PRIORITY_PROCESSING);
		// note: once filtering is complete, leadAp must copy FIL_IMG to ORG_IMG
		// before continue with edge detection!!!
		} else {
			//otherwise, this is the command for edge detection
			blkInfo.opType = (payload >> 16) & 0xFF;
			blkInfo.isGrey = (payload >> 24) & 0xF;
			spin1_schedule_callback(imgProcessing,0,0,PRIORITY_PROCESSING);
		}
	}
	// this is leadAp part
	else if(key==MCPL_INFO_TO_LEADER) {
		workers.wID[workers.available] = payload;	// this will be automatically mapped to workers.subBlockID[workers.available]
		workers.available++;
	}
	// workers send MCPL_FLAG_TO_LEADER after finishing either
	// filtering or edge detection
	else if(key==MCPL_FLAG_TO_LEADER) {
		if(payload==FLAG_FILTERING_DONE) {
			nJobDone++;
			if(nJobDone==workers.available)
				// before detection, move FIL_IMG to ORG_IMG
				spin1_schedule_callback(triggerPreProcessing, 0, 0, PRIORITY_PROCESSING);
		}
	}
*/
}

void pingWorkers(uint arg0, uint arg1)
{
	spin1_send_mc_packet(MCPL_BCAST_INFO_KEY, 0, WITH_PAYLOAD);
}

// how to carry subBlockID and other info for core in MCPL?
// using MCPL_BCAST_INFO_KEY, send [wImg, hImg]
// using particular core key for triggering filtering or detection, send:
//		[cmd_Type(4), isGray(4), opType(8), subBlockID(8), mainBlockID(8)]
// if leadAp detects filtering requirement, it ask workers
// to do filtering prior to edge detection. Leadap will
// wait until all workers reporting "done" on filtering!
void triggerWorker(uint cmd_Type, uint arg1)
{
	uint i, info, subBlockID, key_core;

	nJobDone = 0;	// initialize counter

	// send notification to all workers about image info (width and height)
	info = (blkInfo.wImg << 16) + blkInfo.hImg;
	spin1_send_mc_packet(MCPL_BCAST_INFO_KEY, info, WITH_PAYLOAD);

	// let's the worker prepare the next command
	spin1_delay_us(10);

	// send notification to all workers (except myself) about their part
	// and inform them to perform particular operation:
	//		[isGray(4), isFiltering(4), isSobel(8), subBlockID(8), mainBlockID(8)]
	for(i=1; i<workers.available; i++) {
		key_core = workers.wID[i];
		subBlockID = workers.subBlockID[i];
//		info = (cmd_Type << 28) + (blkInfo.isGrey << 24) +
//				(blkInfo.opType << 16) + (blkInfo.subBlockID << 8) + blkInfo.nodeBlockID;
		spin1_send_mc_packet(key_core, info, WITH_PAYLOAD);
	}
	// once the worker core receives key_core, it start the imgProcessing immediately

	// then start my own processing
	if(cmd_Type==CMD_FILTERING)
		imgFiltering(0,0);
	else
		imgProcessing(0,0);
}

// triggerPreProcessing() will inform workers, where the imgIn and imgOut
void triggerPreProcessing(uint arg0, uint arg1)
{

}

void imgFiltering(uint arg0, uint arg1)
{

	// note: once we're done with filtering, report to leadAp
	// so that the leadAp can move FIL_IMG to ORG_IMG
	if(leadAp) {
		nJobDone++;
		if(nJobDone==workers.available)
			spin1_schedule_callback(triggerPreProcessing, 0, 0, PRIORITY_PROCESSING);
	}
}

void imgProcessing(uint arg0, uint arg1)
{
	spin1_callback_off(TIMER_TICK);

	// at this point, each core holds information about its responsibility region
	// step-1: compute my part

	spin1_callback_on(TIMER_TICK, hTimer, PRIORITY_TIMER);
}

void c_main()
{
	initImage();
	myCoreID = sark_core_id();
	io_printf(IO_STD, "SpiNNEdge running @ core-%d id-%d\n",
			  sark_core_id(), sark_app_id());
	spin1_callback_on(MCPL_PACKET_RECEIVED, hMCPL, PRIORITY_MCPL);
	// only leadAp has access to dma and sdp
	if(leadAp) {
		spin1_set_timer_tick(TIMER_TICK_PERIOD_US);
		spin1_callback_on(TIMER_TICK, hTimer, PRIORITY_TIMER);
		workers.wID[0] = myCoreID;
		workers.available = 1;
		workers.subBlockID[0] = 0;
//		blkInfo.subBlockID = 0;	// the subBlockID of leadAP is always 0
		for(uint i=1; i<17; i++) workers.subBlockID[i] = i;
		spin1_callback_on(DMA_TRANSFER_DONE, hDMADone, PRIORITY_DMA);
		initSDP();
		initRouter();
	}
	// first, notify workers that leadAP is ready and collect their IDs
	// this mechanism should finish in less than 1s
	spin1_schedule_callback(pingWorkers, 0,0,PRIORITY_MCPL);
	spin1_start(SYNC_NOWAIT);
}
