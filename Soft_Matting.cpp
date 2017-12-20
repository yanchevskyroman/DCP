#include <opencv2/opencv.hpp>
#include <Eigen/SparseCholesky>
#include <opencv2/core/eigen.hpp>
#include "Consts.h"
using namespace std;
using namespace cv;
using namespace Eigen;


Mat Get_Pixel_Indices_Mat(int rows, int cols) {//Fill the Pixels mat as indices of image pixels - by rows
	Mat Pixel_idx(rows, cols, DataType<int>::type, Scalar(0));
	int i, j, k = 0;
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			Pixel_idx.at<int>(i,j) = k;
			k++;
		}
	}
	return Pixel_idx;
}


void Calc_Mean(Mat values, Mat * mean) {
	double result = 0;
	int i, j;
	for (i = 0; i < CHANNELS_NUM; i++) {
		for (j = 0; j < values.rows; j++) {
			result += values.at<double>(j, i);
		}
		(*mean).at<double>(0,i) = result/(double)values.rows;
		result = 0;
	}
	return;
}


Rect Get_Frame(int x, int y, int frame_size, int rows, int cols) {
	int width = frame_size, height = frame_size;
	int m = x - frame_size/2, n = y - frame_size/2;
	if (x > frame_size / 2 && x < cols - frame_size / 2 && y > frame_size / 2 && y < rows - frame_size / 2) {
		Rect R(m, n, width, height);
		return R;
	}
	else {
		if (m < 0) {
			width = frame_size + m;
			m = 0;
		}
		else if (cols - m - frame_size < 0) {
			width = cols - m;
		}
		if (n < 0) {
			height = frame_size + n;
			n = 0;
		}
		else if (rows - n - frame_size < 0) {
			height = rows - n;
		}
		Rect R(m, n, width, height);
		return R;
	}
}


Mat Laplacian_In_Patch(Mat Values, Mat Mean, Mat Covar, double regularization_parameter) {
	int i, j;
	Mat Image_Part_Mat_T(CHANNELS_NUM, Values.cols, DataType<double>::type);
	Mat Image_Part_Mat(Values.rows, CHANNELS_NUM, DataType<double>::type);
	Mat Covar_Part_Mat(CHANNELS_NUM, CHANNELS_NUM, DataType<double>::type);
	Mat Identity(CHANNELS_NUM, CHANNELS_NUM, DataType<double>::type);
	Mat Result;
	setIdentity(Identity, Scalar(1));
	Covar_Part_Mat = (regularization_parameter / (double)Values.rows)*Identity;
	Covar_Part_Mat = Covar + Covar_Part_Mat;
	Covar_Part_Mat = Covar_Part_Mat.inv();
	for (i = 0; i < CHANNELS_NUM; i++) {
		for (j = 0; j < Values.rows; j++) {
			Image_Part_Mat.at<double>(j, i) = Values.at<double>(j, i) - Mean.at<double>(0, i);
		}
	}
	transpose(Image_Part_Mat, Image_Part_Mat_T);
	Result = Image_Part_Mat*Covar_Part_Mat*Image_Part_Mat_T;
	for (i = 0; i < Result.rows; i++) {
		for (j = 0; j < Result.cols; j++) {
			Result.at<double>(i, j) = -Result.at<double>(i, j) - 1;
		}
	}
	Result = Result / Values.rows;
	return Result;
}


Mat Reshape_To_Channels_And_Normalize(Mat source){
	int i, j, k , Patch_reshaped_iterator = 0;
	Mat Patch_reshaped(source.rows*source.cols, CHANNELS_NUM, DataType<double>::type);
	for (j = 0; j < source.rows; j++) {
		for (i = 0; i < source.cols; i++) {
			for (k = 0; k < CHANNELS_NUM; k++) {
				Patch_reshaped.at<double>(Patch_reshaped_iterator, k) = (double)source.at<Vec3b>(j, i)[CHANNELS_NUM - k - 1] / (double)255;
			}
			Patch_reshaped_iterator++;
		}
	}
	return Patch_reshaped;
}


void Calc_Covar(Mat patch, Mat patch_idx, int patch_size, Mat Mean, Mat * Covar) {
	Mat Mean_Mul_Mean_T(CHANNELS_NUM, CHANNELS_NUM, DataType<double>::type);
	Mat Mean_T(CHANNELS_NUM, 1, DataType<double>::type);
	Mat Patch_T(CHANNELS_NUM, patch_size, DataType<double>::type);
	transpose(Mean, Mean_T);
	Mean_Mul_Mean_T = Mean_T*(Mean);
	transpose(patch, Patch_T);
	*Covar = Patch_T*patch / patch_size;
	*Covar = *Covar - Mean_Mul_Mean_T;
	return;
}


Mat Normalize_Mat(Mat Transmission) {
	int i, j;
	Mat normalized(Transmission.rows, Transmission.cols, DataType<double>::type);
	for (i = 0; i < Transmission.rows; i++) {
		for (j = 0; j < Transmission.cols; j++) {
			normalized.at<double>(i, j) = (double)Transmission.at<uchar>(i, j) / 255;
		}
	}
	return normalized;
}


