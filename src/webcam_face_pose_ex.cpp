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

using namespace dlib;

// Averages points given to it and stores in given x and y references
template <class T>
void average(float& x, float& y, const dlib::full_object_detection& points, std::initializer_list<T> list) {
	x = 0;
	y = 0;
	int n = 0;
	for (auto elem : list) {
		x += static_cast<float>(points.part(elem).x());
		y += static_cast<float>(points.part(elem).y());
		n++;
	}
	x /= n;
	y /= n;
}


class ZmqConnector {
private:
	zmq::socket_t sub;
	zmq::socket_t push;
	static zmq::context_t context;

public:
	ZmqConnector() : sub(context, ZMQ_SUB), push(context, ZMQ_PUSH) {
		sub.connect("tcp://localhost:5556");
		push.connect("tcp://localhost:5557");
		const char *filter = "@face";
		sub.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));
		send("face starting");
	}

	virtual ~ZmqConnector() {
		sub.close();
		push.close();
	}

	void send(char* buffer) {
		zmq::message_t sending(strlen(buffer));
		memcpy(sending.data(), buffer, strlen(buffer));
		push.send(sending);
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
				}
				else if (strstr(message.c_str(), "marco")) {
					send("face polo");
				}
			}
		}
		return true;
	}
};

zmq::context_t ZmqConnector::context = zmq::context_t(1);


int main() {
    try {

		ZmqConnector zmq_connector;

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

				auto a = shapes[0];
				// x and y for right brow, left brow, right eye, left eye, nose, upper mouth, lower mouth
				float brx, bry, blx, bly, erx, ery, elx, ely, nx, ny, mux, muy, mlx, mly, jawx, jawy;
				average(brx, bry, shapes[0], { 17, 18, 19, 20, 21 });
				average(blx, bly, shapes[0], { 22, 23, 24, 25, 26 });
				average(erx, ery, shapes[0], { 36, 37, 38, 39, 40, 41 });
				average(elx, ely, shapes[0], { 42, 43, 44, 45, 46, 47 });
				average(nx, ny, shapes[0], { 27, 28, 29, 30, 31, 32, 33, 34, 35 });
				average(mux, muy, shapes[0], { 48, 49, 50, 51, 52, 53, 54 });
				average(mlx, mly, shapes[0], { 54, 55, 56, 57, 58, 59, 60 });
				average(jawx, jawy, shapes[0], { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 });

				// Create the tranmission buffer and load it with a string in the form of:
				// face position [comma seperated face point values]
				char buffer[512];
				sprintf(buffer,
					"face position %f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
					brx, bry, blx, bly, erx, ery, elx, ely, nx, ny, mux, muy, mlx, mly, jawx, jawy
				);
				
				zmq_connector.send(buffer);
				alive = zmq_connector.receive();
			}
		}
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

