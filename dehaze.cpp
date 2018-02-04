#include "dehaze.h"

/*
�����ļ�·������ȡ��ͨ��ͼ
*/
void readImage(Mat &output, char *img_name)
{
	Mat img = imread(img_name, CV_LOAD_IMAGE_COLOR);
	if (img.empty()) {
		cout << "image load error!" << endl;
		exit(-1);
	}
	img.convertTo(output, CV_8UC3);

	img.release();
}

/*
��ͨ��ȥ��
*/
int dehaze_dcp(Mat & src) {
	// ��ʼ����Ϊ3�������Զ����ڣ�ͨ��opencv��trackbar
	int window = 3;
	int max_window = 51;

	// 1.read image,U8C3����

	// 2.��ȡ�Ҷ�ͼ
	Mat gray = Mat::zeros(src.size(), CV_8UC1);
	int time_gray = getGrayByMin(src, gray);

	// 3.��ȡ��ͨ��ͼ
	Mat dc = Mat::zeros(src.size(), CV_8UC1);
	int time_dc = getDarkChannel(gray, dc, window);

	// 4.��ȡ������ֵA
	Pixel airLightPixel;
	int time_al = getAirLightDcp(dc, src, AIRLIGHT_RATIO, airLightPixel);

	// 5.��ȡ͸��ͼT������͸����tΪ����������������Ϊ32fc1,[0,1]֮��ֵ
	Mat trans = Mat::zeros(src.size(), CV_32FC1);
	int time_t = getTransDcp(trans, dc, airLightPixel);

	// 6.��ȡ��ϸ��Ͷ��ͼT,���ַ�ʽ��δʵ��
	// softmatting()
	// guidefilter()

	// 7.��ȡȥ��ͼ
	Mat hazefree = Mat::zeros(src.size(),CV_8UC3);
	int time_dehaze = dehaze(hazefree, src, trans, airLightPixel, NULL);

	// 8.��ʾ
	imshow("ԭͼ", src);
	imshow("�Ҷ�ͼ", gray);
	imshow("͸��ͼ",trans);
	imshow("��ͨ��ͼ", dc);
	imshow("ȥ��ͼ", hazefree);

	// ���ڰ�ͨ������
	UserData ud;
	ud.data1 = &gray;
	ud.data2 = &dc;
	createTrackbar("window:", "��ͨ��ͼ", &window, max_window, CallBack_Dcp_by_Window, (void*)&ud);

	int time_total = time_gray + time_dc + time_al + time_t + time_dehaze;
#ifdef DEBUG
	// debug��Ϣ���
	cout << "A:" << airLightPixel.b << "," << airLightPixel.g << "," << airLightPixel.r << endl;
	cout << "get gray time:" << time_gray << endl;
	cout << "get darkchannel time:" << time_dc << endl;
	cout << "get air light time:" << time_al << endl;
	cout << "get T map time:" << time_t << endl;
	cout << "restore time:" << time_dehaze << endl;
	cout << "time total:" << time_total << endl;
#endif

	waitKey();

	return time_total;
}

/*
��ȡ��ͨ���Ķ�̬���ڵ��ڻص�����
*/
void CallBack_Dcp_by_Window(int window, void* userdata) {
	Mat * gray = ((UserData*)userdata)->data1;
	Mat * dc = ((UserData*)userdata)->data2;
	getDarkChannel(*gray, *dc, window);
	imshow("��ͨ��ͼ", *dc);
}

