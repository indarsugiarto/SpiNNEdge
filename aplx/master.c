#include "SpiNNEdge.h"

void hTimer(uint tick, uint Unused)
{
	if(tick==1) {
		io_printf(IO_STD, "Collecting all workers ID\n");
		spin1_schedule_callback(pingWorkers, 0,0,PRIORITY_PROCESSING);
	}
	// after 3 ticks, all workers should have reported
	else if(tick==3) {
		io_printf(IO_STD, "Total workers: %d\n", workers.tAvailable);
		for(int i=0; i<workers.tAvailable; i++) {
			io_printf(IO_STD, "worker-%d is core-%d\n", i, workers.wID[i]);
		}
	}
	else if(tick==5) {
		io_printf(IO_STD, "Distributing workload...\n");
		// send worker's block ID, except to leadAp
		uint check, key, payload;
		for(uint i=1; i<workers.tAvailable; i++) {
			key = workers.wID[i];
			payload = workers.tAvailable << 16;
			payload += i;
			check = spin1_send_mc_packet(key, payload, WITH_PAYLOAD);
			if(check==SUCCESS)
				io_printf(IO_BUF, "Sending id-%d to core-%d\n", payload, key);
			else
				io_printf(IO_BUF, "Fail sending id-%d to core-%d\n", payload, key);
		}

		io_printf(IO_STD, "Ready for image...\n");
		spin1_callback_off(TIMER_TICK);
	}
}


