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
#define DISTRIBUTED
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
using namespace std;

class FaceMetrics {
public:
	static double brow_threshold;
	static double mouth_threshold;
	static long long max_delta_ms;
	point eyebrow_left;
	point eyebrow_right;
	point nose_center;
	point lip_upper;
	point lip_lower;
	double between_eyebrows;
	double left_eyebrow_to_nose;
	double right_eyebrow_to_nose;
	double mouth_height;
	double mouth_metric;
	double eyebrow_metric;
	std::chrono::time_point<std::chrono::steady_clock> time_constructed;
	bool initialized;

	FaceMetrics() : initialized(false) {
		// Default void constructor	
	}

	FaceMetrics(point eye_l, point eye_r, point nose, point lip_u, point lip_l) 
		: eyebrow_left(eye_l), eyebrow_right(eye_r), nose_center(nose), lip_upper(lip_u), lip_lower(lip_l)
	{
		between_eyebrows = (eyebrow_left - eyebrow_right).length();
		left_eyebrow_to_nose = (nose_center - eyebrow_left).length();
		right_eyebrow_to_nose = (nose_center - eyebrow_right).length();
		mouth_height = (lip_upper - lip_lower).length();
		time_constructed = std::chrono::high_resolution_clock::now();
		initialized = true;
	}

	double total_difference(FaceMetrics &other) const {
		double d_eye_l((other.eyebrow_left - eyebrow_left).length());
		double d_eye_r((other.eyebrow_right - eyebrow_right).length());
		double d_nose((other.nose_center - nose_center).length());
		return d_eye_l + d_eye_r + d_nose;
	}
	
	double triangle_perimeter() const {
		return between_eyebrows + left_eyebrow_to_nose + right_eyebrow_to_nose;
	}

	int check_trigger(FaceMetrics & other) {
		auto delta(time_constructed - other.time_constructed);
		auto delta_ms(std::chrono::duration_cast<std::chrono::milliseconds>(delta).count());
		double d_left(other.left_eyebrow_to_nose - left_eyebrow_to_nose);
		double d_right(other.left_eyebrow_to_nose - left_eyebrow_to_nose);
		double d_nose((other.nose_center - nose_center).length());
		double d_mouth_height(other.mouth_height - mouth_height);
		const double p(triangle_perimeter());
		d_left /= p;
		d_right /= p;
		d_nose /= p;
		d_mouth_height /= p;
		
		delta_ms = delta_ms > max_delta_ms ? max_delta_ms : delta_ms;
		const double eyebrow_scale(15000);
		const double mouth_scale(5000);
		mouth_metric = d_mouth_height * mouth_scale / delta_ms;
		eyebrow_metric = ((d_left + d_right) / 2 - d_nose) * eyebrow_scale / delta_ms;

		//cout << eyebrow_metric << ", " << mouth_metric << endl;

		int ret(0);
		if (eyebrow_metric > brow_threshold)	ret |= 0x1;
		if (mouth_metric > mouth_threshold)		ret |= 0x2;
		return ret;
	}
};

double FaceMetrics::brow_threshold(1.0);
double FaceMetrics::mouth_threshold(1.0);
long long FaceMetrics::max_delta_ms(300);
int num_loops(500);

template <typename T>
bool from_named_value(const std::string name, const std::string desired_name, const std::string value, T &returned) {
	if (name == desired_name) {
		std::istringstream double_sstream(value);
		if (!(double_sstream >> returned)) {
			cerr << name << " value " << value << " not readable" << endl;
		} else {
			return true;
		}
	}
	return false;
}

bool load_settings(bool &cv_window_ref, double &brow_threshold_ref) {
	std::ifstream settings;
	settings.open("settings.txt");
	if (!settings.is_open()) return false; // File open error
	std::string line;
	while (std::getline(settings, line)) {
		std::istringstream in_line(line);
		std::string var_name;
		std::string equals;
		std::string value;
		if (!(in_line >> var_name >> equals >> value)) {
			cerr << "Settings value corrupted" << endl;
			break;
		}
		if (var_name == std::string("cv_window")) {
			cv_window_ref = (value == std::string("True"));
		}
		else if (from_named_value(var_name, "brow_threshold", value, FaceMetrics::brow_threshold)) {}
		else if (from_named_value(var_name, "mouth_threshold", value, FaceMetrics::mouth_threshold)) {}
		else if (from_named_value(var_name, "max_delta_ms", value, FaceMetrics::max_delta_ms)) {}
		else if (from_named_value(var_name, "num_loops", value, num_loops)) {}
	}
	return true;
}

