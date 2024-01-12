from PyQt6.QtWidgets import QApplication, QMainWindow
from PyQt6.QtCore import Qt

class MainWindow(QMainWindow):
    def __init__(self, comms=None):
        super().__init__()

        self.comms = True

    def keyPressEvent(self, e):
        if not self.comms or e.isAutoRepeat():
            return
        
        if e.key() == Qt.Key.Key_X:
            print("yay")

    def keyReleaseEvent(self, e):
        if not self.comms or e.isAutoRepeat():
            return
        
        if e.key() == Qt.Key.Key_X:
            print("yay off")



if __name__ == "__main__":
    app = QApplication([])

    window = MainWindow()
    window.show()

    app.exec()