void Assign_Lap_Values_To_Correct_Indices(Mat Values, Mat indices, vector<Triplet<double>> * tripletList) {
	int i, j, lap_index_x, lap_index_y;
	double res_value = 0;
	for (i = 0; i < Values.rows; i++) {
		for (j = 0; j < Values.cols; j++) {
			lap_index_x = indices.at<int>(0, i);
			lap_index_y = indices.at<int>(0, j); 
			if (lap_index_y > lap_index_x) {
				continue;
			}
			res_value = Values.at<double>(i, j);
			if (i == j) {
				res_value += 1;
			}
			(*tripletList).push_back(Triplet<double>(lap_index_x, lap_index_y, res_value));
		}
	}
	return;
}


SparseMatrix<double> GetLaplacian(Mat image, int frame_size, double regularization_parameter) { //frame_size is supposably 3
	int rows = image.rows;
	int cols = image.cols;
	int size = rows*cols;
	int i, area, j = 0;
	int num_of_frames_for_single_pixel = frame_size*frame_size;
	int entries_per_column_estimation = frame_size * 2 - 1;
	int estimation_of_entries = size*entries_per_column_estimation;
	Mat mean(1, CHANNELS_NUM, DataType<double>::type, Scalar(0));
	Mat covar(CHANNELS_NUM, CHANNELS_NUM, DataType<double>::type, Scalar(0));
	Mat Patch, Patch_idx, Patch_Reshaped, Lap_In_Patch;
	Mat Pixel_idx = Get_Pixel_Indices_Mat(rows, cols);
	Mat Mean(1, CHANNELS_NUM, DataType<double>::type, Scalar(0));
	Mat Covar(CHANNELS_NUM, CHANNELS_NUM, DataType<double>::type, Scalar(0));
	vector<Triplet<double>> tripletList;
	SparseMatrix<double> result_LowerTriangle(size, size);
	tripletList.reserve(estimation_of_entries);
	result_LowerTriangle.reserve(VectorXi::Constant(size, entries_per_column_estimation));//CORRECT RESERVE FOR LOWER TRIANGLEVIEW
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			Rect R = Get_Frame(j, i, frame_size, rows, cols);
			area = R.area();
			Patch = image(R);
			Patch_Reshaped = Reshape_To_Channels_And_Normalize(Patch);
			Calc_Mean(Patch_Reshaped, &Mean);
			Calc_Covar(Patch_Reshaped, Patch_idx, area, Mean, &Covar);
			Lap_In_Patch = Laplacian_In_Patch(Patch_Reshaped, Mean, Covar, regularization_parameter);
			Patch_idx = Pixel_idx(R);
			if (!Patch_idx.isContinuous())
			{
				Patch_idx = Patch_idx.clone();
			}
			Patch_idx = Patch_idx.reshape(0, 1);
			Assign_Lap_Values_To_Correct_Indices(Lap_In_Patch, Patch_idx, &tripletList);
		}
	}
	result_LowerTriangle.setFromTriplets(tripletList.begin(), tripletList.end());
	SparseMatrix<double> result = result_LowerTriangle.selfadjointView<Lower>();
	return result;
}


Mat SoftMatting(Mat Transmission, Mat image, int frame_size, double regularization_parameter, double lambda) {
	int i, j;
	int Laplacian_Size = image.rows*image.cols;
	Mat t;
	Mat normalized_Transmission = Normalize_Mat(Transmission);
	Mat b = normalized_Transmission.reshape(0, Laplacian_Size);
	Mat Transmission_new(Transmission.rows, Transmission.cols, DataType<uchar>::type);
	Matrix<double, Dynamic, Dynamic, ColMajor> b_Eigen(b.rows, b.cols);
	Map<VectorXd> b_Vector(b_Eigen.data(), b_Eigen.size());
	SparseMatrix<double> Laplacian = GetLaplacian(image, frame_size, regularization_parameter);
	SparseMatrix<double> Identity_Mat(Laplacian_Size, Laplacian_Size);
	SparseMatrix<double> A(Laplacian_Size, Laplacian_Size);
	SimplicialLLT<SparseMatrix<double>> solver;
	VectorXd x;
	b = lambda*b;
	cv2eigen(b, b_Eigen);
	Identity_Mat.setIdentity();
	A = lambda*Identity_Mat;
	A += Laplacian;
	x = solver.compute(A).solve(b_Vector);//Solve Ax = b for x
	b_Eigen.col(0) = x;
	eigen2cv(b_Eigen, t);
	t = t.reshape(0, Transmission.rows);
	for (i = 0; i < Transmission.rows; i++) {
		for (j = 0; j < Transmission.cols; j++) {
			Transmission_new.at<uchar>(i, j) = (uchar)min(255, (int)(t.at<double>(i, j) * 255));
		}
	}
	return Transmission_new;
}