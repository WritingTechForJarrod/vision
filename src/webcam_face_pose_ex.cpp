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
#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <zmq.hpp>

using namespace dlib;

int main() {
    try {
		zmq::context_t context(1);
		zmq::socket_t sub(context, ZMQ_SUB);
		zmq::socket_t push(context, ZMQ_PUSH);
		sub.connect("tcp://localhost:5556");
		push.connect("tcp://localhost:5557");
		const char *filter = "face";
		sub.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));

        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            std::cerr << "Unable to connect to camera" << std::endl;
            return 1;
        }

        // Load face detection and pose estimation models.
        frontal_face_detector detector = get_frontal_face_detector();
        shape_predictor pose_model;
        deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;

        // Grab and process frames until the main window is closed by the user.
		bool alive(true);

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
			// Find the pose of each face
			std::vector<full_object_detection> shapes;
			for (unsigned long i = 0; i < faces.size(); ++i)
				shapes.push_back(pose_model(cimg, faces[i]));

			if (faces.size() > 0) {
				// Parts of face
				// 1 - right side of jaw
				// 3 - right side of jaw
				// 5 - right side of jaw
				// 19 - middle right eyebrow
				// 21 - inner right eyebrow
				// 22 - inner left eyebrow
				// 24 - middle left eyebrow
				// 33 - nose bottom centre
				// 34 - nose bottom left
				// 35 - nose left
				// 37 - upper right eye 3/4
				// 39 - inner right eye
				// 41 - lower right eye 3/4
				// 45 - outer left eye
				// 49 - upper right lip
				// 51 - middle of upper part of upper lip
				// 53 - upper left lip
				// 57 - lower part of lower lip
				// 59 - lower right lip
				// 62 - lower middle part of upper lip
				// 67 - upper middle right part of lower lip

				char buffer[] = "Default buffer message this is how long? I don't know!";
				sprintf(buffer,
					"eyebrows %d,%d,%d,%d",
					shapes[0].part(24).x(),
					shapes[0].part(24).y(),
					shapes[0].part(19).x(),
					shapes[0].part(19).y()
				);
				zmq::message_t sending(strlen(buffer));
				memcpy(sending.data(), buffer, strlen(buffer));
				push.send(sending);

				zmq::message_t received(13);
				while (sub.recv(&received, ZMQ_DONTWAIT)) {
					std::istringstream iss(static_cast<char*>(received.data()));
					std::string topic;
					std::string message;
					iss >> topic >> message;
					if (message.compare("cmd=quit")) {
						alive = false;
					}
				}
			}
		}
		sub.close();
		push.close();
		context.close();
    } catch(serialization_error& e) {
        std::cout << "You need dlib's default face landmarking model file to run this example." << std::endl;
        std::cout << "You can get it from the following URL: " << std::endl;
        std::cout << "   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" << std::endl;
        std::cout << std::endl << e.what() << std::endl;
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

