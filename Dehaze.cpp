#include "Soft_Matting.h"
#include "Consts.h"
#include <fstream>
//#include "Debug.h"//DEBUG//


Mat Dark_Channel_Extractor(Mat image, int frame_size) {
	Mat Channels[3];
	Mat Patch[3];
	Mat Padded_Img;
	double Minimal_pixel_per_channel[3];
	if (frame_size % 2 == 0) {//Make frame odd-sized for symmetry purposes
		frame_size--;
	}
	int center_of_frame = ((frame_size + 1) / 2) - 1;
	Mat dark_channel;
	int i, j;

	//Pad the image
	copyMakeBorder(image, Padded_Img, center_of_frame, center_of_frame, center_of_frame, center_of_frame, BORDER_CONSTANT, Scalar(255, 255, 255));
	Mat Padded_dark_channel(Padded_Img.size(), CV_8UC1, Scalar(255));
	split(Padded_Img, Channels);

	//Find minimal pixel among RGB channels
	for (j = center_of_frame; j < Padded_Img.rows - center_of_frame; j++) {
		for (i = center_of_frame; i < Padded_Img.cols - center_of_frame; i++) {
			Rect R(i - center_of_frame, j - center_of_frame, frame_size, frame_size);
			for (int k = 0; k < 3; k++) {
				Patch[k] = Channels[k](R);
				minMaxLoc(Patch[k], &Minimal_pixel_per_channel[k], NULL, NULL, NULL);
			}
			Padded_dark_channel.at<uchar>(j, i) = (uchar)min(min(Minimal_pixel_per_channel[0], Minimal_pixel_per_channel[1]), Minimal_pixel_per_channel[2]);
		}
	}

	//Remove padding
	Rect padding_remover_frame(center_of_frame, center_of_frame, image.cols, image.rows);
	dark_channel = Padded_dark_channel(padding_remover_frame);

	return dark_channel;
}


void Airlight_Estimator(Mat image, Mat DarkChannel, int percentage, uchar(*Airlight)[CHANNELS_NUM]) {
	int image_size = image.cols * image.rows;
	int num_of_pixels = (int)floor((percentage / (double)100) * image_size);
	int i = 0, j = 0;
	float RGB[CHANNELS_NUM] = { 0 };
	Point * max_location = (Point *)malloc(sizeof(Point)* image_size);
	for (i; i < num_of_pixels; i++) {
		minMaxLoc(DarkChannel, NULL, NULL, NULL, &max_location[i]);
		DarkChannel.at<uchar>(max_location[i]) = (uchar)0;
	}
	for (i = 0; i < num_of_pixels; i++) {
		for (j = 0; j < CHANNELS_NUM; j++) {
			RGB[j] += (int)image.at<Vec3b>(max_location[i])[j];
		}
	}
	for (j = 0; j < CHANNELS_NUM; j++) {
		RGB[j] = ceil(RGB[j] / num_of_pixels);
		(*Airlight)[j] = (uchar)RGB[j];
	}
	free(max_location);
	return;
}


Mat Dark_Channel_Normalizer(Mat Image, uchar * Airlight, int frame_size) {
	Mat Image_channels[CHANNELS_NUM];
	Mat temp_channels[CHANNELS_NUM];
	Mat Normalize_image;
	Mat dark_channel_normalized;
	Mat temp(Image.size(), CV_8UC3, Scalar(0, 0, 0));
	vector<Mat> channels_vector;
	int i, j, k;
	split(Image, Image_channels);
	split(temp, temp_channels);
	for (k = 0; k < 3; k++) {
		for (i = 0; i < Image_channels[0].rows; i++) {
			for (j = 0; j < Image_channels[0].cols; j++) {
				temp_channels[k].at<uchar>(i, j) = min(255, (int)((float)255 * ((float)Image_channels[k].at<uchar>(i, j)) / (float)Airlight[k]));
			}
		}
	}
	for (i = 0; i < CHANNELS_NUM; i++) {
		channels_vector.push_back(temp_channels[i]);
	}
	merge(channels_vector, Normalize_image);
	dark_channel_normalized = Dark_Channel_Extractor(Normalize_image, frame_size);
	return dark_channel_normalized;
}


Mat Transmision_Extractor(Mat Image, uchar * Airlight, int frame_size, double omega) {
	Mat dark_channel_normalized = Dark_Channel_Normalizer(Image, Airlight, frame_size);
	Mat ones(dark_channel_normalized.size(), CV_8UC1, Scalar(255));
	return ones - omega*dark_channel_normalized;
}


