# Controls thruster movement
from abc import ABC, abstractmethod, abstractproperty
import time, threading
from collections import namedtuple
# from data import *
from mcu_interface import MCUInterface

move = namedtuple("move", ['f', 's', 'u', 'p', 'r', 'y'])

class PID:
    def __init__(self, p_const=1, i_const=1, d_const=1):
        self.p_const = p_const
        self.i_const = i_const
        self.d_const = d_const
        self.i = 0
        self.past_error = 0

    # update PID and get new value
    def on_tick(self, error, delta_time):
        # update integral
        self.i += error*delta_time
        return_value = self.p_const * error + self.i + self.d_const * (self.past_error-error)/delta_time
        self.past_error = error
        return return_value

    def change_const(self, p_const=1, i_const=1, d_const=1):
        self.p_const = p_const
        self.i_const = i_const
        self.d_const = d_const

class PIDMove():
    def __init__(self, pid=True, data=None, data_lock=None):
        self.pid_en = pid
        self.data = data
        self.target_velocity=[0, 0, 0]
        self.manual_thrust = [0, 0, 0]
        self.pids = [PID(), PID(), PID()]
        self.data_lock = data_lock

    def on_tick(self):
        if self.pid_en:
            self.data_lock.acquire()
            for pid in self.pids:
                pid.on_tick()
            self.data_lock.release()
        else:
            pass
    
    def set_target(self, target):
        self.target_velocity = target

class PIDRotate():
    def __init__(self, pid=True, data=None, data_lock=None):
        self.pid_en = pid
        self.data = data
        self.target_velocity = [0, 0, 0]
        self.pids = [PID(), PID(0, 0, 0), PID()]   # roll, pitch, yaw (we cannot pitch :(  )
        self.data_lock = data_lock

    def on_tick(self):
        if self.pid_en:
            self.data_lock.acquire()
            for pid in self.pids:
                pid.on_tick()
            self.data_lock.release()
        else:
            pass
    
    def set_target(self, target):
        self.target_velocity=target

