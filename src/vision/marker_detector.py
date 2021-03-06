from __future__ import print_function, division, absolute_import, unicode_literals
import cv2
import logging
import numpy as np
import time

class ContrastFilter(object):
	def __init__(self):
		logging.getLogger('ContrastFilter').debug('AYUPP')

def within_range(px0,px1,r):
	if abs(px0[0]-px1[0]) <= r:
		if abs(px0[1]-px1[1]) <= r:
			return True
	return False

def belongs_to(px,group,r):
	for gpx in group:
		if within_range(px,gpx,r): return True
	return False

def contour_area(contour):
	return cv2.moments(contour)['m00'] 

def contour_position(contour):
	try:
		M = cv2.moments(contour)
		if M['m00'] > 0:
			return (int(M['m10']/M['m00']),int(M['m01']/M['m00']))
	except TypeError:
		return None
	return None

class ContourWrapper(object):
	def __init__(self,contour):
		self._history = []
		self._range = 10
		self.updated = False
		self.update(contour)

	def is_same_contour(self,contour):
		this_pos = self.last_position()
		other_pos = contour_position(contour)
		if this_pos is not None and other_pos is not None:
			return within_range(this_pos,other_pos,self._range)

	def last_position(self):
		if len(self._history) > 0:
			return self._history[-1][0]
		else:
			return None

	def last_area(self):
		if len(self._history) > 0:
			return self._history[-1][1]
		else:
			return None

	def update(self,contour):
		pos = contour_position(contour)
		area = contour_area(contour)
		if pos is not None:
			self._history.append([pos,area])
			self.updated = True

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
	last_contours = []

	t0 = time.clock()

	shape = get_image().shape
	result = np.empty(shape,np.uint8)
	final = np.empty(shape,np.uint8)
	final.fill(0)

	datetime_start = time.strftime('%Y%m%d%H%M%S')
	with open('../../gen/'+datetime_start+'.vcr','w') as f:
		while user_in != ord('x'):
			global last_result
			img = cv2.flip(get_image(),1)
			#img = cv2.medianBlur(img,5)
			#img = cv2.imread('../../img/dot_rg_low_light.png')
			
			if len(shape) < 3:
				raise ValueError

			result = cv2.cvtColor(img,cv2.COLOR_BGR2HSV)
			result[:,:,2] = cv2.inRange(result[:,:,2],63,255)
			#result[:,:,2] = cv2.inRange(result[:,:,1],64,255)
			result = cv2.cvtColor(result,cv2.COLOR_HSV2BGR)
			result = result[:,:,1]//2 - result[:,:,2]//2 + 127
			
			result = cv2.pyrDown(result)
			#result = cv2.pyrDown(result)
			k_array =  [[-3,-1,-1,-1,-3],
						[-1, 1, 3, 1,-1],
						[-1, 3, 6, 3,-1],
						[-1, 1, 3, 1,-1],
						[-3,-1,-1,-1,-3]]
			kernel = np.array(k_array,np.float32)
			final = cv2.filter2D(result,-1,kernel)
			#final = cv2.medianBlur(final,3)
			final = cv2.inRange(final,63,255)

			if 1:
				# TODO make this less ugly-like. maybe just store default cv2 types and operate on them?
				ret,contours,hierarchy = cv2.findContours(final,cv2.RETR_LIST,cv2.CHAIN_APPROX_NONE)
				new_contours = [ContourWrapper(contour) for contour in contours]
				
				new_contours[:] = [c for c in new_contours if c.last_area() > 1.0]

				t = time.clock() - t0
				f.write('t '+str(t)+'\n')
				cv2.imwrite('../../img/captured/'+datetime_start+'_'+str(t)+'_.png',img)

				for contour in new_contours:
					area = contour.last_area()
					pos = contour.last_position()
					if area is not None and pos is not None:
						f.write(str(area)+' '+str(pos[0])+' '+str(pos[1])+'\n')

				if 1:
					for last_contour in last_contours:
						last_contour.updated = False
						for new_contour in contours:
							if last_contour.is_same_contour(new_contour):
								last_contour.update(new_contour)
								break

					if len(last_contours) > 1:
						circle_r = 7
						circle_t = 2
						circle_c = np.array([255,0,0])
						
						for contour in last_contours:
							if contour.updated:
								x,y = contour.last_position()
								cv2.circle(img,(x,y),circle_r,circle_c,circle_t)
								cv2.circle(result,(x,y),circle_r,circle_c,circle_t)
								cv2.circle(final,(x,y),circle_r,circle_c,circle_t)

				last_contours = new_contours

			cv2.imshow(window_name,img)
			last_result = final
			user_in = cv2.waitKey(1)

		cv2.destroyAllWindows()
	

