#include "SpiNNEdge.h"

void hTimer(uint tick, uint Unused)
{
	io_printf(IO_STD, "tick-%d\n", tick);
	// debugging MCPL
	if(tick==5) {

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

			// just debugging:
			io_printf(IO_BUF, "Image w = %d, h = %d, ", blkInfo.wImg, blkInfo.hImg);
			if(blkInfo.isGrey==1)
				io_printf(IO_BUF, "grascale, ");
			else
				io_printf(IO_BUF, "color, ");
			switch(msg->seq & 0xFF) {
			case IMG_OP_SOBEL_NO_FILT:
				io_printf(IO_BUF, "for sobel without filtering\n");
				break;
			case IMG_OP_SOBEL_WITH_FILT:
				io_printf(IO_BUF, "for sobel with filtering\n");
				break;
			case IMG_OP_LAP_NO_FILT:
				io_printf(IO_BUF, "for laplace without filtering\n");
				break;
			case IMG_OP_LAP_WITH_FILT:
				io_printf(IO_BUF, "for laplace with filtering\n");
				break;
			}
			io_printf(IO_BUF, "nodeBlockID = %d with maxBlock = %d\n",
					  blkInfo.nodeBlockID, blkInfo.maxBlock);
		}
		else if(msg->cmd_rc == SDP_CMD_PROCESS) {
			// if filtering is required, it should be proceeded first
			if(blkInfo.opFilter==IMG_WITH_FILTERING)
				spin1_schedule_callback(triggerWorker,CMD_FILTERING,0,PRIORITY_PROCESSING);
			else
				spin1_schedule_callback(triggerWorker,CMD_DETECTION,0,PRIORITY_PROCESSING);
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
	e = rtr_alloc(5);
	if(e==0) {
		rt_error(RTE_ABORT);
	} else {
		allRoute &= ~leader;
		rtr_mc_set(e, MCPL_BCAST_CMD_KEY, 0xFFFFFFFF, allRoute);
		rtr_mc_set(e+1, MCPL_BCAST_FILT_KEY, 0xFFFFFFFF, allRoute);
		rtr_mc_set(e+2, MCPL_BCAST_INFO_KEY, 0xFFFFFFFF, allRoute);
		rtr_mc_set(e+3, MCPL_INFO_TO_LEADER, 0xFFFFFFFF, leader);
		rtr_mc_set(e+4, MCPL_FLAG_TO_LEADER, 0xFFFFFFFF, leader);
	}
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