string generate_name(string output, char * image_name, int image_data_type, int iteration) {
	int i = 0;
	string iteration_str = "";
	string filename;
	string output_dir = output;
	iteration_str.append("_");
	iteration_str.append(static_cast<ostringstream*>(&(ostringstream() << iteration))->str());
	iteration_str.append("_Iteration");
	while (image_name[i] != '.') {
		filename += image_name[i];
		i++;
	}
	switch (image_data_type) {
	case INPUTIMAGE:
		filename.append("_Input");
		while (image_name[i] != '\0') {
			filename += image_name[i];
			i++;
		}
		output_dir.append(filename);
		return output_dir;
	case RADIANCE:
		filename.append("_Radiance");
		break;
	case AIRLIGHT:
		filename.append("_Airlight");
		break;
	case TRANSMISSION:
		filename.append("_Transmission");
		break;
	}
	filename.append(iteration_str);
	while (image_name[i] != '\0') {
		filename += image_name[i];
		i++;
	}
	output_dir.append(filename);
	return output_dir;
}


bool Parameters_list_array(ifstream * infile, double **parameters) {
	string param_line = "\n";
	char * single_parameter;
	char *temp_token = NULL;
	int i = 0;
	while (param_line == "\n" || param_line == "") {
		getline(*infile, param_line);
		if ((*infile).eof()) {
			return false;
		}
	}
	single_parameter = strtok_s(&param_line[0], " ", &temp_token);
	while (single_parameter != NULL)
	{
		(*parameters)[i] = stof(single_parameter);
		i++;
		single_parameter = strtok_s(NULL, " ", &temp_token);
	}
	return true;
}


Mat Get_Radiance(Mat Image, uchar * Airlight, Mat Transmission, double t0) {
	int i, j, k;
	double temp1, temp2;
	Mat Image_channels[CHANNELS_NUM];
	Mat Result(Image.size(), CV_8UC3, Scalar(0, 0, 0));
	Mat Result_channels[CHANNELS_NUM];
	vector<Mat> channels_vector;
	split(Image, Image_channels);
	split(Result, Result_channels);
	for (k = 0; k < CHANNELS_NUM; k++) {
		for (i = 0; i < Image.rows; i++) {
			for (j = 0; j < Image.cols; j++) {
				temp1 = ((double)Image_channels[k].at<uchar>(i, j) - (double)Airlight[k]) / (double)255;
				temp2 = max(t0, (double)Transmission.at<uchar>(i, j) / 255);
				temp1 = temp1 / temp2;
				Result_channels[k].at<uchar>(i, j) = min(255, max(0, (int)(temp1 * 255 + Airlight[k])));
			}
		}
	}

	for (i = 0; i < CHANNELS_NUM; i++) {
		channels_vector.push_back(Result_channels[i]);
	}
	merge(channels_vector, Result);
	return Result;
}


/*Turns airlight vector into an airlight image*/
Mat Form_Airlight_Mat(uchar *Airlight_Array, int width, int height) {
	int i, j, k;
	Mat Airlight_Mat(width, height, CV_8UC3, Scalar(0, 0, 0));
	Mat Airlight_channels[CHANNELS_NUM];
	vector<Mat> channels_vector;
	split(Airlight_Mat, Airlight_channels);
	for (k = 0; k < CHANNELS_NUM; k++) {
		for (i = 0; i < width; i++) {
			for (j = 0; j < height; j++) {
				Airlight_channels[k].at<uchar>(i, j) = Airlight_Array[k];
			}
		}
	}
	for (i = 0; i < CHANNELS_NUM; i++) {
		channels_vector.push_back(Airlight_channels[i]);
	}
	merge(channels_vector, Airlight_Mat);
	return Airlight_Mat;
}


void Dehaze(Mat Image, char * image_name, double * parameters, Mat *Transmission, Mat *Radiance, Mat *Airlight) {
	Mat dark_channel = Dark_Channel_Extractor(Image, (int)parameters[WINDOW_SIZE]);
	uchar Airlight_RGB[CHANNELS_NUM] = { 0,0,0 };
	Airlight_Estimator(Image, dark_channel, (int)parameters[PERCENT]/*, (int)parameters[PIXEL_NUM]*/, &Airlight_RGB);
	*Transmission = Transmision_Extractor(Image, Airlight_RGB, (int)parameters[WINDOW_SIZE], parameters[OMEGA]);
	*Transmission = SoftMatting(*Transmission, Image, (int)parameters[LAPLACE_WINDOW_SIZE], parameters[EPSILON], parameters[LAMBDA]);
	*Radiance = Get_Radiance(Image, Airlight_RGB, *Transmission, parameters[T0]);
	*Airlight = Form_Airlight_Mat(Airlight_RGB, Image.rows, Image.cols);
	return;
}

int find_format(string file) {
	int i = 0;
	string format;
	if (file.find('.') == std::string::npos) {
		return 1;
	}
	while (file[i] != '.') {
		if (i == file.length()) {
			return 1;
		}
		i++;
	}
	i++;
	while (i < file.length()) {
		format += file[i];
		i++;
	}
	if (format == "txt" || format == "exe") {
		return 1;
	}
	return 0;
}