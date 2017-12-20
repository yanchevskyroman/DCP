#pragma once
#ifndef SOFT_MATTING_H
#define SOFT_MATTING_H
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;
Mat SoftMatting(Mat Transmission, Mat image, int frame_size, double regularization_parameter, double lambda);
#endif // SOFT_MATTING_H