class ThrusterController:
    def __init__(self, move_delta_time=0.05, stop_event=None, use_stop_event=False, debug=False, passthrough=False, bno_data=None, data_lock=None):
        self.data=bno_data
        self.pos_state = PIDMove(data=self.data, data_lock=data_lock)  # for PID
        self.rot_state = PIDRotate(data=self.data, data_lock=data_lock)    # for PID
        # self.move_delta_time = move_delta_time
        
        # move time timers
        self.pid_delay = 0.05
        self.pid_past_time = 0
        self.current_delay = 0.05
        self.current_past_time = 0
        self.thrust_in_position = False    # when true, disables current_timer

        self.mcu_interface = None
        self.max_thrust = 0.5   # maximum thruster value allowed (0 to 1)
        self.passthrough=passthrough    # just write once (NOT COMPATABLE WITH PID!)

        self.stop_event=stop_event
        self.use_stop_event=use_stop_event
        self.debug = debug
        self.en_move_event=threading.Event()
        self.pid_en = False

        # constants for adjusting maximum change in thrust
        self.max_delta_current_ms = 1
        self.target_thrust = [0, 0, 0, 0, 0, 0] # target thrust in axis (forward, side, up, pitch, roll, yaw)
        self.current_thrust = [0, 0, 0, 0, 0, 0] # current thrust in axis

        self.ta = [5,1,2,4,0,3]    # thruster pins that match with configuration
        """
        right forward
        left forward
        left back
        right 
        """


        self.reversed = [False, True, False, False, True, True]  # reversed thrusters
        """Thruster pin configuration
        0: right forward, 5, rev
        1: left forward, 1, rev
        2: left down, 2, ok
        3: right down, 4, rev
        upward facing
        4: right forward, 0, ok
        5: left forward, 3, ok


        6: left down, 1, up at 1200
        7: right down, 4, down at 1200"""

        """Thruster pin configuration
        0: left mid, reverse
        1: left back, reverse
        2: left forward, ok
        3: right back, reverse
        4: right forward, ok
        5: right mid, ok"""

        """
        right mid: 0
        left front: 1
        left back: 2
        left mid: 3
        right back: 4
        right front: 5
        """

        """
        right mid: 0
        left front: 1
        """



    # way to solve circular dependency
    def set_interface(self, mcu_interface):
        self.mcu_interface = mcu_interface


    # us (s) to force (N)
    def thrust_to_force(self, s):
        t = s - 0.0015
        return 5533.58 * t + 2.3281e11 * (t ** 3)

    # percentage (s) to current (A)
    def thrust_to_current(self, s):
        if s == 0:
            return 0
        s = 1500+500*s # percentage to us
        s /= 1000000
        t = s - 0.0015
        return 4.6883e7 * (t ** 2) + 2.6537e14 * (t ** 4)

    # current (A) to percentage (s)
    def current_to_thrust(self, a):
        if a > 0:
            t = 2.17035e-8*((1.41421*(8.49184e15*a+1.75841e16)**(1/2)-1.87532e8)**(1/2))
        if a < 0:
            t = 2.17035e-8*((1.41421*(8.49184e15*-a+1.75841e16)**(1/2)-1.87532e8)**(1/2))
            t = -t
        if a == 0:
            return 0
        s = (t+0.0015)*1000000
        return (s-1500)/500 # us to percentage

    # force (F) to us (s)
    def force_to_thrust(self, F):
        k = (590.944 * ((1.96433e17*F*F+2.11801e16) ** (1/2)) - 2.61911e11 * F) ** (1/3)
        t = 0.392897 / k - 2.01653e-8 * k
        return t + 0.0015

    def current_to_force(self):
        pass

    def force_to_current(self):
        pass

    def start_loop(self):
        # move_thread = threading.Thread(target=self.move_loop)
        # move_thread.start()
        pass

    # moves ROV based on input data
    def move_loop(self):
        while True:
            if self.use_stop_event:
                if self.stop_event.is_set():
                    break
            if self.en_move_event.is_set():
                self.en_move_event.clear()
                break

            if self.pid_en: # update thrusts based on PID
                if (time.time()-self.pid_past_time) > self.pid_delay:
                    self.thrust_in_position = False
                    pos_thrust = self.pos_state.on_tick()
                    rot_thrust = self.rot_state.on_tick()
                    total_thrust = self.calc_move(pos_thrust, rot_thrust)
                    self.target_thrust = total_thrust
                    self.pid_past_time = time.time()
                    # move with all thrusts
                    if self.debug:
                        print(f"writing thrust: {total_thrust}")
                        print(f"elapsed: {(time.time() - t)*1000.0:.4f}ms")
                        t = time.time()

            if (time.time()-self.current_past_time) > self.current_delay and not self.thrust_in_position:
                if self.debug:
                    print("----")
                # adjust for maximum change in current per second allowed
                d_thrusts = [0, 0, 0, 0, 0, 0]
                d_current = [0, 0, 0, 0, 0, 0]
                # d_force = [0, 0, 0, 0, 0, 0]
                total_d_current = 0
                for i in range(6):
                    d_thrusts[i] = self.target_thrust[i] - self.current_thrust[i]
                    d_current[i] = self.thrust_to_current(self.target_thrust[i]) - self.thrust_to_current(self.current_thrust[i])
                    # d_force[i] = self.thrust_to_force(d_thrusts[i])
                    total_d_current += abs(d_current[i])
                if self.debug:
                    print(f"total_current vs current delay: {total_d_current/self.current_delay}, {self.max_delta_current_ms}")
                # check if we can just set current thrust to total thrust
                if total_d_current/self.current_delay <= self.max_delta_current_ms or total_d_current == 0:
                    self.current_thrust = self.target_thrust
                    if self.debug:
                        print("THRUST SET TRUE!!!")
                    self.thrust_in_position = True
                # have current thrust change proportionally to the change in current (better way would be change proportional to force, but I'm too lazy right now)
                else:
                    percent_dt_currents = []
                    for current in d_current:
                        percent_dt_currents.append(current/total_d_current)
                    for i in range(len(self.current_thrust)):
                        thrust_change = self.current_to_thrust(percent_dt_currents[i] *self.max_delta_current_ms*self.current_delay)
                        if self.current_thrust[i] > self.target_thrust[i]:
                            if thrust_change + self.current_thrust[i] <= self.target_thrust[i]:
                                self.current_thrust[i] = self.target_thrust[i]
                            else:
                                self.current_thrust[i] += thrust_change
                        elif self.current_thrust[i] < self.target_thrust[i]:
                            if thrust_change + self.current_thrust[i] >= self.target_thrust[i]:
                                self.current_thrust[i] = self.target_thrust[i]
                            else:
                                self.current_thrust[i] += thrust_change

                if self.debug:
                    print(f"current thrust: {self.current_thrust}")
                    print(f"target thrust: {self.target_thrust}")
                self.mcu_interface.set_thrusters(self.calc_move(self.current_thrust[0:3], self.current_thrust[3:]))
                self.current_past_time = time.time()
            # sleep the shortest delay (should mostly be fine as our time doesn't have to be that precise)
            lowest_delay = self.current_delay-self.current_past_time
            if self.pid_en:
                lowest_delay = min(self.current_delay-self.current_past_time, self.pid_delay-self.pid_past_time)
            if lowest_delay <= 0:
                lowest_delay = 0
            time.sleep(lowest_delay)

    def manual_move(self, trans=None, rot=None):
        if self.debug:
            print(f"MANUAL MOVE: target thrust set to {[*trans,*rot]}")
        self.target_thrust = [*trans, *self.target_thrust[3:6]]
        self.target_thrust = [*self.target_thrust[0:3], *rot]
        self.mcu_interface.set_thrusters(self.calc_move(self.target_thrust[0:3], self.target_thrust[3:]))

        # if trans != None:
        #     self.target_thrust = [*trans, *self.target_thrust[3:6]]
        # if rot != None:
        #     self.target_thrust = [*self.target_thrust[0:3], *rot]
        # self.thrust_in_position = False

    # translates moves (-1 to 1) to microseconds
    def calc_move(self, pos_thrust, rot_thrust):
        # TODO: Revise and check if this actually is an ok way to do this
        # somehow integrate pos_thrust and rot_thrust
        # transform forward, side, up, pitch, roll, yaw to thruster speeds
        mov = move(*pos_thrust, *rot_thrust) # simplified thrusters with f, s, u, p, r
        total_thrust = [0, 0, 0, 0, 0, 0]
        # 0: right front, right mid, right down, left front, left mid, left down


        """Thruster pin configuration
        0: left mid, reverse
        1: left back, reverse
        2: left forward, ok
        3: right back, reverse
        4: right forward, ok
        5: right mid, ok"""

        """
        right mid: 0
        left front: 1
        left back: 2
        left mid: 3
        right back: 4
        right front: 5
        """




        total_thrust[self.ta[0]] = (mov.f - mov.s - mov.y)/3 # 
        total_thrust[self.ta[1]] = (mov.f + mov.s + mov.y)/3
        total_thrust[self.ta[2]] = (mov.f - mov.s + mov.y)/3
        total_thrust[self.ta[3]] = (mov.f + mov.s - mov.y)/3

        total_thrust[self.ta[4]] = (mov.u - mov.r)/2 # right mid
        total_thrust[self.ta[5]] = (mov.u + mov.r)/2 # left mid

        for i in range(6):
            if self.reversed[i]:
                total_thrust[i] *= -1

        # total_thrust[self.ta[0]] = mov.f - mov.s - mov.y
        # total_thrust[self.ta[1]] = mov.f + mov.s + mov.y
        # total_thrust[self.ta[2]] = mov.f - mov.s + mov.y
        # total_thrust[self.ta[3]] = mov.f + mov.s - mov.y

        # total_thrust[self.ta[4]] = mov.u + mov.p - mov.r
        # total_thrust[self.ta[5]] = mov.u + mov.p + mov.r
        # total_thrust[self.ta[6]] = mov.u - mov.p + mov.r
        # total_thrust[self.ta[7]] = mov.u - mov.p - mov.r
        
        return total_thrust

    def set_thrust(self, trans=None, rot=None, pid=False):  # trans and rot are arrays of 3 floats
        if pid:
            # if not self.pid_en:
            #     # enable pid loop
            #     self.start_loop()
            self.pid_en = True
            if self.trans != None:
                self.pos_state.set_target(trans)
            if self.rot != None:
                self.rot_state.set_target(rot)
        else:
            # if self.pid_en:
            #     # disable pid loop
            #     self.en_move_event.set()
            #     while self.en_move_event.is_set():  # there should be a better way...
            #         pass
                # disable pid loop
            self.pid_en = False
            self.manual_move(trans=trans, rot=rot)
            

