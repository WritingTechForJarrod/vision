import cv2
import numpy as np
import PIL
import math

camera = cv2.VideoCapture(0)
mode = 1
mode_switched = False

def get_image():
	retval, im = camera.read()
	return im
 
print("Opening window...")
cv2.namedWindow('webcam', cv2.WINDOW_NORMAL)
cv2.resizeWindow('webcam', 640, 480)
print("Taking image...")

camera_capture = None
current_frame = get_image()
grey_frame = get_image()
grey_frame.fill(127)
captures = [grey_frame, grey_frame, grey_frame]

while(True):
	global mode
	global mode_switched
	captures[2] = captures[1]
	captures[1] = captures[0]
	captures[0] = cv2.medianBlur(cv2.flip(get_image(), 1), 3)
	#ret, captures[0] = cv2.threshold(captures[0], 150, 255, cv2.THRESH_BINARY)
	v1 = np.subtract(captures[1], captures[2])
	v2 = np.subtract(captures[0], captures[1])
	current_frame = captures[0]/2 + captures[1]/2# + captures[2]/3
	#current_frame = np.subtract(v1, v2)
	#np.add(current_frame, grey_frame, current_frame)
	# current_frame = np.fft.fft2(current_frame) # remove complex part
	greyscale_frame = np.empty((480, 640), np.uint8)
	greyscale_frame = current_frame[:,:,0]/3 + current_frame[:,:,1]/3 + current_frame[:,:,2]/3
	#cv2.mixChannels(current_frame, greyscale_frame, [[0,0],[1,0],[2,0]])
	#greyscale_frame = cv2.merge((current_frame[:,:,0],current_frame[:,:,1],current_frame[:,:,2]))
	screen_w = 640
	screen_h = 480
	#current_frame = cv2.imread("lines.png")
	greyscale_frame = current_frame[:,:,0].copy()
	white_frame = np.empty((480, 640), np.uint8)
	white_frame.fill(255)
	greyscale_frame = cv2.Canny(np.subtract(white_frame, greyscale_frame), 100, 250)
	#current_frame.fill(0)
	lines = cv2.HoughLines(greyscale_frame, 2, math.pi/180, 150)
	if lines is not None:
		print(str(len(lines)) + ' lines detected')
		for line in lines:
			rho = line[0,0]
			theta = line[0,1]
			try:
				x1, y1 = rho * math.cos(theta), rho * math.sin(theta)
				x2 = x1 + 1000 * math.sin(theta)
				y2 = y1 + 1000 * -math.cos(theta)
				x1 = x1 - 1000 * math.sin(theta)
				y1 = y1 - 1000 * -math.cos(theta)
				#greyscale_frame[y,x] = greyscale_frame[y,x] + 255
				#current_frame[y,x] = current_frame[y,x] - [1,1,1]
				cv2.line(current_frame, (int(x1), int(y1)), (int(x2), int(y2)), (127, 0, 127))
				cv2.line(greyscale_frame, (int(x1), int(y1)), (int(x2), int(y2)), 127)
				#print(current_frame[y,x])
			except IndexError:
				pass
	if mode == 1:
		cv2.imshow('webcam', current_frame)
	if mode == 0:
		#cv2.equalizeHist(greyscale_frame, greyscale_frame)
		cv2.imshow('webcam', greyscale_frame)

	key = cv2.waitKey(1)
	if key == 27: # Esc pressed
		break
	elif key == ord('m'):
		if not mode_switched:
			mode = (mode + 1) % 2
			mode_switched = True
	elif key == ord(' '):
		cv2.imwrite('screenshot_rgb.png', current_frame)
		cv2.imwrite('screenshot_bw.png', greyscale_frame)
	else:
		mode_switched = False

cv2.destroyAllWindows()
 
# You'll want to release the camera, otherwise you won't be able to create a new
# capture object until your script exits
del(camera)