#!/usr/bin/python -u

import struct

file = open( "/dev/input/mice", "rb" );

mode = 0

def getMouseEvent():
  global mode
  buf = file.read(3);
  button = ord( buf[0] );
  bLeft = button & 0x1;
  bMiddle = ( button & 0x4 ) > 0;
  bRight = ( button & 0x2 ) > 0;
  x,y = struct.unpack( "bb", buf[1:] );
  if bLeft:
    mode = mode + 1
    if mode > 10:
      mode = 0
  print '%d %d %d' % (mode, x, y)

  # return stuffs

while( 1 ):
  getMouseEvent();
file.close();