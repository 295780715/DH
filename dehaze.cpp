#include "dehaze.h"

/*
根据文件路径，读取三通道图
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
暗通道去雾
*/
int dehaze_dcp(Mat & src) {
	// 初始窗口为3，可以自动调节，通过opencv的trackbar
	int window = 3;
	int max_window = 51;

	// 1.read image,U8C3类型

	// 2.获取灰度图
	Mat gray = Mat::zeros(src.size(), CV_8UC1);
	int time_gray = getGrayByMin(src, gray);

	// 3.获取暗通道图
	Mat dc = Mat::zeros(src.size(), CV_8UC1);
	int time_dc = getDarkChannel(gray, dc, window);

	// 4.获取大气光值A
	Pixel airLightPixel;
	int time_al = getAirLightDcp(dc, src, AIRLIGHT_RATIO, airLightPixel);

	// 5.获取透射图T，由于透射率t为浮点数，这里类型为32fc1,[0,1]之间值
	Mat trans = Mat::zeros(src.size(), CV_32FC1);
	int time_t = getTransDcp(trans, dc, airLightPixel);

	// 6.获取精细的投射图T,两种方式，未实现
	// softmatting()
	// guidefilter()

	// 7.获取去雾图
	Mat hazefree = Mat::zeros(src.size(),CV_8UC3);
	int time_dehaze = dehaze(hazefree, src, trans, airLightPixel, NULL);

	// 8.显示
	imshow("原图", src);
	imshow("灰度图", gray);
	imshow("透射图",trans);
	imshow("暗通道图", dc);
	imshow("去雾图", hazefree);

	// 调节暗通道窗口
	UserData ud;
	ud.data1 = &gray;
	ud.data2 = &dc;
	createTrackbar("window:", "暗通道图", &window, max_window, CallBack_Dcp_by_Window, (void*)&ud);

	int time_total = time_gray + time_dc + time_al + time_t + time_dehaze;
#ifdef DEBUG
	// debug信息输出
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
获取暗通道的动态窗口调节回调函数
*/
void CallBack_Dcp_by_Window(int window, void* userdata) {
	Mat * gray = ((UserData*)userdata)->data1;
	Mat * dc = ((UserData*)userdata)->data2;
	getDarkChannel(*gray, *dc, window);
	imshow("暗通道图", *dc);
}

/*行方向 求取窗口最小值*/
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
/*列方向 求取窗口最小值*/
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
/*最小值滤波*/
int myMinfilter(Mat &dst, const Mat &src, int window)
{
	Mat temp = Mat::zeros(src.size(), CV_8UC1);
	int radius = (window - 1) / 2;
	/*先以行方向为主，求取最小值*/
	horizonMinfilter(temp, src, radius);
	/*然后以列方向为主，求取最小值*/
	verticalMinfilter(dst, temp, radius);
	return 0;
}

/*
根据原彩色图，获取最小值灰度图
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
根据原彩色图像图像的灰度图，获取暗通道图
src 原雾图
dc 对应的暗通道图，必须为单通道图
window 暗通道窗口半径
*/
int getDarkChannel(const Mat & gray, Mat & dc, int window)
{
	long t0 = clock();
	// 根据灰度图，通过最小值滤波，及滤波半径，生成暗通道图
	myMinfilter(dc, gray, window);
	return clock() - t0;
}

/* qsort比较函数 */
int cmpByPixel(const void *p1, const void *p2)
{
	return ((Pixel *)p2)->value - ((Pixel *)p1)->value;
}

/* 估计全局大气光 */
int getAirLightDcp(Mat &dark_img, Mat &src, float ratio, Pixel & air_light_pixel)
{
	long t0 = clock();
	// 总像素数目
	int dark_size = dark_img.rows * dark_img.cols;
	// 前ratio个最亮的像素的数目
	int top_size = dark_size * ratio;
	// 所有的像素
	Pixel *allpixels = (Pixel *)malloc(sizeof(Pixel)*dark_size);

	// 限定求取A的样本最小为500个像素
	if (top_size < 500)
		top_size = 500;

	// 初始化像素值
	for (int i = 0; i < dark_img.rows; i++)
	{
		for (int j = 0; j < dark_img.cols; j++)
		{
			allpixels[i*dark_img.cols + j].x = i;
			allpixels[i*dark_img.cols + j].y = j;
			allpixels[i*dark_img.cols + j].value = dark_img.at<uchar>(i, j);
		}
	}

	// 库函数qsort为所有像素排序，排序函数为cmp
	qsort(allpixels, dark_size, sizeof(Pixel), cmpByPixel);

	// 根据allpixels的前top_size个像素到原图src中到对应的像素点最亮的点作为大气光值
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

		// 这三种最亮的描述方式哪个更合理？
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

/* 计算透射率 */
int getTransDcp(Mat &trans, Mat & dc, Pixel & air_light_pixel) {
	long t0 = clock();
	// A值如何取？从三通道到单通道如何映射A，这里采用均值
	float A = (air_light_pixel.b + air_light_pixel.g + air_light_pixel.r) / 3.0;
	float temp = 0;

	for (int i = 0; i < dc.rows; i++){
		for (int j = 0; j < dc.cols; j++){
			// 会不会出现temp<0，及暗通道图值比大气光值大很多？
			temp = 1 - DEHAZE_LEVEL * dc.at<uchar>(i, j) / A;
			trans.at<float>(i, j) = temp;
		}
	}
	return clock() - t0;
}

/* 去雾 */
int dehaze(Mat & hazefree, Mat & src, Mat & trans, Pixel & air_light_pixel, uchar *gamma_table)
{
	long t0 = clock();
	int A_B = air_light_pixel.b;
	int A_G = air_light_pixel.g;
	int A_R = air_light_pixel.r;

	// 进行gamma校正
	if (gamma_table){
		for (int i = 0; i < src.rows; i++){
			for (int j = 0; j < src.cols; j++){
				hazefree.at<Vec3b>(i, j)[0] = (uchar)gamma_table[(uchar)(CLIP((src.at<Vec3b>(i, j)[0] - A_B) / CLIP_T(trans.at<float>(i, j)) + A_B))];
				hazefree.at<Vec3b>(i, j)[1] = (uchar)gamma_table[(uchar)(CLIP((src.at<Vec3b>(i, j)[1] - A_G) / CLIP_T(trans.at<float>(i, j)) + A_G))];
				hazefree.at<Vec3b>(i, j)[2] = (uchar)gamma_table[(uchar)(CLIP((src.at<Vec3b>(i, j)[2] - A_R) / CLIP_T(trans.at<float>(i, j)) + A_R))];
			}
		}
	}else{ // 不校正
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