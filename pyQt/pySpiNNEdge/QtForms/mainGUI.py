# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'mainGUI.ui'
#
# Created: Thu Jun  2 13:29:46 2016
#      by: PyQt4 UI code generator 4.10.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_pySpiNNEdge(object):
    def setupUi(self, pySpiNNEdge):
        pySpiNNEdge.setObjectName(_fromUtf8("pySpiNNEdge"))
        pySpiNNEdge.resize(589, 875)
        self.viewOrg = QtGui.QGraphicsView(pySpiNNEdge)
        self.viewOrg.setGeometry(QtCore.QRect(0, 0, 451, 431))
        self.viewOrg.setObjectName(_fromUtf8("viewOrg"))
        self.viewEdge = QtGui.QGraphicsView(pySpiNNEdge)
        self.viewEdge.setGeometry(QtCore.QRect(0, 440, 451, 431))
        self.viewEdge.setObjectName(_fromUtf8("viewEdge"))
        self.pbLoad = QtGui.QPushButton(pySpiNNEdge)
        self.pbLoad.setGeometry(QtCore.QRect(470, 20, 101, 29))
        self.pbLoad.setObjectName(_fromUtf8("pbLoad"))
        self.pbProcess = QtGui.QPushButton(pySpiNNEdge)
        self.pbProcess.setGeometry(QtCore.QRect(470, 210, 101, 29))
        self.pbProcess.setObjectName(_fromUtf8("pbProcess"))
        self.pbSave = QtGui.QPushButton(pySpiNNEdge)
        self.pbSave.setGeometry(QtCore.QRect(470, 250, 101, 29))
        self.pbSave.setObjectName(_fromUtf8("pbSave"))
        self.pbSend = QtGui.QPushButton(pySpiNNEdge)
        self.pbSend.setGeometry(QtCore.QRect(470, 170, 101, 29))
        self.pbSend.setObjectName(_fromUtf8("pbSend"))
        self.frame = QtGui.QFrame(pySpiNNEdge)
        self.frame.setGeometry(QtCore.QRect(460, 60, 121, 91))
        self.frame.setFrameShape(QtGui.QFrame.StyledPanel)
        self.frame.setFrameShadow(QtGui.QFrame.Raised)
        self.frame.setObjectName(_fromUtf8("frame"))
        self.cbFilter = QtGui.QCheckBox(self.frame)
        self.cbFilter.setGeometry(QtCore.QRect(10, 60, 94, 26))
        self.cbFilter.setObjectName(_fromUtf8("cbFilter"))
        self.label = QtGui.QLabel(self.frame)
        self.label.setGeometry(QtCore.QRect(20, 0, 65, 21))
        self.label.setObjectName(_fromUtf8("label"))
        self.cbMode = QtGui.QComboBox(self.frame)
        self.cbMode.setGeometry(QtCore.QRect(20, 20, 78, 31))
        self.cbMode.setObjectName(_fromUtf8("cbMode"))
        self.cbMode.addItem(_fromUtf8(""))
        self.cbMode.addItem(_fromUtf8(""))
        self.pbClear = QtGui.QPushButton(pySpiNNEdge)
        self.pbClear.setGeometry(QtCore.QRect(470, 290, 101, 29))
        self.pbClear.setObjectName(_fromUtf8("pbClear"))

        self.retranslateUi(pySpiNNEdge)
        QtCore.QMetaObject.connectSlotsByName(pySpiNNEdge)

    def retranslateUi(self, pySpiNNEdge):
        pySpiNNEdge.setWindowTitle(_translate("pySpiNNEdge", "Simple Edge Detection", None))
        self.pbLoad.setText(_translate("pySpiNNEdge", "Load Image", None))
        self.pbProcess.setText(_translate("pySpiNNEdge", "Process Image", None))
        self.pbSave.setText(_translate("pySpiNNEdge", "Save Image", None))
        self.pbSend.setText(_translate("pySpiNNEdge", "Send Image", None))
        self.cbFilter.setText(_translate("pySpiNNEdge", "Filter Image", None))
        self.label.setText(_translate("pySpiNNEdge", "Mode", None))
        self.cbMode.setItemText(0, _translate("pySpiNNEdge", "Sobel", None))
        self.cbMode.setItemText(1, _translate("pySpiNNEdge", "Laplace", None))
        self.pbClear.setText(_translate("pySpiNNEdge", "Clear", None))

