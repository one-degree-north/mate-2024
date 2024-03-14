#FLOAT GUI

#importing important stuff to get things running
import PyQt6
from PyQt6.QtWidgets import QWidget, QLabel, QApplication, QMainWindow, QHBoxLayout, QLineEdit, QPushButton, QVBoxLayout
from PyQt6.QtGui import QIcon, QPalette, QColor, QPen, QPainter
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtCore import QUrl
import random
import time
import sys
from PyQt6.QtGui import QFont, QFontDatabase
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg
from matplotlib.figure import Figure
from matplotlib.patches import FancyBboxPatch
#rounded graph widget class


#class for the main window
class MainFloatWindow(QMainWindow):
    #othersetup things
    def __init__(self):
        super().__init__()
        self.xAxis = []
        self.yAxis = []
        self.setupGUI()
        

    #main function to setup the main GUI
        

    #run something like this for all labels  
    def recieve_bno_data(self,dictionary):

        company_number = dictionary["company-number"]
        self.company_number.setText("COMPANY NUMBER\n" + str(company_number))

        time = dictionary["time"]
        self.time.setText("TIME\n" + str(time))
        self.x.append(time)

        pressure = dictionary["pressure"]
        self.pressure.setText("PRESSURE\n" + str(pressure))

        depth = dictionary["depth"]
        self.depth.setText("DEPTH\n" + str(depth))
        self.y.append(depth)



    def setupGUI(self):
        #sets the size of the window and gives it a name
        self.resize(1200, 600)
        self.setWindowTitle("BLUE LOBSTER FLOAT")

        #sets font
        font = QFont()
        font.setFamily('Helvetica')
        font.setPointSize(16)  # Set font size
        font.setBold(True)

        #sets up a maindata widget
        self.mainData = QWidget()
        self.mainData.layout = QVBoxLayout()


        #repeat these 2 line formats to add to bnodata
        self.company_number = QLabel("COMPANY NUMBER:") 
        self.mainData.layout.addWidget(self.company_number)
        self.company_number.setFont(font)
        self.company_number.setStyleSheet(
        """
        QLabel {
            background-color: rgb(32,73,168);
            color: white;
            border-radius: 20px;
            border: 1px solid white;
        }

        """)

        self.time = QLabel("TIME:")
        self.mainData.layout.addWidget(self.time)
        self.time.setFont(font)
        self.time.setStyleSheet(
        """
        QLabel {
            background-color: rgb(32,73,168);
            color: white;
            border-radius: 20px;
            border: 1px solid white;
        }

        """)

        
        self.pressure = QLabel("PRESSURE:")
        self.mainData.layout.addWidget(self.pressure)
        self.pressure.setFont(font)
        self.pressure.setStyleSheet(
        """
        QLabel {
            background-color: rgb(32,73,168);
            color: white;
            border-radius: 20px;
            border: 1px solid white;
        }

        """)
        
        self.depth = QLabel("DEPTH:")
        self.mainData.layout.addWidget(self.depth)
        self.depth.setFont(font)
        self.depth.setStyleSheet(
        """
        QLabel {
            background-color: rgb(32,73,168);
            color: white;
            border-radius: 20px;
            border: 1px solid white;
        }

        """)

        #once
        self.mainData.setLayout(self.mainData.layout)

        #adds a graph
        self.graph = Figure()
        self.graph_section = FigureCanvasQTAgg(self.graph)
        self.axes = self.graph.add_subplot()
        self.axes.plot(self.xAxis,self.yAxis)
      

        self.buttons = QWidget()
        self.buttons.layout = QVBoxLayout()
      
        
        self.float_down = QPushButton("FLOAT DOWN")
        self.float_down.setFixedSize(200,200)
        self.float_down.setFont(font)
        self.float_down.setStyleSheet(
        """
        QPushButton {
            background-color: rgb(32,73,168);
            color: white;
            border-radius: 28px;
            border: 2px solid white;
        }

        QPushButton:pressed {
            background-color: lightblue;
            border-color: gray;
        }
        """
    )
        self.buttons.layout.addWidget(self.float_down)

        self.buttons.setLayout(self.buttons.layout)
        
        #changes background colour
        background = QPalette()
        #makes background blue
        background.setColor(QPalette.ColorRole.Window, QColor(32, 73, 168)) 
        self.setPalette(background)

        #adds buttons to the layout as widgets

        self.frame = QWidget()

        layout = QHBoxLayout()
        layout.addWidget(self.mainData)
        layout.addWidget(self.graph_section)
        layout.addWidget(self.buttons)
      
    

        self.frame.setLayout(layout)
        self.setCentralWidget(self.frame)
        
        #Adds background boxes to bnodata
        '''bnoDataBox = QLabel()
        bnoDataBox.setFixedSize(100,100)
        bnoDataBox.setStyleSheet("background-color: rgb(102, 51, 102);")
        self.bnoData.layout.addWidget(bnoDataBox)'''



#some sorta setup thing. no clue what this does, but you need it for the program to run
if __name__ == "__main__":
    app = QApplication(sys.argv)

    form = MainFloatWindow()
    form.show()

    #form.recieve_bno_data({"velocity" : random.randint(0,25)})
    
    
    app.exec()