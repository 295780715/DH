#include "dehaze.h"

int main(int argc, char **argv) {
	while (1) {
		// ȥ����
		int method;
		cout << "1.dcp-he" << endl;
		cout << "2.dcp-test1" << endl;
		cout << "3.dcp-test2" << endl;
		cout << "input dehaze method:" << endl;
		cin >> method;

		// ����ͼƬ
		cout << "input haze image name:" << endl;
		char img_name[50] = { 0 };
		char img_path[128] = "D:\\dehazeImages\\";
		cin >> img_name;
		strcat_s(img_path, img_name);

		Mat src;
		readImage(src, img_path);

		switch (method)
		{
		case 1:
			// ��ͨ��ȥ��
			dehaze_dcp(src);
			break;
		case 2:
			// ��ͨ�����Է���
			dehaze_dcp_test1(src);
			break;
		case 3:
			dehaze_dcp_test2(src);
			break;
		default:
			cout << "method input error.." << endl;
			break;
		}
		cout << "-------------end--------------" << endl;
	}
}


