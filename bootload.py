#!/usr/bin/env python2

import os
import sys
import shutil
import serial

img_path = '/mnt/helios_nfs/home/rounin/suburbanmarine/projects/can_usb_fw/build/ram/release/canusbfdiso.bin'
tty_path = '/dev/ttyACM0'

img_info = os.stat(img_path)

download_cmd = 'download:{0:08x}\r'.format(img_info.st_size)
download_resp = 'DATA{0:08x}'.format(img_info.st_size)
flash_cmd    = 'flash:app.bin\r'
ok_resp      = 'OKAY'

tty_port = serial.Serial(tty_path, 115200, timeout=10)

print 'Device open'

print 'Starting Download'
tty_port.write(download_cmd)
tty_port.flush()
tty_line = tty_port.read_until(size=12)

if tty_line != download_resp:
	print 'lost dl start ok, expected {0} got {1}'.format(download_resp, tty_line)
	sys.exit(-1)

img_file = open(img_path, 'rb')
img_line = img_file.readline()

while img_line:
	tty_port.write(img_line)
	img_line = img_file.readline()

img_file.close()

tty_port.flush()

tty_line = tty_port.read_until(size=4)
if tty_line != ok_resp:
	print 'lost dl finish ok, got {0}'.format(tty_line)
	sys.exit(-1)

print 'Download Complete'

print 'Starting Flash'

tty_port.write(flash_cmd)
tty_port.flush()

tty_line = tty_port.read_until(size=4)
if tty_line != ok_resp:
	print 'lost flash ok, got {0}'.format(tty_line)
	sys.exit(-1)

print 'Flash complete'
sys.exit(0)
