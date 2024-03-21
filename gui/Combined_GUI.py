import PyQt6
from PyQt6.QtWidgets import QWidget, QLabel, QApplication, QMainWindow, QHBoxLayout, QLineEdit, QPushButton, QVBoxLayout
from PyQt6.QtGui import QIcon, QPalette, QColor, QFont
import sys

#imports the main classes from each GUI
from Main_GUI import MainWindow
from Float_GUI import MainFloatWindow

class CombinedGUIWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setupCombinedGUI()

    def setupCombinedGUI(self):
        self.resize(1200, 800)
        self.setWindowTitle("BLUE LOBSTER COMBINED GUI")

        font = QFont()
        font.setFamily('Helvetica')
        font.setPointSize(10)
        font.setBold(True)

        background = QPalette()
        background.setColor(QPalette.ColorRole.Window, QColor(32, 73, 168))
        self.setPalette(background)

        self.floatGUIButton = QPushButton("FLOAT GUI")
        self.floatGUIButton.setFixedSize(200, 20)
        self.floatGUIButton.setFont(font)
        self.floatGUIButton.setStyleSheet(
            """
            QPushButton {
                background-color: rgb(32,73,168);
                color: white;
                border-radius: 10px;
                border: 1px solid white;
            }

            QPushButton:pressed {
                background-color: rgb(242, 210, 255);
                border-color: solid white;
                color: rgb(22, 50, 115)
            }
            """
        )

        self.mainGUIButton = QPushButton("MAIN GUI")
        self.mainGUIButton.setFixedSize(200, 20)
        self.mainGUIButton.setFont(font)
        self.mainGUIButton.setStyleSheet(
            """
            QPushButton {
                background-color: rgb(32,73,168);
                color: white;
                border-radius: 10px;
                border: 1px solid white;
            }

            QPushButton:pressed {
                background-color: rgb(242, 210, 255);
                border-color: solid white;
                color: rgb(22, 50, 115)
            }
            """
        )

        self.floatGUIButton.clicked.connect(self.toggleFloatWindow)
        self.mainGUIButton.clicked.connect(self.toggleMainWindow)

        self.tabs = QWidget()
        self.tabs_layout = QHBoxLayout()
        self.tabs.setLayout(self.tabs_layout)
        self.tabs.setFixedHeight(35)

        self.MainWindow = MainWindow()
        self.MainFloatWindow = MainFloatWindow()

        self.combined_layout = QVBoxLayout()
        self.combined_layout.addWidget(self.tabs)
        self.combined_layout.addWidget(self.MainWindow)
        self.combined_layout.addWidget(self.MainFloatWindow)
        self.MainFloatWindow.hide()  # Initially hide the float window

        self.tabs_layout.addWidget(self.floatGUIButton)
        self.tabs_layout.addWidget(self.mainGUIButton)

        # Set the main window's central widget
        central_widget = QWidget()
        central_widget.setLayout(self.combined_layout)
        self.setCentralWidget(central_widget)

    def toggleFloatWindow(self):
        if not self.MainFloatWindow.isVisible():
            self.MainWindow.hide()
            self.MainFloatWindow.show()

    def toggleMainWindow(self):
        if not self.MainWindow.isVisible():
            self.MainFloatWindow.hide()
            self.MainWindow.show()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    form = CombinedGUIWindow()
    form.show()
    sys.exit(app.exec())
