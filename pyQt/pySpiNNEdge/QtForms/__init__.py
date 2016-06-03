"""
The idea is:
- Use QImage for direct pixel manipulation and use QPixmap for rendering
- http://www.doc.gold.ac.uk/~mas02fl/MSC101/ImageProcess/edge.html
"""
from PyQt4 import Qt, QtGui, QtCore, QtNetwork
import mainGUI
import math
import os
import struct
import socket

# Sobel operator
"""
 /* 3x3 GX Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
   GX[0][0] = -1; GX[0][1] = 0; GX[0][2] = 1;
   GX[1][0] = -2; GX[1][1] = 0; GX[1][2] = 2;
   GX[2][0] = -1; GX[2][1] = 0; GX[2][2] = 1;

   /* 3x3 GY Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
   GY[0][0] =  1; GY[0][1] =  2; GY[0][2] =  1;
   GY[1][0] =  0; GY[1][1] =  0; GY[1][2] =  0;
   GY[2][0] = -1; GY[2][1] = -2; GY[2][2] = -1;
"""
GX = [[-1,0,1],[-2,0,2],[-1,0,1]]
GY = [[1,2,1],[0,0,0],[-1,-2,-1]]

# Laplace operator
"""
   /* 5x5 Laplace mask.  Ref: Myler Handbook p. 135 */
   MASK[0][0] = -1; MASK[0][1] = -1; MASK[0][2] = -1; MASK[0][3] = -1; MASK[0][4] = -1;
   MASK[1][0] = -1; MASK[1][1] = -1; MASK[1][2] = -1; MASK[1][3] = -1; MASK[1][4] = -1;
   MASK[2][0] = -1; MASK[2][1] = -1; MASK[2][2] = 24; MASK[2][3] = -1; MASK[2][4] = -1;
   MASK[3][0] = -1; MASK[3][1] = -1; MASK[3][2] = -1; MASK[3][3] = -1; MASK[3][4] = -1;
   MASK[4][0] = -1; MASK[4][1] = -1; MASK[4][2] = -1; MASK[4][3] = -1; MASK[4][4] = -1;
"""
LAP = [[-1,-1,-1,-1,-1],
       [-1,-1,-1,-1,-1],
       [-1,-1,24,-1,-1],
       [-1,-1,-1,-1,-1],
       [-1,-1,-1,-1,-1]]

# Gaussian filter
FILT = [[2,4,5,4,2],
        [4,9,12,9,4],
        [5,12,15,12,5],
        [4,9,12,9,4],
        [2,4,5,4,2]]
FILT_DENOM = 159


IMG_R_BUFF0_BASE = 0x61000000
IMG_G_BUFF0_BASE = 0x62000000
IMG_B_BUFF0_BASE = 0x63000000
DEF_HOST = '192.168.240.253'
DEF_SEND_PORT = 17893 #tidak bisa diganti dengan yang lain
DEF_REPLY_PORT = 20000
chipX = 0
chipY = 0
sdpImgRedPort = 1
sdpImgGreenPort = 2
sdpImgBluePort = 3
sdpImgConfigPort = 7
SDP_CMD_CONFIG = 1
SDP_CMD_PROCESS = 2
SDP_CMD_CLEAR = 3
IMG_OP_SOBEL_NO_FILT	= 1	# will be carried in arg2.low
IMG_OP_SOBEL_WITH_FILT	= 2	# will be carried in arg2.low
IMG_OP_LAP_NO_FILT		= 3	# will be carried in arg2.low
IMG_OP_LAP_WITH_FILT	= 4	# will be carried in arg2.low
IMG_NO_FILTERING		= 0
IMG_WITH_FILTERING		= 1
IMG_SOBEL				= 0
IMG_LAPLACE				= 1

sdpCore = 1

