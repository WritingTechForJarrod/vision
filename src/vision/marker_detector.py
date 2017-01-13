from __future__ import print_function, division, absolute_import, unicode_literals
import cv2
import logging
import numpy as np

class ContrastFilter(object):
	def __init__(self):
		logging.getLogger('ContrastFilter').debug('AYUPP')

if __name__ == '__main__':
	mainlog = logging.getLogger('main')
	logging.basicConfig(level=logging.DEBUG)

	screen_w = 640
	screen_h = 480
	mainlog.debug('Opening window...')
	window_name = 'main'
	cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
	cv2.resizeWindow(window_name, screen_w, screen_h)

	c = ContrastFilter()
	img = cv2.imread('../../img/dot_rg_low_light.png')
	#img = cv2.pyrDown(img)
	#img = cv2.pyrDown(img)
	shape = img.shape
	mainlog.debug('Img has shape of '+str(shape))
	result = np.empty(shape,np.uint8)
	final = np.empty(shape,np.uint8)
	final.fill(0)
	if len(shape) < 3:
		raise ValueError

	#img = cv2.medianBlur(cv2.flip(img, 1), 3)

	result = cv2.cvtColor(img,cv2.COLOR_BGR2HSV)
	mode = 't'
	if mode is 't':
		lower = np.array([0,0,0],np.uint8)
		upper = np.array([255,255,255],np.uint8)
		thinger = cv2.inRange(result,lower,upper)

		result[:,:,2] = cv2.inRange(result[:,:,1],200,255)
		'''
		stride = 5
		for y in range(shape[0]-stride):
			for x in range(shape[1]):
				px0 = img[y,x]
				px1 = img[y+stride,x]
				result[y,x,0] = (int(px0[0]) - int(px1[0])) + 127
				#result[y,x,1] = 255#result[y,x,0]
				result[y,x,2] = 255#result[y,x,0]
		'''
		stride = 5
		result = cv2.cvtColor(result,cv2.COLOR_HSV2BGR)
		result = result[:,:,2]//2 - result[:,:,1]//2 + 127
		'''
		g_acc = 0
		r_acc = 0
		for x in range(shape[1]):
			for y in range(shape[0]-stride):
				px0 = result[y,x]
				px1 = result[shape[0]-1-y,x]
				#if px0[2] > 127:
					#final[y,x] = (0,0,255)
				if px0[1] > 127:
					g_acc += 1
					final[y,x][1] = 10*g_acc
				else:
					if g_acc > 0:
						final[y,x][1] = 10*g_acc
						g_acc = 0
				if px1[2] > 127:
					r_acc += 1
					final[shape[0]-1-y,x][2] = 10*r_acc
				else:
					if r_acc > 0:
						final[shape[0]-1-y,x][2] = 10*r_acc
						r_acc = 0
		'''
		kernel = np.zeros((5,1),np.float32)
		mainlog.debug("Kernel:\n"+str(kernel))
		kernel[0,0] = -0.5
		kernel[1,0] = -0.2
		kernel[2,0] = 0.0	
		kernel[3,0] = 0.2
		kernel[4,0] = 0.5
		final = cv2.filter2D(result,-1,kernel)
		final = cv2.inRange(final,127,255)
					
	if mode is 'hue':
		result[:,:,1] = result[:,:,0]
		result[:,:,2] = result[:,:,0]
	if mode is 'sat':
		result[:,:,0] = result[:,:,1]
		result[:,:,2] = result[:,:,1]
	if mode is 'val':
		result[:,:,0] = result[:,:,2]
		result[:,:,1] = result[:,:,2]
	#result = img
	#final = cv2.Sobel(img,0,0,0)
	cv2.imshow(window_name,final)
	cv2.waitKey(0)
	cv2.destroyAllWindows()
	

