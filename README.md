# DCP
C++ implementation of the Dark Channel Prior algorithm for single image haze removal.
Input: folder containing input images path (string), *.txt file path containing input parameters for dehazing (string), output folder path.
Dehazing parameters txt file contains: patch size, percentage of pixel for airlight estimation, omega regularization parameter, 
  laplacian window size, epsilon (laplacian matrix parameter, lambda smooth factor (soft matting), transmission lower threshold (double).
  suggested values:
    15 10 0.95 3 0.0000001 0.0001 0.1
Output: original image, dehazed image, transmission map, airlight (same size as the input image) saved in a seperate directory
  for each input image.
