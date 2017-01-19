// test_lib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "face_detection_functions.h"
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/image_io.h>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
	int mycount(0);
	dlib::frontal_face_detector detector;
	dlib::shape_predictor sp;
	dlib::array2d<dlib::rgb_pixel> img;
	dlib::image_window win;

	void init(void) {
		using namespace dlib;
		detector = get_frontal_face_detector();
		deserialize("C:/Programming/dlib-19.2/examples/shape_predictor_68_face_landmarks.dat") >> sp;
	}

	int detect(void) {
		std::vector<dlib::rectangle> dets;
		try {
			load_image(img, "C:/Users/Maxim/Documents/GitHub/writingtechforjarrod/vision/img/test_group.jpg");
			win.set_image(img);
			dets = detector(img);
		}
		catch (const std::exception& e) {
			std::ofstream err_file("err.txt");
			err_file << e.what();
		}
		return dets.size();
	}

	int test_inc(void) {
		mycount++;
		return mycount;
	}

	void load_rgb_img(const int m, const int n, const int rgb, const double ***x, double ***y) {
		size_t i, j, k;
		for (i = 0; i<m; ++i)
			for (j = 0; j<n; ++j)
				for (k = 0; k<rgb; ++k)
					y[i][j][k] = 2*x[i][j][k];
	}

	void foobar(const int m, const int n, const double **x, double **y) {
		size_t i, j;
		for (i = 0; i<m; i++)
			for (j = 0; j<n; j++)
				y[i][j] = x[i][j];
	}

	double add(double a, double b) {
		return a + b;
	}
#ifdef __cplusplus
}
#endif

