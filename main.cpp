#include "dehaze.h"

int main(int argc, char **argv) {
	cout << "input haze image name:" << endl;

	char img_name[50] = { 0 };
	char img_path[128] = "D:\\dehazeImages\\";
	cin >> img_name;
	strcat_s(img_path, img_name);

	Mat src;
	readImage(src, img_path);

	// °µÍ¨µÀÈ¥Îí
	dehaze_dcp(src);
}


