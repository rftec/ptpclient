#!/usr/bin/python
from pyptp import Camera

VID_SONY = 0x054C
PID_SONY_A6000 = 0x094E

def main():
	cam = Camera(vid=VID_SONY, pid=PID_SONY_A6000)
	cam.setparams(drive='low')
	#cam.setparams(iso=1000)
	#cam.setparams(shutter=(1, 2000))
	
if __name__ == '__main__':
	main()

