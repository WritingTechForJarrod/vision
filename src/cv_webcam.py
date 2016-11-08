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
 
screen_w = 640
screen_h = 480
print("Opening window...")
cv2.namedWindow('webcam', cv2.WINDOW_NORMAL)
cv2.resizeWindow('webcam', screen_w, screen_h)
print("Taking images...")

white_frame_gs = np.empty((480, 640), np.uint8)
white_frame_gs.fill(255)
white_frame = np.empty((480, 640,3), np.uint8)
white_frame.fill(255)
gray_frame = get_image()
gray_frame.fill(127)
NUM_FRAMES = 3
frames = [gray_frame.copy() for x in range(0, NUM_FRAMES)]
grayscale_frames = [cv2.cvtColor(gray_frame, cv2.COLOR_BGR2GRAY) for x in range(0, NUM_FRAMES)]

def cycle(frames, new_frame=None):
    for i in range(1, NUM_FRAMES):
        frames[NUM_FRAMES-i] = frames[NUM_FRAMES-i-1]
    if new_frame is None:
        frames[0] = cv2.medianBlur(cv2.flip(get_image(), 1), 3)
    else:
        frames[0] = new_frame

class Drawable():
    def draw(self, canvas):
        raise NotImplementedError

class LineDetector(Drawable):
    def __init__(self):
        self.lines = None

    def detect(self, grayscale_input):
        self.lines = cv2.HoughLines(cv2.Canny(np.subtract(white_frame_gs, grayscale_input), 100, 250), 2, math.pi/60, 150)

    def draw(self, canvas):
        lines = self.lines
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
                    cv2.line(canvas, (int(x1), int(y1)), (int(x2), int(y2)), (127, 0, 127))
                    #cv2.line(grayscale_frame, (int(x1), int(y1)), (int(x2), int(y2)), 127)
                except IndexError:
                    pass

face_cascade = cv2.CascadeClassifier('../xml/haarcascade_frontalface_default.xml')
eye_cascade = cv2.CascadeClassifier('../xml/haarcascade_eye.xml')
ld = LineDetector()

''' Bits of junk
current_frame = np.fft.fft2(current_frame) # remove complex part
cv2.equalizeHist(grayscale_frame, grayscale_frame)

'''
def screenshot():
    cv2.imwrite('screenshot_rgb.png', current_frame)
    cv2.imwrite('screenshot_bw.png', grayscale_frame)

def toggle_mode(mode_to_toggle):
    if mode_to_toggle in mode:
        mode.remove(mode_to_toggle)
    else:
        mode.append(mode_to_toggle)
        print('mode ' + mode_to_toggle + ' engaged')

process_key = {}
process_key[' '] = screenshot
process_key['l'] = lambda: toggle_mode('lines')
process_key['m'] = lambda: toggle_mode('grayscale')
process_key['t'] = lambda: toggle_mode('threshold')
process_key['e'] = lambda: toggle_mode('equalize')
process_key['d'] = lambda: toggle_mode('dft')
process_key['g'] = lambda: toggle_mode('edge')
process_key['h'] = lambda: toggle_mode('haar')
process_key['s'] = lambda: toggle_mode('sift')
process_key['c'] = lambda: toggle_mode('corner')
process_key['f'] = lambda: toggle_mode('file')

mode = []
#process_key['d']()

def on_mouse(event, x, y, flags, params):
    print(x, y)
cv2.setMouseCallback('mouse input', on_mouse)

def get_derivative(frames, degree=1):
    assert((degree < NUM_FRAMES) is True)
    if degree == 1:
        return frames[1]/2 - frames[0]/2
    if degree == 2:
        return frames[2]/2 - frames[1]/2 + frames[0]

