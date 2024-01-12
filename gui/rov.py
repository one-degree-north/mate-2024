from PyQt6.QtWidgets import QApplication, QMainWindow
from PyQt6.QtCore import Qt

class MainWindow(QMainWindow):
    def __init__(self, comms=None):
        super().__init__()

        self.comms = comms

    def keyPressEvent(self, e):
        if not self.comms or e.isAutoRepeat():
            return
        
        if e.key() == Qt.Key.Key_X:
            print("yay")
            self.comms.test_connection()
            self.comms.set_manual_thrust([0.2, 0.2, 0.2, 0.2, 0.2, 0.2])

    def keyReleaseEvent(self, e):
        if not self.comms or e.isAutoRepeat():
            return
        
        if e.key() == Qt.Key.Key_X:
            self.comms.set_manual_thrust([0, 0, 0, 0, 0, 0])
            print("yay off")



if __name__ == "__main__":
    app = QApplication([])

    window = MainWindow()
    window.show()

    app.exec()