int main()
{
    try
    {
#ifdef DISTRIBUTED
		zmq::context_t context(1);
		zmq::socket_t socket(context, ZMQ_REQ);
		std::cout << "Connecting to hello world server…" << std::endl;
		socket.connect("tcp://localhost:5555");
		int messages_sent(0);
#endif
        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            cerr << "Unable to connect to camera" << endl;
            return 1;
        }

		bool cv_window;

		if (!load_settings(cv_window, FaceMetrics::brow_threshold)) {
			cerr << "settings.txt file not found" << endl;
		}
		
		image_window *p_win(nullptr);
		if (cv_window) {
			p_win = new image_window;
		}

        // Load face detection and pose estimation models.
        frontal_face_detector detector = get_frontal_face_detector();
        shape_predictor pose_model;
        deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;

		FaceMetrics last_metrics;

		std::ofstream trigger_out;
		trigger_out.open("trigger.txt", std::ios::out);
		if (!trigger_out.is_open()) {
			cerr << "Unable to open trigger.txt for writing";
			return -1;
		}
		trigger_out.seekp(0, std::ios::beg);

        // Grab and process frames until the main window is closed by the user.
		bool alive(true);
		while (alive)
		{
			if (cv_window)
				alive = !p_win->is_closed();
			else
				alive = true;
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

			if (cv_window) {
				p_win->clear_overlay();
				p_win->set_image(cimg);
				p_win->add_overlay(render_face_detections(shapes));
			}

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

				rgb_pixel red(255, 0, 0);
				rgb_pixel blue(0, 0, 255);
				rgb_pixel white(255, 255, 255);
				rgb_pixel purple(255, 0, 255);

				auto point1 = shapes[0].part(51);
				auto point2 = shapes[0].part(57);

				FaceMetrics this_metrics(
					shapes[0].part(24), 
					shapes[0].part(19), 
					shapes[0].part(33),
					shapes[0].part(51),
					shapes[0].part(57)
				);

				//auto delta(this_metrics.time_constructed - last_metrics.time_constructed);
				//cout << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() << endl;

				auto circle1(image_window::overlay_circle(point1, 5, red));
				auto circle2(image_window::overlay_circle(point2, 5, blue));

				int triggered(0);
				if (last_metrics.initialized) {
					triggered = this_metrics.check_trigger(last_metrics);
					trigger_out << triggered << "," 
						<< this_metrics.eyebrow_metric << ","
						<< this_metrics.mouth_metric << endl;
					cout << triggered << endl;
				}

#ifdef DISTRIBUTED
				char buffer[] = "default";
				if (messages_sent > num_loops) {
					alive = false;
				} else {
					zmq::message_t request(7);
					if (messages_sent == num_loops) {
						memcpy(request.data(), "quit", 5);
						socket.send(request);
					} else {
						sprintf(buffer, "#face:%d", triggered);
						memcpy(request.data(), buffer, 7);
						socket.send(request);
						zmq::message_t reply;
						socket.recv(&reply);
						std::cout << "Received World " << messages_sent << std::endl;
					}
					messages_sent++;
				}
#endif
				last_metrics = this_metrics;

				if (cv_window) {
					p_win->add_overlay(circle1);
					p_win->add_overlay(circle2);
				}
			}
		}
    }
    catch(serialization_error& e)
    {
        cout << "You need dlib's default face landmarking model file to run this example." << endl;
        cout << "You can get it from the following URL: " << endl;
        cout << "   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" << endl;
        cout << endl << e.what() << endl;
    }
    catch(exception& e)
    {
        cout << e.what() << endl;
    }
}