/*
// Let's use:
// port-7 for receiving configuration and command:
//		cmd_rc == SDP_CMD_CONFIG -->
//			arg1.high == img_width, arg1.low == img_height
//			arg2.high : 0==gray, 1==colour(rgb)
//			seq.high == isGray: 1==gray, 0==colour(rgb)
//			seq.low == type_of_operation:
//						1 = sobel_no_filtering
//						2 = sobel_with_filtering
//						3 = laplace_no_filtering
//						4 = laplace_with_filtering
//		cmd_rc == SDP_CMD_PROCESS
//		cmd_rc == SDP_CMD_CLEAR
If leadAp detects filtering, first it ask workers to do filtering only.
Then each worker will start filtering. Once finished, each worker
will report to leadAp about this filtering.
Once leadAp received filtering report from all workers, leadAp
trigger the edge detection
*/
void hSDP(uint mBox, uint port)
{
	sdp_msg_t *msg = (sdp_msg_t *)mBox;
	// if host send SDP_CONFIG, means the image has been
	// loaded into sdram via ybug operation
	if(port==SDP_PORT_CONFIG) {
		if(msg->cmd_rc == SDP_CMD_CONFIG) {
			blkInfo.isGrey = msg->seq >> 8; //1=Gray, 0=color
			blkInfo.wImg = msg->arg1 >> 16;
			blkInfo.hImg = msg->arg1 & 0xFFFF;
			blkInfo.nodeBlockID = msg->arg2 >> 16;
			blkInfo.maxBlock = msg->arg2 & 0xFF;
			switch(msg->seq & 0xFF) {
			case IMG_OP_SOBEL_NO_FILT:
				blkInfo.opFilter = IMG_NO_FILTERING; blkInfo.opType = IMG_SOBEL;
				break;
			case IMG_OP_SOBEL_WITH_FILT:
				blkInfo.opFilter = IMG_WITH_FILTERING; blkInfo.opType = IMG_SOBEL;
				break;
			case IMG_OP_LAP_NO_FILT:
				blkInfo.opFilter = IMG_NO_FILTERING; blkInfo.opType = IMG_LAPLACE;
				break;
			case IMG_OP_LAP_WITH_FILT:
				blkInfo.opFilter = IMG_WITH_FILTERING; blkInfo.opType = IMG_LAPLACE;
				break;
			}
			imageInfoRetrieved = 1;

			// and then distribute to all workers
			spin1_schedule_callback(infoSzImgWorkers, 0, 0, PRIORITY_PROCESSING);
			spin1_schedule_callback(infoBlkImgWorkers, 0, 0, PRIORITY_PROCESSING);
			// after receiving block info, workers will compute their working region

			// for leadAp, it should be implicitely instructed here:
			spin1_schedule_callback(computeMyRegion, 0, 0, PRIORITY_PROCESSING);
		}
		else if(msg->cmd_rc == SDP_CMD_PROCESS) {
			/*
			// if filtering is required, it should be proceeded first
			if(blkInfo.opFilter==IMG_WITH_FILTERING)
				spin1_schedule_callback(triggerWorker,CMD_FILTERING,0,PRIORITY_PROCESSING);
			else
				spin1_schedule_callback(triggerWorker,CMD_DETECTION,0,PRIORITY_PROCESSING);
			*/
		}
		else if(msg->cmd_rc == SDP_CMD_CLEAR) {
			initImage();
		}
	}
	// if host send images
	// NOTE: what if we use arg part of SCP for image data? OK let's try, because see HOW.DO...
	else if(port==SDP_PORT_R_IMG_DATA) {
		// get the image data from SCP+data_part
		ushort dLen = msg->length - sizeof(sdp_hdr_t);
		//io_printf(IO_BUF, "Receiving %d-bytes\n", dLen);

		if(dLen > 0) {

			spin1_memcpy((void *)imgRIn, (void *)&msg->cmd_rc, dLen);
			imgRIn += dLen;

			// send reply
			replyMsg->cmd_rc = SDP_CMD_REPLY_HOST_SEND_IMG;
			replyMsg->seq = dLen;
			spin1_send_sdp_msg(replyMsg, 10);
		}
		// if zero data is sent, means the end of message transfer!
		else {
			io_printf(IO_STD, "layer-R is complete!\n");
			imgRIn = (char *)IMG_R_BUFF0_BASE;	// reset to initial base position
			fullRImageRetrieved = 1;
		}
	}
	else if(port==SDP_PORT_G_IMG_DATA) {
		// get the image data from SCP+data_part
		ushort dLen = msg->length - sizeof(sdp_hdr_t);
		//io_printf(IO_BUF, "Receiving %d-bytes\n", dLen);

		if(dLen > 0) {

			spin1_memcpy((void *)imgGIn, (void *)&msg->cmd_rc, dLen);
			imgGIn += dLen;

			// send reply
			replyMsg->cmd_rc = SDP_CMD_REPLY_HOST_SEND_IMG;
			replyMsg->seq = dLen;
			spin1_send_sdp_msg(replyMsg, 10);
		}
		// if zero data is sent, means the end of message transfer!
		else {
			io_printf(IO_STD, "layer-G is complete!\n");
			imgGIn = (char *)IMG_G_BUFF0_BASE;	// reset to initial base position
			fullGImageRetrieved = 1;
		}
	}
	else if(port==SDP_PORT_B_IMG_DATA) {
		// get the image data from SCP+data_part
		ushort dLen = msg->length - sizeof(sdp_hdr_t);
		//io_printf(IO_BUF, "Receiving %d-bytes\n", dLen);

		if(dLen > 0) {

			spin1_memcpy((void *)imgBIn, (void *)&msg->cmd_rc, dLen);
			imgBIn += dLen;

			// send reply
			replyMsg->cmd_rc = SDP_CMD_REPLY_HOST_SEND_IMG;
			replyMsg->seq = dLen;
			spin1_send_sdp_msg(replyMsg, 10);
		}
		// if zero data is sent, means the end of message transfer!
		else {
			io_printf(IO_STD, "layer-B is complete!\n");
			imgBIn = (char *)IMG_B_BUFF0_BASE;	// reset to initial base position
			fullBImageRetrieved = 1;
		}
	}
	spin1_msg_free(msg);
}

void initSDP()
{
	spin1_callback_on(SDP_PACKET_RX, hSDP, PRIORITY_SDP);
	replyMsg->flags = 0x07;	//no reply
	replyMsg->tag = SDP_TAG_REPLY;
	replyMsg->srce_port = SDP_PORT_CONFIG;
	replyMsg->srce_addr = sv->p2p_addr;
	replyMsg->dest_port = PORT_ETH;
	replyMsg->dest_addr = sv->eth_addr;
	replyMsg->length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t);
}

/* initRouter() initialize MCPL routing table by leadAp. Use two keys:
 * MCPL_BCAST_KEY and MCPL_TO_LEADER
 * */