/*�з��� ��ȡ������Сֵ*/
int horizonMinfilter(Mat &dst, const Mat &src, int radius)
{
	unsigned char minVal;
	int result = 0;
	int tmp = 0;

	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			minVal = 255;
			if (j >= radius && j < src.cols - radius)
			{
				for (int k = -radius; k <= radius; k++)
				{
					tmp = src.at<uchar>(i, j + k);
					minVal = MYMIN(minVal, tmp);
				}
				result = minVal;

			}
			else
				result = src.at<uchar>(i, j);
			dst.at<uchar>(i, j) = result;
		}
	}
	return 0;
}
/*�з��� ��ȡ������Сֵ*/
int verticalMinfilter(Mat &dst, const Mat &src, int radius)
{
	unsigned char minVal;
	int result, tmp;

	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			minVal = 255;
			if (radius <= i && i < src.rows - radius)
			{
				for (int k = -radius; k <= radius; k++)
				{
					tmp = src.at<uchar>(i + k, j);
					minVal = MYMIN(minVal, tmp);
				}
				result = minVal;
			}
			else
				result = src.at<uchar>(i, j);
			dst.at<uchar>(i, j) = result;
		}
	}
	return 0;
}
/*��Сֵ�˲�*/
int myMinfilter(Mat &dst, const Mat &src, int window)
{
	Mat temp = Mat::zeros(src.size(), CV_8UC1);
	int radius = (window - 1) / 2;
	/*�����з���Ϊ������ȡ��Сֵ*/
	horizonMinfilter(temp, src, radius);
	/*Ȼ�����з���Ϊ������ȡ��Сֵ*/
	verticalMinfilter(dst, temp, radius);
	return 0;
}

/*
����ԭ��ɫͼ����ȡ��Сֵ�Ҷ�ͼ
*/
int getGrayByMin(const Mat & src, Mat & gray) {
	long t0 = clock();
	unsigned char r, g, b, tmp;
	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			r = src.at<Vec3b>(i, j)[0];
			g = src.at<Vec3b>(i, j)[1];
			b = src.at<Vec3b>(i, j)[2];
			tmp = MYMIN(MYMIN(r, g), b);
			gray.at<uchar>(i, j) = tmp;
		}
	}
	return clock() - t0;
}

/*
����ԭ��ɫͼ��ͼ��ĻҶ�ͼ����ȡ��ͨ��ͼ
src ԭ��ͼ
dc ��Ӧ�İ�ͨ��ͼ������Ϊ��ͨ��ͼ
window ��ͨ�����ڰ뾶
*/
int getDarkChannel(const Mat & gray, Mat & dc, int window)
{
	long t0 = clock();
	// ���ݻҶ�ͼ��ͨ����Сֵ�˲������˲��뾶�����ɰ�ͨ��ͼ
	myMinfilter(dc, gray, window);
	return clock() - t0;
}

/* qsort�ȽϺ��� */
int cmpByPixel(const void *p1, const void *p2)
{
	return ((Pixel *)p2)->value - ((Pixel *)p1)->value;
}

/* ����ȫ�ִ����� */
int getAirLightDcp(Mat &dark_img, Mat &src, float ratio, Pixel & air_light_pixel)
{
	long t0 = clock();
	// ��������Ŀ
	int dark_size = dark_img.rows * dark_img.cols;
	// ǰratio�����������ص���Ŀ
	int top_size = dark_size * ratio;
	// ���е�����
	Pixel *allpixels = (Pixel *)malloc(sizeof(Pixel)*dark_size);

	// �޶���ȡA��������СΪ500������
	if (top_size < 500)
		top_size = 500;

	// ��ʼ������ֵ
	for (int i = 0; i < dark_img.rows; i++)
	{
		for (int j = 0; j < dark_img.cols; j++)
		{
			allpixels[i*dark_img.cols + j].x = i;
			allpixels[i*dark_img.cols + j].y = j;
			allpixels[i*dark_img.cols + j].value = dark_img.at<uchar>(i, j);
		}
	}

	// �⺯��qsortΪ������������������Ϊcmp
	qsort(allpixels, dark_size, sizeof(Pixel), cmpByPixel);

	// ����allpixels��ǰtop_size�����ص�ԭͼsrc�е���Ӧ�����ص������ĵ���Ϊ������ֵ
	int x, y;
	int r, g, b;
	float bright, max;
	int maxi, maxj;
	max = 0;
	for (int i = 0; i < top_size; i++)
	{
		x = allpixels[i].x; 
		y = allpixels[i].y;
		
		b = src.at<Vec3b>(x, y)[0];
		g = src.at<Vec3b>(x, y)[1];
		r = src.at<Vec3b>(x, y)[2];

		// ������������������ʽ�ĸ�������
		bright = MYMAX(MYMAX(r, g), b);
		//bright = r + g + b;
		//bright = 0.3 * r + 0.6 * g + 0.1 * b;

		if (max < bright)
		{
			max = bright;
			maxi = x;
			maxj = y;
		}
	}

	air_light_pixel.x = maxi;
	air_light_pixel.y = maxj;
	air_light_pixel.b = src.at<Vec3b>(maxi, maxj)[0];
	air_light_pixel.g = src.at<Vec3b>(maxi, maxj)[1];
	air_light_pixel.r = src.at<Vec3b>(maxi, maxj)[2];

	free(allpixels);
	return clock() - t0;
}

