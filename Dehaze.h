#pragma once
#ifndef DEHAZE_H
#define DEHAZE_H

#include "Consts.h"
#include <iostream>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

int find_format(string file);
bool Parameters_list_array(ifstream * infile, double **parameters);
void Dehaze(Mat image, char * image_name, double * parameters, Mat *Transmission, Mat *Radiance, Mat *Airlight);
string generate_name(string output, char * image_name, int image_data_type, int iteration);

#endif //DEHAZE_H
