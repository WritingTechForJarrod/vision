#pragma once

#ifdef TEST_LIB_EXPORTS  
#define TEST_LIB_API __declspec(dllexport)   
#else  
#define TEST_LIB_API __declspec(dllimport)   
#endif

#ifdef __cplusplus
extern "C" {
#endif
		TEST_LIB_API void init();
		TEST_LIB_API int test_inc();
		TEST_LIB_API int detect();
		TEST_LIB_API void load_rgb_img(const int m, const int n, const int rgb, const double ***x, double ***y);
		TEST_LIB_API void foobar(const int m, const int n, const double **x, double **y);
		TEST_LIB_API double add(double a, double b);
#ifdef __cplusplus
}
#endif