/* ����͸���� */
int getTransDcp(Mat &trans, Mat & dc, Pixel & air_light_pixel) {
	long t0 = clock();
	// Aֵ���ȡ������ͨ������ͨ�����ӳ��A��������þ�ֵ
	float A = (air_light_pixel.b + air_light_pixel.g + air_light_pixel.r) / 3.0;
	float temp = 0;

	for (int i = 0; i < dc.rows; i++){
		for (int j = 0; j < dc.cols; j++){
			// �᲻�����temp<0������ͨ��ͼֵ�ȴ�����ֵ��ܶࣿ
			temp = 1 - DEHAZE_LEVEL * dc.at<uchar>(i, j) / A;
			trans.at<float>(i, j) = temp;
		}
	}
	return clock() - t0;
}

/* ȥ�� */
int dehaze(Mat & hazefree, Mat & src, Mat & trans, Pixel & air_light_pixel, uchar *gamma_table)
{
	long t0 = clock();
	int A_B = air_light_pixel.b;
	int A_G = air_light_pixel.g;
	int A_R = air_light_pixel.r;

	// ����gammaУ��
	if (gamma_table){
		for (int i = 0; i < src.rows; i++){
			for (int j = 0; j < src.cols; j++){
				hazefree.at<Vec3b>(i, j)[0] = (uchar)gamma_table[(uchar)(CLIP((src.at<Vec3b>(i, j)[0] - A_B) / CLIP_T(trans.at<float>(i, j)) + A_B))];
				hazefree.at<Vec3b>(i, j)[1] = (uchar)gamma_table[(uchar)(CLIP((src.at<Vec3b>(i, j)[1] - A_G) / CLIP_T(trans.at<float>(i, j)) + A_G))];
				hazefree.at<Vec3b>(i, j)[2] = (uchar)gamma_table[(uchar)(CLIP((src.at<Vec3b>(i, j)[2] - A_R) / CLIP_T(trans.at<float>(i, j)) + A_R))];
			}
		}
	}else{ // ��У��
		for (int i = 0; i < src.rows; i++){
			for (int j = 0; j < src.cols; j++){
				hazefree.at<Vec3b>(i, j)[0] = (uchar)CLIP( (src.at<Vec3b>(i, j)[0] - A_B) / CLIP_T(trans.at<float>(i, j)) + A_B );
				hazefree.at<Vec3b>(i, j)[1] = (uchar)CLIP( (src.at<Vec3b>(i, j)[1] - A_G) / CLIP_T(trans.at<float>(i, j)) + A_G );
				hazefree.at<Vec3b>(i, j)[2] = (uchar)CLIP( (src.at<Vec3b>(i, j)[2] - A_R) / CLIP_T(trans.at<float>(i, j)) + A_R );
			}
		}
	}
	return clock() - t0;
}