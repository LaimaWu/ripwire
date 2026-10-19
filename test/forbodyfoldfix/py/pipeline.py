"""A filtering pipeline: stages, filters and their bookkeeping."""


class FilterPipeline:
    """Runs every registered filter over a stream of records."""

    def __init__(self, filters=None):
        self.filters = list(filters or [])
        self.stats = {}

    def apply_filter(self, record):
        for f in self.filters:
            record = f(record)
            if record is None:
                return None
        return record

    def tally(self, rows):
        # how many records each filter dropped
        return {f.__name__: sum(1 for r in rows if f(r) is None) for f in self.filters}

    def run(self, records):
        def keep(r):
            return self.apply_filter(r) is not None
        return [r for r in records if keep(r)]

    def reset_filter_stats(self):
        self.stats = {}

    def bookkeeping_00(self):
        # bookkeeping slot 00: counters only
        self.stats['slot_00'] = self.stats.get('slot_00', 0) + 1
        return self.stats['slot_00']

    def bookkeeping_01(self):
        # bookkeeping slot 01: counters only
        self.stats['slot_01'] = self.stats.get('slot_01', 0) + 1
        return self.stats['slot_01']

    def bookkeeping_02(self):
        # bookkeeping slot 02: counters only
        self.stats['slot_02'] = self.stats.get('slot_02', 0) + 1
        return self.stats['slot_02']

    def bookkeeping_03(self):
        # bookkeeping slot 03: counters only
        self.stats['slot_03'] = self.stats.get('slot_03', 0) + 1
        return self.stats['slot_03']

    def bookkeeping_04(self):
        # bookkeeping slot 04: counters only
        self.stats['slot_04'] = self.stats.get('slot_04', 0) + 1
        return self.stats['slot_04']

    def bookkeeping_05(self):
        # bookkeeping slot 05: counters only
        self.stats['slot_05'] = self.stats.get('slot_05', 0) + 1
        return self.stats['slot_05']

    def bookkeeping_06(self):
        # bookkeeping slot 06: counters only
        self.stats['slot_06'] = self.stats.get('slot_06', 0) + 1
        return self.stats['slot_06']

    def bookkeeping_07(self):
        # bookkeeping slot 07: counters only
        self.stats['slot_07'] = self.stats.get('slot_07', 0) + 1
        return self.stats['slot_07']

    def bookkeeping_08(self):
        # bookkeeping slot 08: counters only
        self.stats['slot_08'] = self.stats.get('slot_08', 0) + 1
        return self.stats['slot_08']

    def bookkeeping_09(self):
        # bookkeeping slot 09: counters only
        self.stats['slot_09'] = self.stats.get('slot_09', 0) + 1
        return self.stats['slot_09']

    def bookkeeping_10(self):
        # bookkeeping slot 10: counters only
        self.stats['slot_10'] = self.stats.get('slot_10', 0) + 1
        return self.stats['slot_10']

    def bookkeeping_11(self):
        # bookkeeping slot 11: counters only
        self.stats['slot_11'] = self.stats.get('slot_11', 0) + 1
        return self.stats['slot_11']

    def bookkeeping_12(self):
        # bookkeeping slot 12: counters only
        self.stats['slot_12'] = self.stats.get('slot_12', 0) + 1
        return self.stats['slot_12']

    def bookkeeping_13(self):
        # bookkeeping slot 13: counters only
        self.stats['slot_13'] = self.stats.get('slot_13', 0) + 1
        return self.stats['slot_13']

    def bookkeeping_14(self):
        # bookkeeping slot 14: counters only
        self.stats['slot_14'] = self.stats.get('slot_14', 0) + 1
        return self.stats['slot_14']

    def bookkeeping_15(self):
        # bookkeeping slot 15: counters only
        self.stats['slot_15'] = self.stats.get('slot_15', 0) + 1
        return self.stats['slot_15']

    def bookkeeping_16(self):
        # bookkeeping slot 16: counters only
        self.stats['slot_16'] = self.stats.get('slot_16', 0) + 1
        return self.stats['slot_16']

    def bookkeeping_17(self):
        # bookkeeping slot 17: counters only
        self.stats['slot_17'] = self.stats.get('slot_17', 0) + 1
        return self.stats['slot_17']

    def bookkeeping_18(self):
        # bookkeeping slot 18: counters only
        self.stats['slot_18'] = self.stats.get('slot_18', 0) + 1
        return self.stats['slot_18']

    def bookkeeping_19(self):
        # bookkeeping slot 19: counters only
        self.stats['slot_19'] = self.stats.get('slot_19', 0) + 1
        return self.stats['slot_19']

    def bookkeeping_20(self):
        # bookkeeping slot 20: counters only
        self.stats['slot_20'] = self.stats.get('slot_20', 0) + 1
        return self.stats['slot_20']

    def bookkeeping_21(self):
        # bookkeeping slot 21: counters only
        self.stats['slot_21'] = self.stats.get('slot_21', 0) + 1
        return self.stats['slot_21']

    def bookkeeping_22(self):
        # bookkeeping slot 22: counters only
        self.stats['slot_22'] = self.stats.get('slot_22', 0) + 1
        return self.stats['slot_22']

    def bookkeeping_23(self):
        # bookkeeping slot 23: counters only
        self.stats['slot_23'] = self.stats.get('slot_23', 0) + 1
        return self.stats['slot_23']

    def bookkeeping_24(self):
        # bookkeeping slot 24: counters only
        self.stats['slot_24'] = self.stats.get('slot_24', 0) + 1
        return self.stats['slot_24']

    def bookkeeping_25(self):
        # bookkeeping slot 25: counters only
        self.stats['slot_25'] = self.stats.get('slot_25', 0) + 1
        return self.stats['slot_25']

    def bookkeeping_26(self):
        # bookkeeping slot 26: counters only
        self.stats['slot_26'] = self.stats.get('slot_26', 0) + 1
        return self.stats['slot_26']

    def bookkeeping_27(self):
        # bookkeeping slot 27: counters only
        self.stats['slot_27'] = self.stats.get('slot_27', 0) + 1
        return self.stats['slot_27']

    def bookkeeping_28(self):
        # bookkeeping slot 28: counters only
        self.stats['slot_28'] = self.stats.get('slot_28', 0) + 1
        return self.stats['slot_28']

    def bookkeeping_29(self):
        # bookkeeping slot 29: counters only
        self.stats['slot_29'] = self.stats.get('slot_29', 0) + 1
        return self.stats['slot_29']

    def bookkeeping_30(self):
        # bookkeeping slot 30: counters only
        self.stats['slot_30'] = self.stats.get('slot_30', 0) + 1
        return self.stats['slot_30']

    def bookkeeping_31(self):
        # bookkeeping slot 31: counters only
        self.stats['slot_31'] = self.stats.get('slot_31', 0) + 1
        return self.stats['slot_31']

    def bookkeeping_32(self):
        # bookkeeping slot 32: counters only
        self.stats['slot_32'] = self.stats.get('slot_32', 0) + 1
        return self.stats['slot_32']

    def bookkeeping_33(self):
        # bookkeeping slot 33: counters only
        self.stats['slot_33'] = self.stats.get('slot_33', 0) + 1
        return self.stats['slot_33']

    def bookkeeping_34(self):
        # bookkeeping slot 34: counters only
        self.stats['slot_34'] = self.stats.get('slot_34', 0) + 1
        return self.stats['slot_34']

    def bookkeeping_35(self):
        # bookkeeping slot 35: counters only
        self.stats['slot_35'] = self.stats.get('slot_35', 0) + 1
        return self.stats['slot_35']

    def bookkeeping_36(self):
        # bookkeeping slot 36: counters only
        self.stats['slot_36'] = self.stats.get('slot_36', 0) + 1
        return self.stats['slot_36']

    def bookkeeping_37(self):
        # bookkeeping slot 37: counters only
        self.stats['slot_37'] = self.stats.get('slot_37', 0) + 1
        return self.stats['slot_37']

    def bookkeeping_38(self):
        # bookkeeping slot 38: counters only
        self.stats['slot_38'] = self.stats.get('slot_38', 0) + 1
        return self.stats['slot_38']

    def bookkeeping_39(self):
        # bookkeeping slot 39: counters only
        self.stats['slot_39'] = self.stats.get('slot_39', 0) + 1
        return self.stats['slot_39']
