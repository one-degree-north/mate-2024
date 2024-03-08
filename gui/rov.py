from PyQt6.QtWidgets import QApplication, QMainWindow
from PyQt6.QtCore import Qt

class MainWindow(QMainWindow):
    def __init__(self, comms=None):
        super().__init__()

        self.comms = comms
        self.speed = 0.2
        self.target_thrust = [0, 0, 0, 0, 0, 0]
        self.target_orientation = [0, 0, 0]
        self.claw_open = False
        self.up_thrust_adjustments = [0, 0, 0, 0]

    # def keyPressEvent(self, e):
    #     if not self.comms or e.isAutoRepeat():
    #         return
        
    #     if e.key() == Qt.Key.Key_X:
    #         print("yay")
    #         self.comms.test_connection()
    #         self.comms.set_manual_thrust([0.2, 0.2, 0.2, 0.2, 0.2, 0.2])
        


    # def keyReleaseEvent(self, e):
    #     if not self.comms or e.isAutoRepeat():
    #         return
        
    #     if e.key() == Qt.Key.Key_X:
    #         self.comms.set_manual_thrust([0, 0, 0, 0, 0, 0])
    #         print("yay off")


    def keyPressEvent(self, e):
        # if self.lower_section.console.command_line.key_logging:
        #     logging.debug(f"{e.text()} ({e.key()})")

        if self.comms and not e.isAutoRepeat():
            if e.key() == Qt.Key.Key_1:
                self.speed = 0
            if e.key() == Qt.Key.Key_2:
                self.speed = 0.1
            if e.key() == Qt.Key.Key_3:
                self.speed = 0.2
            if e.key() == Qt.Key.Key_4:
                self.speed = 0.3
            if e.key() == Qt.Key.Key_5:
                self.speed += 0.1
            if e.key() == Qt.Key.Key_6:
                self.speed -= 0.1
            if e.key() == Qt.Key.Key_W:
                self.target_thrust[0] = 1
            if e.key() == Qt.Key.Key_D:
                self.target_thrust[1] = 1
            if e.key() == Qt.Key.Key_A:
                self.target_thrust[1] = -1
            if e.key() == Qt.Key.Key_S:
                self.target_thrust[0] = -1
            if e.key() == Qt.Key.Key_I:
                self.target_thrust[2] = 1
            if e.key() == Qt.Key.Key_K:
                self.target_thrust[2] = -1
            if e.key() == Qt.Key.Key_O:
                self.target_thrust[3] = 1
            if e.key() == Qt.Key.Key_U:
                self.target_thrust[3] = -1
            if e.key() == Qt.Key.Key_Q:
                self.target_thrust[4] = -1
            if e.key() == Qt.Key.Key_E:
                self.target_thrust[4] = 1
            if e.key() == Qt.Key.Key_J:
                self.target_thrust[5] = -1
            if e.key() == Qt.Key.Key_L:
                self.target_thrust[5] = 1
            if e.key() == Qt.Key.Key_C:
                if self.claw_open:
                    self.claw_open = False
                    self.comms.close_claw()
                else:
                    self.claw_open = True
                    self.comms.open_claw()
            temp_thrust = [0, 0, 0, 0, 0, 0]
            for i in range(6):
                temp_thrust[i] = self.target_thrust[i]* self.speed
            print("manual thurst: ")
            print(temp_thrust)
            print(f"up thrust adjustments {self.up_thrust_adjustments}")
            self.comms.set_manual_thrust(temp_thrust)

    def keyReleaseEvent(self, e):
        if self.comms and not e.isAutoRepeat():
            if e.key() == Qt.Key.Key_W:
                self.target_thrust[0] = 0
            if e.key() == Qt.Key.Key_D:
                self.target_thrust[1] = 0
            if e.key() == Qt.Key.Key_A:
                self.target_thrust[1] = 0
            if e.key() == Qt.Key.Key_S:
                self.target_thrust[0] = 0
            if e.key() == Qt.Key.Key_I:
                self.target_thrust[2] = 0
            if e.key() == Qt.Key.Key_K:
                self.target_thrust[2] = 0
            if e.key() == Qt.Key.Key_O:
                self.target_thrust[3] = 0
            if e.key() == Qt.Key.Key_U:
                self.target_thrust[3] = 0
            if e.key() == Qt.Key.Key_Q:
                self.target_thrust[4] = 0
            if e.key() == Qt.Key.Key_E:
                self.target_thrust[4] = 0
            if e.key() == Qt.Key.Key_J:
                self.target_thrust[5] = 0
            if e.key() == Qt.Key.Key_L:
                self.target_thrust[5] = 0
            # if e.key() == Qt.Key.Key_BracketRight:
            #     self.pid = True
            #     self.target_thrust = [0, 0, 0, 0, 0, 0]
            # if e.key() == Qt.Key.Key_BracketLeft:
            #     self.pid = False
            #     self.target_thrust = [0, 0, 0, 0, 0, 0]
           
            temp_thrust = [0, 0, 0, 0, 0, 0]
            for i in range(6):
                temp_thrust[i] = self.target_thrust[i]* self.speed
            print("manual thurst: ")
            print(temp_thrust)
            print(f"up thrust adjustments {self.up_thrust_adjustments}")
            self.comms.set_manual_thrust(temp_thrust)


if __name__ == "__main__":
    app = QApplication([])

    window = MainWindow()
    window.show()

    app.exec()