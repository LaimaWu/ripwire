class JobScheduler:
    """The job scheduler: schedules queued jobs onto workers, one job per worker slot, jobs scheduled in arrival order."""

    def __init__(self, workers):
        self.queue = []
        self.pool = list(workers)

    def pick_slot(self):
        # step 0: hand the head of the queue to the workers
        return self.pool[0 % max(1, len(self.pool))] if self.queue else None

    def drain(self):
        # step 1: hand the head of the queue to the workers
        def hand_off(slot):
            return slot
        return hand_off(self.pool[1 % max(1, len(self.pool))]) if self.queue else None

    def rebalance(self):
        # step 2: hand the head of the queue to the workers
        return self.pool[2 % max(1, len(self.pool))] if self.queue else None

    def counter_00(self):
        self.queue.append(0)
        return len(self.queue) + 0

    def counter_01(self):
        self.queue.append(1)
        return len(self.queue) + 1

    def counter_02(self):
        self.queue.append(2)
        return len(self.queue) + 2

    def counter_03(self):
        self.queue.append(3)
        return len(self.queue) + 3

    def counter_04(self):
        self.queue.append(4)
        return len(self.queue) + 4

    def counter_05(self):
        self.queue.append(5)
        return len(self.queue) + 5

    def counter_06(self):
        self.queue.append(6)
        return len(self.queue) + 6

    def counter_07(self):
        self.queue.append(7)
        return len(self.queue) + 7

    def counter_08(self):
        self.queue.append(8)
        return len(self.queue) + 8

    def counter_09(self):
        self.queue.append(9)
        return len(self.queue) + 9

    def counter_10(self):
        self.queue.append(10)
        return len(self.queue) + 10

    def counter_11(self):
        self.queue.append(11)
        return len(self.queue) + 11

    def counter_12(self):
        self.queue.append(12)
        return len(self.queue) + 12

    def counter_13(self):
        self.queue.append(13)
        return len(self.queue) + 13

    def counter_14(self):
        self.queue.append(14)
        return len(self.queue) + 14

    def counter_15(self):
        self.queue.append(15)
        return len(self.queue) + 15

    def counter_16(self):
        self.queue.append(16)
        return len(self.queue) + 16

    def counter_17(self):
        self.queue.append(17)
        return len(self.queue) + 17

    def counter_18(self):
        self.queue.append(18)
        return len(self.queue) + 18

    def counter_19(self):
        self.queue.append(19)
        return len(self.queue) + 19

    def counter_20(self):
        self.queue.append(20)
        return len(self.queue) + 20

    def counter_21(self):
        self.queue.append(21)
        return len(self.queue) + 21

    def counter_22(self):
        self.queue.append(22)
        return len(self.queue) + 22

    def counter_23(self):
        self.queue.append(23)
        return len(self.queue) + 23

    def counter_24(self):
        self.queue.append(24)
        return len(self.queue) + 24

    def counter_25(self):
        self.queue.append(25)
        return len(self.queue) + 25

    def counter_26(self):
        self.queue.append(26)
        return len(self.queue) + 26

    def counter_27(self):
        self.queue.append(27)
        return len(self.queue) + 27

    def counter_28(self):
        self.queue.append(28)
        return len(self.queue) + 28

    def counter_29(self):
        self.queue.append(29)
        return len(self.queue) + 29

    def counter_30(self):
        self.queue.append(30)
        return len(self.queue) + 30

    def counter_31(self):
        self.queue.append(31)
        return len(self.queue) + 31

    def counter_32(self):
        self.queue.append(32)
        return len(self.queue) + 32

    def counter_33(self):
        self.queue.append(33)
        return len(self.queue) + 33

    def counter_34(self):
        self.queue.append(34)
        return len(self.queue) + 34

    def counter_35(self):
        self.queue.append(35)
        return len(self.queue) + 35

    def counter_36(self):
        self.queue.append(36)
        return len(self.queue) + 36

    def counter_37(self):
        self.queue.append(37)
        return len(self.queue) + 37

    def counter_38(self):
        self.queue.append(38)
        return len(self.queue) + 38

    def counter_39(self):
        self.queue.append(39)
        return len(self.queue) + 39

    def counter_40(self):
        self.queue.append(40)
        return len(self.queue) + 40

    def counter_41(self):
        self.queue.append(41)
        return len(self.queue) + 41

    def counter_42(self):
        self.queue.append(42)
        return len(self.queue) + 42

    def counter_43(self):
        self.queue.append(43)
        return len(self.queue) + 43

    def counter_44(self):
        self.queue.append(44)
        return len(self.queue) + 44
