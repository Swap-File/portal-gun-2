#!/usr/bin/python -u

import time

def out(*args):
	print "%d" % args

i =0;
while 1:
	out(i)
	time.sleep(0.5)
	i+=1
	if i == 4: i = 10
	if i == 16: i = 20
	if i > 39: i = 0