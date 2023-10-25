TESTCASE 7:
Our result unblocks child2 before child1c, which is due to the dispatcher implementation 
that we don't have control of. Other than that, everything else matches up.

TESTCASE 10, 11, 12:
Our times are close to correct, but not exactly the same number. They are small enough
and can be explain because we're using a different computer. The ratios and the differences
between our times and the expected output times seem to be the same.

TESTCASE 20:
As Russ mentioned that this was possible, a race condition resulted in one line of output being
printed in a different order.