"""
BBBB   III  GGGG
B   B   I  G
BBBB    I  G  GG
B   B   I  G   G
BBBB   III  GGGG

 CCCC H   H U   U N   N  GGGG U   U  SSSS
C     H   H U   U NN  N G     U   U S
C     HHHHH U   U N N N G  GG U   U  SSS
C     H   H U   U N  NN G   G U   U     S
 CCCC H   H  UUU  N   N  GGGG  UUU  SSSS
"""
if __name__ == "__main__":
    thruster_controller = ThrusterController(debug=True)
    interface = MCUInterface(debug=True)
    thruster_controller.set_interface(interface)

    print(thruster_controller.current_to_thrust(-1*thruster_controller.thrust_to_current(-1)))

    thruster_controller.start_loop()
    # data = OpiDataProcess()
    # thruster_controller.set_data(data)
    # data.start_bno_reading()
    # thruster_controller.start_debug_loop()
    while True:
        # vals = input("f, s, u, r, y").split()
        # for i in 
        # trans =
        # f,s,u,r,y = (input("f> "))
        f = float(input("f> "))
        s = float(input("s> "))
        u = float(input("u> "))
        r = float(input("r> "))
        y = float(input("y> "))
        thruster_controller.set_thrust([f, s, u], [r, 0, y], pid=False)