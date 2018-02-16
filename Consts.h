#pragma once
#ifndef CONSTS_H
#define CONSTS_H

#define CHANNELS_NUM 3
#define PARAMETERS_NUM 8

/*argv holds: image name, frame size, percentage of pixels, number of pixels, omega factor,
frame size for laplacian (supposably 3), regularization parameter (epsilon).*/
enum param {
	WINDOW_SIZE,
	PERCENT,
	OMEGA,
	LAPLACE_WINDOW_SIZE,
	EPSILON,
	LAMBDA,
	T0,
};

enum image_data_type {
	INPUTIMAGE,
	RADIANCE,
	AIRLIGHT,
	TRANSMISSION,
	DARK,
};



#endif // CONSTS_H