class edgeGUI(QtGui.QWidget, mainGUI.Ui_pySpiNNEdge):
    def __init__(self, def_image_dir = "./", parent=None):
        self.img_dir = def_image_dir
        self.img = None
        super(edgeGUI, self).__init__(parent)
        self.setupUi(self)
        self.connect(self.pbLoad, QtCore.SIGNAL("clicked()"), QtCore.SLOT("pbLoadClicked()"))
        self.connect(self.pbSend, QtCore.SIGNAL("clicked()"), QtCore.SLOT("pbSendClicked()"))
        self.connect(self.pbProcess, QtCore.SIGNAL("clicked()"), QtCore.SLOT("pbProcessClicked()"))
        self.connect(self.pbSave, QtCore.SIGNAL("clicked()"), QtCore.SLOT("pbSaveClicked()"))
        self.connect(self.pbClear, QtCore.SIGNAL("clicked()"), QtCore.SLOT("pbClearClicked()"))


    @QtCore.pyqtSlot()
    def pbLoadClicked(self):
        self.fName = QtGui.QFileDialog.getOpenFileName(parent=self, caption="Load an image from file", \
                                                       directory = self.img_dir, filter = "*.*")
        #print self.fName    #this will contains the full path as well!
        if self.fName == "" or self.fName is None:
            return
        self.img = QtGui.QImage(self)
        self.img.load(self.fName)
        self.w = self.img.width()
        self.h = self.img.height()
        self.d = self.img.depth()
        self.c = self.img.colorCount()
        self.isGray = self.img.isGrayscale()
        print "Image width = {}, height = {}, depth = {}, clrCntr = {}".format(self.w,self.h,self.d,self.c)
        # For 16bpp or 32bpp (which is non-indexed), clrCntr is always 0

        # Convert to 32-bit format
        self.img32 = self.img.convertToFormat(QtGui.QImage.Format_RGB32)

        # Idea: use QImage for direct pixel manipulation and use QPixmap for rendering
        self.pixmap = QtGui.QPixmap(self)
        #self.pixmap.fromImage(self.img)    #something not right with this fromImage() function
        self.pixmap.convertFromImage(self.img32)
        #self.pixmap.load(self.fName) #pixmap.load() works just fine
        self.scene = QtGui.QGraphicsScene()
        self.scene.addPixmap(self.pixmap)
        self.scene.addText("Original Image")
        self.viewOrg.setScene(self.scene)
        self.viewOrg.show()

        # Prepare the resulting scene
        self.resultScene = QtGui.QGraphicsScene()
        self.viewEdge.setScene(self.resultScene)
        self.viewEdge.show()

        # Then create 3maps for each color. The sobel/laplace operators will operate on these maps
        self.rmap0 = [[0 for _ in range(self.w)] for _ in range(self.h)]    # the original
        self.rmap1 = [[0 for _ in range(self.w)] for _ in range(self.h)]    # the result
        self.gmap0 = [[0 for _ in range(self.w)] for _ in range(self.h)]
        self.gmap1 = [[0 for _ in range(self.w)] for _ in range(self.h)]
        self.bmap0 = [[0 for _ in range(self.w)] for _ in range(self.h)]
        self.bmap1 = [[0 for _ in range(self.w)] for _ in range(self.h)]

        for x in range(self.w):
            for y in range(self.h):
                # jika pakai r = Qt.QColor.red(va) akan muncul error:
                # TypeError: QColor.red(): first argument of unbound method must have type 'QColor'
                # jadi memang harus di-instantiate!
                rgb = Qt.QColor(self.img32.pixel(x,y))
                self.rmap0[y][x] = rgb.red()    # remember, NOT rmap[x][y] -> because...
                self.gmap0[y][x] = rgb.green()
                self.bmap0[y][x] = rgb.blue()

        print "Image is loaded!"


    @QtCore.pyqtSlot()
    def pbSendClicked(self):
        print "Send button clicked..."
        #TODO:  1. save the rgbmap to a dummy binary file -> use pbSaveClicked()
        #       2. send to all chips using ybug->sload
        #       3. test by downloading using ybug->sdump
        if self.img is None:
            return

        #first, prepare SDP container
        udpSock = QtNetwork.QUdpSocket()
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.bind( ('', DEF_REPLY_PORT) )     #'' sepertinya berarti HostAddress::Any
        except OSError as msg:
            print "%s" % msg
            sock.close()
            return

        pad = struct.pack('2B',0,0)
        da = (chipX << 8) + chipY

        #send image information through SDP_PORT_CONFIG
        dpcc = (sdpImgConfigPort << 5) + sdpCore
        hdrc = struct.pack('4B2H',0x07,1,dpcc,255,da,0)
        cmd = SDP_CMD_CONFIG
        if self.isGray is True:
            isGray = 1
        else:
            isGray = 0
        if self.cbMode.currentIndex()==0 and self.cbFilter.isChecked() is False:
            opt = 1
        elif self.cbMode.currentIndex()==0 and self.cbFilter.isChecked() is True:
            opt = 2
        elif self.cbMode.currentIndex()==1 and self.cbFilter.isChecked() is False:
            opt = 3
        else:
            opt = 4
        seq = (isGray << 8) + opt
        arg1 = (self.w << 16) + self.h
        arg2 = 1    # will contain nodeBlockID and maxBlock, here we use only 1 chip
        scp = struct.pack('2H3I',cmd,seq,arg1,arg2,0)
        sdp = pad + hdrc + scp
        udpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)
        #this time, no need for reply

        #try sending R-layer (in future, grey mechanism must be considered)
        #TODO: in future, we might send accross several chips
        dpcr = (sdpImgRedPort << 5) + sdpCore
        dpcg = (sdpImgGreenPort << 5) + sdpCore
        dpcb = (sdpImgBluePort << 5) + sdpCore
        hdrr = struct.pack('4B2H',0x07,1,dpcr,255,da,0)
        hdrg = struct.pack('4B2H',0x07,1,dpcg,255,da,0)
        hdrb = struct.pack('4B2H',0x07,1,dpcb,255,da,0)

        #second, make a sequence
        rArray = list()
        for y in range(self.h):
            for x in range(self.w):
                rArray.append(self.rmap0[y][x])

        #third, iterate until all elements are sent
        remaining = len(rArray)
        stepping = 16+256   # initially, it's size is SCP+data_part
        stopping = stepping
        starting = 0
        while remaining > 0:
            chunk = rArray[starting:stopping]
            ba = bytearray(chunk)
            sdp = pad + hdrr + ba
            udpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)

            #then waiting for a reply
            reply = sock.recv(1024)
            remaining -= stepping
            starting = stopping

            if remaining < stepping:
                stepping = remaining
            stopping += stepping

        #fourth, send an empty data to signify end of image transfer
        sdp = pad + hdrr
        udpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)

        #check if the image is gray, if not, then send the green and blue layers
        if self.isGray is False:
            gArray = list()
            bArray = list()
            for y in range(self.h):
                for x in range(self.w):
                    gArray.append(self.gmap0[y][x])
                    bArray.append(self.bmap0[y][x])

            #iterate until all elements are sent
            remaining = len(gArray) #which is equal to len(bArray) as well
            stepping = 16+256   # initially, it's size is SCP+data_part
            stopping = stepping
            starting = 0
            while remaining > 0:

                chunk = gArray[starting:stopping]
                ba = bytearray(chunk)
                sdp = pad + hdrg + ba
                udpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)
                #then waiting for a reply from green channel before go to blue one
                reply = sock.recv(1024)

                chunk = bArray[starting:stopping]
                ba = bytearray(chunk)
                sdp = pad + hdrb + ba
                udpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)
                #again, wait for a reply from blue channel
                reply = sock.recv(1024)

                #repeat for the next chunk
                remaining -= stepping
                starting = stopping

                if remaining < stepping:
                    stepping = remaining
                stopping += stepping

            #finally, send an empty data to signify end of image transfer
            sdp = pad + hdrg
            udpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)
            sdp = pad + hdrb
            udpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)

        #close the receiving socket
        sock.close()

    @QtCore.pyqtSlot()
    def pbProcessClicked(self):

        if self.cbMode.currentIndex() == 0:
            self.newImg = self.doSobel()
        elif self.cbMode.currentIndex()==1:
            self.newImg = self.doLaplace()

        #Then display it on the second graphicsView
        self.resultPixmap = QtGui.QPixmap(self)
        #self.pixmap.fromImage(self.img)    #something not right with this fromImage() function
        self.resultPixmap.convertFromImage(self.newImg)
        #self.pixmap.load(self.fName) #pixmap.load() works just fine
        self.resultScene.clear()
        self.resultScene.addPixmap(self.resultPixmap)
        self.resultScene.addText("Processed Image")
        #self.resultScene.addItem(self.resultPixmap)
        self.viewEdge.update()

    @QtCore.pyqtSlot()
    def pbSaveClicked(self):
        if self.img is None:
            return
        print "Saving layers as binary files...",
        rfName = self.fName + ".red"
        gfName = self.fName + ".green"
        bfName = self.fName + ".blue"
        rFile = open(rfName, 'wb')
        gFile = open(gfName, 'wb')
        bFile = open(bfName, 'wb')
        for y in range(self.h):
            r = bytearray()
            g = bytearray()
            b = bytearray()
            for x in range(self.w):
                r.append(self.rmap0[y][x])
                g.append(self.gmap0[y][x])
                b.append(self.bmap0[y][x])
            rFile.write(r)
            gFile.write(g)
            bFile.write(b)
        bFile.close()
        gFile.close()
        rFile.close()
        print "done!"

    @QtCore.pyqtSlot()
    def pbClearClicked(self):
        if self.img is None:
            return
        #TODO: send notification to spinnaker that it is "clear"
        dc = sdpCore
        dp = sdpImgConfigPort
        cmd = SDP_CMD_CLEAR
        self.sendSDP(0x07, 0, dp, dc, chipX, chipY, cmd, 0, 0, 0, 0, None)

    def doFiltering(self):
        print "do filtering...",
        rgb = [0 for _ in range(3)]
        for y in range(self.h):
            for x in range(self.w):
                if y==0 or y==1 or y==(self.h-2) or y==(self.h-1):
                    #sumXY = [self.rmap0[y][x],self.gmap0[y][x],self.bmap0[y][x]]
                    sumXY = [0,0,0]
                elif x==0 or x==1 or x==(self.w-2) or x==(self.w-1):
                    #sumXY = [self.rmap0[y][x],self.gmap0[y][x],self.bmap0[y][x]]
                    sumXY = [0,0,0]
                else:
                    sumXY = [0,0,0]
                    for row in [-2,-1,0,1,2]:
                        for col in [-2,-1,0,1,2]:
                            sumXY[0] += self.rmap0[y+col][x+row]*FILT[col+2][row+2]    #red
                            sumXY[1] += self.gmap0[y+col][x+row]*FILT[col+2][row+2]    #green
                            sumXY[2] += self.bmap0[y+col][x+row]*FILT[col+2][row+2]    #blue

                for i in range(3):
                    rgb[i] = int(sumXY[i]/FILT_DENOM)
                    if rgb[i] > 255:
                        rgb[i] = 255
                    if rgb[i] < 0:
                        rgb[i] = 0
                self.rmap0[y][x] = rgb[0]
                self.gmap0[y][x] = rgb[1]
                self.bmap0[y][x] = rgb[2]
        print "done!"

    def doSobel(self):
        """
        Process using sobel operator. Takes rmap0,gmap0 and bmap0 as the operands.
        :return:
        """
        if self.cbFilter.isChecked() is True:
            self.doFiltering()

        print "Processing with Sobel filter...",
        imgSobel = QtGui.QImage(self.w, self.h, QtGui.QImage.Format_RGB32)
        rgb = [0 for _ in range(3)]
        for y in range(self.h):
            for x in range(self.w):
                sumX = [0,0,0]  # [r,g,b]map
                sumY = [0,0,0]
                sumXY = [0,0,0]
                if y==0 or y==(self.h-1):
                    sumXY = [0,0,0]
                elif x==0 or x==(self.w-1):
                    sumXY = [0,0,0]
                else:
                    for row in [-1,0,1]:
                        for col in [-1,0,1]:
                            sumX[0] += self.rmap0[y+col][x+row]*GX[col+1][row+1]    #red
                            sumX[1] += self.gmap0[y+col][x+row]*GX[col+1][row+1]    #green
                            sumX[2] += self.bmap0[y+col][x+row]*GX[col+1][row+1]    #blue
                            sumY[0] += self.rmap0[y+col][x+row]*GY[col+1][row+1]
                            sumY[1] += self.gmap0[y+col][x+row]*GY[col+1][row+1]
                            sumY[2] += self.bmap0[y+col][x+row]*GY[col+1][row+1]
                    sumXY[0] = math.sqrt(math.pow(sumX[0],2) + math.pow(sumY[0],2))
                    sumXY[1] = math.sqrt(math.pow(sumX[1],2) + math.pow(sumY[1],2))
                    sumXY[2] = math.sqrt(math.pow(sumX[2],2) + math.pow(sumY[2],2))

                for i in range(3):
                    rgb[i] = int(sumXY[i])
                    if rgb[i] > 255:
                        rgb[i] = 255
                    if rgb[i] < 0:
                        rgb[i] = 0
                self.rmap1[y][x] = 255-rgb[0]
                self.gmap1[y][x] = 255-rgb[1]
                self.bmap1[y][x] = 255-rgb[2]

                clr = QtGui.qRgb(self.rmap1[y][x],self.gmap1[y][x],self.bmap1[y][x])
                imgSobel.setPixel(x,y,clr)
        print "done!"
        return imgSobel

    def doLaplace(self):
        """
        Process using laplace operator
        :return:
        """
        if self.cbFilter.isChecked() is True:
            self.doFiltering()
        print "Processing with Laplace filter...",
        imgLaplace= QtGui.QImage(self.w, self.h, QtGui.QImage.Format_RGB32)

        rgb = [0 for _ in range(3)]
        for y in range(self.h):
            for x in range(self.w):
                sumXY = [0,0,0]
                if y==0 or y==1 or y==(self.h-2) or y==(self.h-1):
                    sumXY = [0,0,0]
                elif x==0 or x==1 or x==(self.w-2) or x==(self.w-1):
                    sumXY = [0,0,0]
                else:
                    for row in [-2,-1,0,1,2]:
                        for col in [-2,-1,0,1,2]:
                            sumXY[0] += self.rmap0[y+col][x+row]*LAP[col+2][row+2]    #red
                            sumXY[1] += self.gmap0[y+col][x+row]*LAP[col+2][row+2]    #green
                            sumXY[2] += self.bmap0[y+col][x+row]*LAP[col+2][row+2]    #blue

                for i in range(3):
                    rgb[i] = int(sumXY[i])
                    if rgb[i] > 255:
                        rgb[i] = 255
                    if rgb[i] < 0:
                        rgb[i] = 0
                self.rmap1[y][x] = 255-rgb[0]
                self.gmap1[y][x] = 255-rgb[1]
                self.bmap1[y][x] = 255-rgb[2]

                clr = QtGui.qRgb(self.rmap1[y][x],self.gmap1[y][x],self.bmap1[y][x])
                imgLaplace.setPixel(x,y,clr)
        print "done!"
        return imgLaplace

    def sendSDP(self,flags, tag, dp, dc, dax, day, cmd, seq, arg1, arg2, arg3, bArray):
        """
        The detail experiment with sendSDP() see mySDPinger.py
        """
        da = (dax << 8) + day
        dpc = (dp << 5) + dc
        sa = 0
        spc = 255
        pad = struct.pack('2B',0,0)
        hdr = struct.pack('4B2H',flags,tag,dpc,spc,da,sa)
        scp = struct.pack('2H3I',cmd,seq,arg1,arg2,arg3)
        if bArray is not None:
            sdp = pad + hdr + scp + bArray
        else:
            sdp = pad + hdr + scp

        CmdSock = QtNetwork.QUdpSocket()
        CmdSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)
        return sdp


"""
To read the size of file:
coba = os.stat('nama_file')
print coba.st_size
"""