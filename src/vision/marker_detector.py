from __future__ import print_function, division, absolute_import, unicode_literals
import cv2
import logging
import numpy as np

class ContrastFilter(object):
	def __init__(self):
		logging.getLogger('ContrastFilter').debug('AYUPP')

def beside(px0,px1):
	if abs(px0[0]-px1[0]) <= 1:
		if abs(px0[1]-px1[1]) <= 1:
			return True
	return False

def belongs_to(px,group):
	for gpx in group:
		if beside(px,gpx): return True
	return False

if __name__ == '__main__':
	mainlog = logging.getLogger('main')
	logging.basicConfig(level=logging.DEBUG)

	screen_w = 640
	screen_h = 480
	mainlog.debug('Opening window...')
	window_name = 'main'
	cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
	cv2.resizeWindow(window_name, screen_w, screen_h)

	camera = cv2.VideoCapture(0)
	def get_image():
	    retval, im = camera.read()
	    return im

	user_in = ''
	mainlog.debug('Press x to quit')
	last_result = None

	while user_in != ord('x'):
		global last_result
		img = cv2.flip(get_image(),1)
		#img = cv2.medianBlur(img,5)
		#img = cv2.imread('../../img/dot_rg_low_light.png')
		shape = img.shape
		result = np.empty(shape,np.uint8)
		final = np.empty(shape,np.uint8)
		final.fill(0)
		if len(shape) < 3:
			raise ValueError

		result = cv2.cvtColor(img,cv2.COLOR_BGR2HSV)
		result[:,:,2] = cv2.inRange(result[:,:,1],64,255)
		result = cv2.cvtColor(result,cv2.COLOR_HSV2BGR)
		result = result[:,:,2]//2 - result[:,:,1]//2 + 127
		
		kernel = np.zeros((9,1),np.float32)
		kernel[0,0] = -0.0
		kernel[1,0] = -0.3
		kernel[2,0] = -0.3
		kernel[3,0] = -0.3
		kernel[4,0] = 0.0
		kernel[5,0] = 0.3
		kernel[6,0] = 0.3
		kernel[7,0] = 0.3
		kernel[8,0] = 0.0

		final = cv2.filter2D(result,-1,kernel)
		final = cv2.medianBlur(final,3)
		final = cv2.inRange(final,127,255)

		if last_result is not None:
			real_final = last_result//2+final//2
		else:
			real_final = final
		real_final = cv2.inRange(real_final,162,255)
		
		ret,contours,hierarchy = cv2.findContours(real_final,cv2.RETR_LIST,cv2.CHAIN_APPROX_NONE)
		
		if len(contours) > 0:
			#longest = max(contours,key=len)
			contours.sort(lambda x,y: cmp(len(x),len(y)))
			longest = contours[-1]
	
			circle_r = 7
			circle_t = 2
			circle_c = np.array([255,0,0])

			for contour in [longest]:
				M = cv2.moments(contour)
				if M['m00'] > 1:
					centroid_x = int(M['m10']/M['m00'])
					centroid_y = int(M['m01']/M['m00'])
					cv2.circle(img,(centroid_x,centroid_y),circle_r,circle_c,circle_t)				
					cv2.circle(result,(centroid_x,centroid_y),circle_r,circle_c,circle_t)
					cv2.circle(final,(centroid_x,centroid_y),circle_r,circle_c,circle_t)

			#px_pts = cv2.findNonZero(final)
		
		cv2.imshow(window_name,img)
		last_result = final
		user_in = cv2.waitKey(1)

	cv2.destroyAllWindows()
	

