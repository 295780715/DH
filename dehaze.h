#ifndef __DEHAZE_H__
#define __DEHAZE_H__
#define DEBUG

#include <opencv2/opencv.hpp>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;
using namespace cv;

/*------------- 数据结构 ------------------*/
/* 用于TrackBar的CallBack传递数据的UserData数据结构，避免全局变量 */
typedef struct {
	Mat * data1;
	Mat * data2;
} UserData;

/* 像素 数据结构 */
typedef struct {
	int x, y;	 // 坐标
	int value;	 // 灰度值
	int b, g, r; // 彩色图为bgr三通道值
}Pixel;

/*------------- 去雾参数 ------------------------*/
/* 求取大气光值得百分比，论文默认为0.1% */
const float AIRLIGHT_RATIO = 0.001f;
/* 去雾程度，论文默认参数为0.95 */
const float DEHAZE_LEVEL = 0.95f;
/* 透射率t的下限值T0，这里取0.1 */
const float T0 = 0.1f;

/*------------- 功能函数 -----------------------*/
#define MYMIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MYMAX(a, b) (((a) >= (b)) ? (a) : (b))
/* [0,255]边界判断 */
#define CLIP(x) ((x)<(0)?0:((x)>(255)?(255):(x)))
/* 透射率t的阈值处理，防止t太小，导致恢复的无雾图过亮 */
#define CLIP_T(x) ((x)<(T0)?T0:((x)>(1.0f)?(1.0f):(x)))

/* 根据文件路径，读取三通道图 */
void readImage(Mat &output, char *img_name);
/* 根据原图src和透射率T及大气光A去雾，gamma_table为去雾后得伽马校正 */
int dehaze(Mat & hazefree, Mat & src, Mat & trans, Pixel & air_light_pixel, uchar *gamma_table);

/*------------- 最小值滤波 ---------------------*/
int myMinfilter(Mat &dst, const Mat &src, int window);
/* 列方向 求取窗口最小值 */
int verticalMinfilter(Mat &dst, const Mat &src, int radius);
/* 行方向 求取窗口最小值 */
int horizonMinfilter(Mat &dst, const Mat &src, int radius);

/*------------- DCP-HE（SOFTMATTING & GUIDEFILTER） -----------*/
/* 暗通道去雾 */
int dehaze_dcp(Mat & src);
/* 获取暗通道的动态窗口调节回调函数 */
void CallBack_Dcp_by_Window(int window, void* userdata);
/* 根据原彩色图获取最小值灰度图 */
int getGrayByMin(const Mat & src, Mat & gray);
/* 根据原图像的灰度图获取其对应的暗通道图 */
int getDarkChannel(const Mat & gray, Mat & dc, int window);
/* 根据原图及暗通道图，获取大气光值A，ratio为论文中提到的计算大气光的暗通道的最亮的前百分之几的点 */
int getAirLightDcp(Mat &dark_img, Mat &src, float ratio, Pixel & air_light_pixel);
/* 根据像素的灰度值的比较函数，用于求取A时的像素排序 */
int cmpByPixel(const void *p1, const void *p2);
/* 根据暗通道图dc和大气光值A，计算透射率T */
int getTransDcp(Mat &trans, Mat & dc, Pixel & air_light_pixel);

/*--------------- DCP TEST ---------------------------*/
int dehaze_dcp_test1(Mat & src);
int dehaze_dcp_test2(Mat & src);

#endif