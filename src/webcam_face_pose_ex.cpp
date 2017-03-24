// The contents of this file are in the public domain. See LICENSE_FOR_EXAMPLE_PROGRAMS.txt
/*

    This example program shows how to find frontal human faces in an image and
    estimate their pose.  The pose takes the form of 68 landmarks.  These are
    points on the face such as the corners of the mouth, along the eyebrows, on
    the eyes, and so forth.  
    

    This example is essentially just a version of the face_landmark_detection_ex.cpp
    example modified to use OpenCV's VideoCapture object to read from a camera instead 
    of files.


    Finally, note that the face detector is fastest when compiled with at least
    SSE2 instructions enabled.  So if you are using a PC with an Intel or AMD
    chip then you should enable at least SSE2 instructions.  If you are using
    cmake to compile this program you can enable them by using one of the
    following commands when you create the build project:
        cmake path_to_dlib_root/examples -DUSE_SSE2_INSTRUCTIONS=ON
        cmake path_to_dlib_root/examples -DUSE_SSE4_INSTRUCTIONS=ON
        cmake path_to_dlib_root/examples -DUSE_AVX_INSTRUCTIONS=ON
    This will set the appropriate compiler options for GCC, clang, Visual
    Studio, or the Intel compiler.  If you are using another compiler then you
    need to consult your compiler's manual to determine how to enable these
    instructions.  Note that AVX is the fastest but requires a CPU from at least
    2011.  SSE4 is the next fastest and is supported by most current machines.  
*/

/*
	This file has been modified by max@theprogrammingclub.com to serve the purposes of JRRD
	https://github.com/writingtechforjarrod/vision/
*/

#define _WINSOCKAPI_ 
#include <iostream>
#include <chrono>
#include <thread>
#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <zmq.hpp>
#include <cmath>

using namespace dlib;

struct vector2f {
	float x;
	float y;
	vector2f operator+(const vector2f& other) {
		float new_x = x + other.x;
		float new_y = y + other.y;
		return vector2f{ new_x, new_y };
	}
	vector2f operator/(const float& scalar) {
		float new_x = x / scalar;
		float new_y = y / scalar;
		return vector2f{ new_x, new_y };
	}
};

// Averages points given to it and stores in given x and y references
void average(const dlib::full_object_detection& points, int start, int stop, vector2f& v) {
	v.x = 0;
	v.y = 0;
	for (int n = start; n <= stop; n++) {
		v.x += static_cast<float>(points.part(n).x());
		v.y += static_cast<float>(points.part(n).y());
	}
	v.x /= stop - start + 1;
	v.y /= stop - start + 1;
}

// Euclidean distance
float distance(float x1, float y1, float x2, float y2) {
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

float distance(vector2f v1, vector2f v2) {
	return distance(v1.x, v1.y, v2.x, v2.y);
}

enum Mode {
	RAW,
	NORMALIZED
};

class Face {
private:
	zmq::socket_t sub;
	zmq::socket_t push;
	static zmq::context_t context;
	Mode mode;

public:
	Face() : sub(context, ZMQ_SUB), push(context, ZMQ_PUSH), mode(Mode::NORMALIZED) {
		sub.connect("tcp://localhost:5556");
		push.connect("tcp://localhost:5557");
		const char *filter = "@face";
		sub.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));
		send("face starting");
	}

	virtual ~Face() {
		sub.close();
		push.close();
	}

	void send(char* buffer) {
		zmq::message_t sending(strlen(buffer));
		memcpy(sending.data(), buffer, strlen(buffer));
		push.send(sending);
	}

	Mode get_mode() const {
		return mode;
	}

	bool receive() {
		zmq::message_t received;
		while (sub.recv(&received, ZMQ_DONTWAIT)) {
			std::istringstream iss(static_cast<char*>(received.data()));
			std::string topic;
			std::string message;
			iss >> topic >> message;
			if (strstr(topic.c_str(), "@face")) {
				if (strstr(message.c_str(), "stop")) {
					char* exit_message = "face stopping";
					send(exit_message);
					return false;
				} else if (strstr(message.c_str(), "marco")) {
					send("face polo");
				} else if (strstr(message.c_str(), "mode raw")) {
					mode = Mode::RAW;
					send("face ack raw");
				} else if (strstr(message.c_str(), "mode normalized")) {
					mode = Mode::NORMALIZED;
					send("face ack normalized");
				}
			}
		}
		return true;
	}
};

zmq::context_t Face::context = zmq::context_t(1);

