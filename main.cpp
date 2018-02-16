#include <Windows.h>
#include <fstream>
#include <direct.h>
#include "Dehaze.h"

//#include "Debug.h"//DEBUG//

int main(int argc, char** argv) {
	int iteration = 0, i = 0, skip = 0, first_indicator = 0;
	double parameters[PARAMETERS_NUM] = { 0 };
	double *parameters_pionter = &parameters[0];
	string directory = argv[1];
	ifstream parameters_file(argv[2]);
	string output_directory = argv[3];
	HANDLE hFind;
	WIN32_FIND_DATA data;
	string img = "";
	string img_path = directory;
	string file_name_to_create;
	Mat Image, Transmission, Radiance, Airlight, DarkChannel;
	directory += "\\*.*";
	hFind = FindFirstFile(directory.c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("hFind == INVALID_HANDLE_VALUE\n");
		return 0;
	}
	do {
		if (skip < 2) {
			skip++;
			continue;
		}
		if (find_format(data.cFileName) == 1) {
			continue;
		}
		output_directory.append("\\");
		while (data.cFileName[i] != '.') {
			output_directory += data.cFileName[i];
			i++;
		}
		i = 0;
		_mkdir(output_directory.c_str());
		output_directory.append("\\");
		img.append(img_path);
		img.append("\\");
		img.append(data.cFileName);
		while (Parameters_list_array(&parameters_file, &parameters_pionter)) {
			Image = imread(img, 1);
			printf("Processing image %s, iteration %d.\n", data.cFileName, iteration);
			Dehaze(Image, data.cFileName, parameters, &Transmission, &Radiance, &Airlight);
			printf("Finished Processing image %s, iteration %d.\n", data.cFileName, iteration);
			if (first_indicator == 0) {
				file_name_to_create = generate_name(output_directory, data.cFileName, INPUTIMAGE, iteration);
				imwrite(file_name_to_create, Image);
				first_indicator = 1;
			}
			file_name_to_create = generate_name(output_directory, data.cFileName, RADIANCE, iteration);
			imwrite(file_name_to_create, Radiance);
			file_name_to_create = generate_name(output_directory, data.cFileName, AIRLIGHT, iteration);
			imwrite(file_name_to_create, Airlight);
			file_name_to_create = generate_name(output_directory, data.cFileName, TRANSMISSION, iteration);
			imwrite(file_name_to_create, Transmission);
			iteration++;
		}
		first_indicator = 0;
		parameters_file.clear();
		parameters_file.seekg(0);
		img = "";
		iteration = 0;
		output_directory = argv[3];
	} while (FindNextFile(hFind, &data));
	i = FindClose(hFind);
	parameters_file.close();
	return 0;
}