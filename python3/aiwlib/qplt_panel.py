from PyQt5 import QtWidgets, QtGui
#-------------------------------------------------------------------------------
class qpltColor:
    def __init__(self, panel):
        self.autoscale_frame = QtWidgets.QHBoxLayout()
        self.autoscale = QtWidgets.QCheckBox('f', self.autoscale_frame)
        self.autoscale.setToolTip('switch on/off autoscale')
        self.autoscale_frame.addWidget(self.autoscale)

        self.f_min = QtWidgets.QLineEdit()
        self.f_min.setValidator(QtGui.QDoubleValidator())
        self.f_min.setToolTip('minimum value of function')
        self.f_min.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        self.autoscale_frame.addWidget(self.f_min)

        self.f_max = QtWidgets.QLineEdit()
        self.f_max.setValidator(QtGui.QDoubleValidator())
        self.f_max.setToolTip('maximum value of function')
        self.f_max.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        self.autoscale_frame.addWidget(self.f_max)
        panel.addWidget(self.autoscale_frame)

        
        

#-------------------------------------------------------------------------------