while(True):
    cycle(frames) # get new frame, also does gaussian filter
    cycle(grayscale_frames, cv2.cvtColor(frames[0], cv2.COLOR_BGR2GRAY))

    if 'derive' in mode:
        current_frame = (frames[1]/2 - frames[0]/2) + gray_frame
        current_frame = cv2.blur(current_frame, (3,3))
        grayscale_frame = cv2.cvtColor(current_frame, cv2.COLOR_BGR2GRAY)
        lower = 141
        ret, grayscale_frame2 = cv2.threshold(grayscale_frame, lower, 255, cv2.THRESH_TOZERO)
        grayscale_frame = white_frame_gs - grayscale_frame
        ret, grayscale_frame = cv2.threshold(grayscale_frame, lower, 255, cv2.THRESH_TOZERO)
        grayscale_frame = grayscale_frame + grayscale_frame2
    else:
        current_frame = frames[0]/2 + frames[1]/2 # low-pass time-wise noise filter
        grayscale_frame = cv2.cvtColor(current_frame, cv2.COLOR_BGR2GRAY)

    if 'haar' in mode:
        #print(reduce(lambda x, y: x + y, grayscale_frame[:,:]))
        faces = face_cascade.detectMultiScale(grayscale_frame, 1.3, 5)
        for (x,y,w,h) in faces:
            cv2.rectangle(current_frame, (x,y), (x+w,y+h), (255,0,0), 2)
            roi_gray = grayscale_frame[y:y+h, x:x+w]
            roi_color = current_frame[y:y+h, x:x+w]
            eyes = eye_cascade.detectMultiScale(roi_gray)
            for (ex,ey,ew,eh) in eyes:
                cv2.rectangle(roi_color, (ex,ey), (ex+ew,ey+eh), (0,255,0), 2)

    if 'edge' in mode:  
        grayscale_frame = cv2.Canny(cv2.cvtColor(current_frame, cv2.COLOR_BGR2GRAY), 40, 255)
    if 'dft' in mode:
        dft = cv2.dft(np.float32(grayscale_frame), flags = cv2.DFT_COMPLEX_OUTPUT)
        dft_shift = np.fft.fftshift(dft)
        grayscale_frame = np.log(cv2.magnitude(dft_shift[:,:,0],dft_shift[:,:,1]))
        max_val = np.max(grayscale_frame)
        grayscale_frame /= (max_val/256)
        grayscale_frame = np.array(grayscale_frame, np.uint8)

    ld.detect(grayscale_frame)

    if 'grayscale' in mode: 
        current_frame = grayscale_frame
    if 'threshold' in mode:
        ret, current_frame = cv2.threshold(current_frame, 150, 255, cv2.THRESH_BINARY)
    if 'lines' in mode:
        ld.draw(current_frame)
    if 'sum' in mode:
        num_row, num_col = (grayscale_frame.shape[0], grayscale_frame.shape[1])
        row_average = np.ndarray((num_row, 1), dtype=float)
        col_average = np.ndarray((1, num_col), dtype=float)
        row_ones = np.ndarray((num_row, 1), dtype=float)
        row_ones.fill(1)
        col_ones = np.ndarray((1, num_col), dtype=float)
        col_ones.fill(1)
        for row in range(0, num_row):
            average = np.average(grayscale_frame[row]) 
            row_average[row] = average
        for col in range(0, num_col):
            average = np.average(grayscale_frame[:,col])
            col_average[0][col] = average
        grayscale_frame = row_ones * col_average / 512 + row_average * col_ones / 512
        current_frame = grayscale_frame
    if 'file' in mode:
        current_frame = cv2.imread("test.png")
        grayscale_frame = cv2.cvtColor(current_frame, cv2.COLOR_BGR2GRAY)
    if 'corner' in mode:
        #ret, grayscale_frame = cv2.threshold(grayscale_frame, 127, 255, cv2.THRESH_BINARY)
        grayscale_frame = cv2.cornerHarris(grayscale_frame, 7, 5, 0.00001)
        thresh = 0.00000000000000001
        grayscale_frame[grayscale_frame < thresh] = thresh
        grayscale_frame = 10*np.log(np.log(grayscale_frame + 1) + 1)
        max_val = np.max(grayscale_frame)
        #grayscale_frame = grayscale_frame / max_val
        #ret, grayscale_frame = cv2.threshold(grayscale_frame, 0.01, 255, cv2.THRESH_BINARY)
        current_frame = grayscale_frame
    if 'sift' in mode:
        sift = cv2.xfeatures2d.SIFT_create()
        kp = sift.detect(gray,None)
        img=cv2.drawKeypoints(gray,kp,img,flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)

    cv2.imshow('webcam', current_frame)

    key = cv2.waitKey(1)
    if key == 27: # Esc pressed
        break
    else:
        try:
            process_key[chr(key)]()
        except ValueError:
            pass # waitKey timed out, is fine
        except KeyError:
            print('No function for key ' + str(key) + ' found')

cv2.destroyAllWindows()
del(camera)