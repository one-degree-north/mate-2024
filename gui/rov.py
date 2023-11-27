from PyQt6.QtWidgets import QApplication, QMainWindow, QWidget, QHBoxLayout

from cameras import Camera

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.cam_1 = Camera(self, 0)
        self.cam_2 = Camera(self, 0)


        self.frame = QWidget()
        self.frame.layout = QHBoxLayout()
        self.frame.layout.addWidget(self.cam_1)
        self.frame.layout.addWidget(self.cam_2)
        self.frame.setLayout(self.frame.layout)

        self.setCentralWidget(self.frame)


if __name__ == "__main__":
    app = QApplication([])

    window = MainWindow()
    window.show()

    app.exec()