void initRouter()
{
	uint allRoute = 0xFFFF80;	// excluding core-0 and external links
	uint leader = (1 << (myCoreID+6));
	uint e = rtr_alloc(17);
	if(e==0) {
		rt_error(RTE_ABORT);
	} else {
		// each destination core might have its own key association
		// so that leadAp can tell each worker, which region is their part
		for(uint i=0; i<17; i++)
			// starting from core-1 up to core-17
			rtr_mc_set(e+i, i+1, 0xFFFFFFFF, (MC_CORE_ROUTE(i+1)));
	}
	// then add another 4 generic keys
	e = rtr_alloc(7);
	if(e==0) {
		rt_error(RTE_ABORT);
	} else {
		allRoute &= ~leader;
		rtr_mc_set(e, MCPL_BCAST_CMD_KEY, 0xFFFFFFFF, allRoute);
		rtr_mc_set(e+1, MCPL_BCAST_FILT_KEY, 0xFFFFFFFF, allRoute);
		rtr_mc_set(e+2, MCPL_BCAST_INFO_KEY, 0xFFFFFFFF, allRoute);
		rtr_mc_set(e+3, MCPL_INFO_TO_LEADER, 0xFFFFFFFF, leader);
		rtr_mc_set(e+4, MCPL_FLAG_TO_LEADER, 0xFFFFFFFF, leader);
		rtr_mc_set(e+5, MCPL_BCAST_SZIMG_KEY, 0xFFFFFFFF, allRoute);
		rtr_mc_set(e+6, MCPL_BCAST_BLK_KEY, 0xFFFFFFFF, allRoute);
	}
}

void initIPTag()
{
	uint tagNum = SDP_TAG_REPLY;
	uint udpPort = SDP_UDP_REPLY_PORT;
	sdp_msg_t iptag;
	iptag.flags = 0x07;	// no replay
	iptag.tag = 0;		// send internally
	iptag.cmd_rc = 26;
	iptag.seq = 0;
	iptag.arg1 = (1 << 16) + tagNum;	// iptag '1'
	iptag.arg2 = udpPort;
	iptag.arg3 = 0x02F0A8C0;	// for 192.168.240.2
	iptag.srce_addr = sv->p2p_addr;
	iptag.srce_port = (1 << 5) + myCoreID;
	iptag.dest_addr = 0;
	iptag.dest_port = 0;
	iptag.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t);
	spin1_send_sdp_msg(&iptag, 10);
}

void initImage()
{
	imageInfoRetrieved = 0;
	fullRImageRetrieved = 0;
	fullGImageRetrieved = 0;
	fullBImageRetrieved = 0;
	imgRIn = (uchar *)IMG_R_BUFF0_BASE;
	imgGIn = (uchar *)IMG_G_BUFF0_BASE;
	imgBIn = (uchar *)IMG_B_BUFF0_BASE;
	imgROut = (uchar *)IMG_R_BUFF1_BASE;
	imgGOut = (uchar *)IMG_G_BUFF1_BASE;
	imgBOut = (uchar *)IMG_B_BUFF1_BASE;
}

void pingWorkers(uint arg0, uint arg1)
{
	uint check = spin1_send_mc_packet(MCPL_BCAST_INFO_KEY, 0, WITH_PAYLOAD);
	if(check==SUCCESS)
		io_printf(IO_BUF, "Ping sent!\n");
	else
		io_printf(IO_BUF, "Ping fail!\n");
}


/* Info that will be broadcast to worker
	g ->	blkInfo.isGrey = msg->seq >> 8; //1=Gray, 0=color
	wwww ->	blkInfo.wImg = msg->arg1 >> 16;
	hhhh ->	blkInfo.hImg = msg->arg1 & 0xFFFF;
	bb ->	blkInfo.nodeBlockID = msg->arg2 >> 16;
	mm ->	blkInfo.maxBlock = msg->arg2 & 0xFF;
 * So, the format for payload will be:
 * wwwwhhhh for infoSzImgWorkers()
 * 000gbbmm for infoBlkImgWorkers()
 * */
void infoSzImgWorkers(uint arg0, uint arg1)
{
	uint info = blkInfo.wImg << 16;
	info += blkInfo.hImg;
	spin1_send_mc_packet(MCPL_BCAST_SZIMG_KEY, info, WITH_PAYLOAD);
}

void infoBlkImgWorkers(uint arg0, uint arg1)
{
	uint info = blkInfo.isGrey << 16;
	info += (blkInfo.nodeBlockID << 8);
	info += blkInfo.maxBlock;
	spin1_send_mc_packet(MCPL_BCAST_BLK_KEY, info, WITH_PAYLOAD);
}
