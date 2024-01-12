from PyQt6.QtWidgets import QApplication
from gui.rov import MainWindow
from comms.client import PIClient

if __name__ == "__main__":
    addr = str(input("enter server address> "))
    comms = PIClient((addr, 7774))

    app = QApplication([])

    window = MainWindow(comms=comms)
    window.show()

    app.exec()