int main() {
    try {

		Face face;

        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            std::cerr << "Unable to connect to camera" << std::endl;
            return 1;
        }

        // Load face detection and pose estimation models.
        frontal_face_detector detector = get_frontal_face_detector();
        shape_predictor pose_model;
		try {
			deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;
		} catch (serialization_error& e) {
			try {
				deserialize("../bin/face/shape_predictor_68_face_landmarks.dat") >> pose_model;
			} catch (serialization_error& e) {
				std::cout << "You need dlib's default face landmarking model file to run this example." << std::endl;
				std::cout << "You can get it from the following URL: " << std::endl;
				std::cout << "http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" << std::endl;
				std::cout << std::endl << e.what() << std::endl;
			}
		}

        // Grab and process frames until the main window is closed by the user.
		bool alive(true);
		int index(0);

		while (alive) {
			// Grab a frame
			cv::Mat temp;
			cap >> temp;
			// Turn OpenCV's Mat into something dlib can deal with.  Note that this just
			// wraps the Mat object, it doesn't copy anything.  So cimg is only valid as
			// long as temp is valid.  Also don't do anything to temp that would cause it
			// to reallocate the memory which stores the image as that will make cimg
			// contain dangling pointers.  This basically means you shouldn't modify temp
			// while using cimg.
			cv_image<bgr_pixel> cimg(temp);

			// Detect faces
			std::vector<rectangle> faces = detector(cimg);

			// Don't hog the CPU, this limits updates to max 50Hz
			std::this_thread::sleep_for(std::chrono::milliseconds(20));

			// Find the pose of each face
			std::vector<full_object_detection> shapes;
			for (unsigned long i = 0; i < faces.size(); ++i)
				shapes.push_back(pose_model(cimg, faces[i]));

			if (faces.size() > 0) {
				// Parts of face
				// 1 - 16 : jawline
				// 17 - 21 : right eyebrow
				// 22 - 26 : left eyebrow
				// 27 - 35 : nose
				// 36 - 41 : right eye
				// 42 - 47 : left eye
				// 48 - 54 : upper lip
				// 54 - 60 : lower lip
				// 61,62,63 : upper lip
				// 48,60,54,64 : r-l corners (lower, upper?)
				// 65,66,67 : lower lip

				auto& detected_face = shapes[0];

				vector2f brows[2];
				vector2f eyes[2];
				vector2f nose;
				vector2f jaw;
				vector2f upper_mouth;
				vector2f lower_mouth;
				vector2f face_center;

				average(detected_face, 17, 21, brows[0]);
				average(detected_face, 22, 26, brows[1]);
				average(detected_face, 36, 41, eyes[0]);
				average(detected_face, 42, 47, eyes[1]);
				average(detected_face, 27, 35, nose);
				average(detected_face, 1, 16, jaw);
				average(detected_face, 48, 54, upper_mouth);
				average(detected_face, 54, 60, lower_mouth);
				average(detected_face, 1, 68, face_center);

				// Create the tranmission buffer and load it with a string in the form of:
				// face position [comma seperated face point values]
				char buffer[512];
				if (face.get_mode() == Mode::RAW) {
					sprintf(buffer,
						"face position %f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
						brows[0].x, brows[0].y, brows[1].x, brows[1].y,
						eyes[0].x, eyes[0].y, eyes[1].x, eyes[1].y,
						nose.x, nose.y,
						jaw.x, jaw.y,
						upper_mouth.x, upper_mouth.y, lower_mouth.x, lower_mouth.y
					);
				} else if (face.get_mode() == Mode::NORMALIZED) {
					float face_vector[6]; 
					// All important face points calculated relative to center
					// In order, average brow distance, average eye distance, nose distance, jaw distance
					//	upper mouth distance, lower mouth distance

					vector2f avg_brow = (brows[0] + brows[1]) / 2;
					vector2f avg_eye = (eyes[0] + eyes[1]) / 2;
					face_vector[0] = distance(avg_brow, face_center);
					face_vector[1] = distance(avg_eye, face_center);
					face_vector[2] = distance(nose, face_center);
					face_vector[3] = distance(jaw, face_center);
					face_vector[4] = distance(upper_mouth, face_center);
					face_vector[5] = distance(lower_mouth, face_center);
					
					// Find length of face vector
					float length(0);
					for (float f : face_vector) {
						length += (f * f);
					}
					length = sqrt(length);
					
					// Normalize face vector
					for (float& f : face_vector) {
						f = f / length;
					}

					auto& fv = face_vector;
					sprintf(buffer,
						"face vector %f,%f,%f,%f,%f,%f",
						fv[0], fv[1], fv[2], fv[3], fv[4], fv[5]
					);
				}
				
				face.send(buffer);
				alive = face.receive();
			}
		}
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

