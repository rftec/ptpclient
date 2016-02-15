#!/usr/bin/python
from pyptp import Camera
from time import sleep
from os import mkdir

VID_SONY = 0x054C
PID_SONY_A6000 = 0x094E

def main():
	try:
		mkdir('images')
	except OSError:
		pass
	
	cam = Camera(vid=VID_SONY, pid=PID_SONY_A6000, image_dir='images')
	cam.setparams(drive='low')
	cam.setparams(iso=1000)
	cam.setparams(shutter=(1, 2000))
	cam.setparams(fnumber=4.0)
	#cam.start()
	#sleep(10)
	#cam.stop()
	
if __name__ == '__main__':
	main()

