#!/usr/bin/python -u

import time

def out(*args):
	print "%f %f %f %d" % args

while 1:
	out(1 ,.25,  0  , 0)
	time.sleep(0.03)