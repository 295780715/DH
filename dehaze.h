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

/*------------- ���ݽṹ ------------------*/
/* ����TrackBar��CallBack�������ݵ�UserData���ݽṹ������ȫ�ֱ��� */
typedef struct {
	Mat * data1;
	Mat * data2;
} UserData;

/* ���� ���ݽṹ */
typedef struct {
	int x, y;	 // ����
	int value;	 // �Ҷ�ֵ
	int b, g, r; // ��ɫͼΪbgr��ͨ��ֵ
}Pixel;

/*------------- ȥ����� ------------------------*/
/* ��ȡ������ֵ�ðٷֱȣ�����Ĭ��Ϊ0.1% */
const float AIRLIGHT_RATIO = 0.001f;
/* ȥ��̶ȣ�����Ĭ�ϲ���Ϊ0.95 */
const float DEHAZE_LEVEL = 0.95f;
/* ͸����t������ֵT0������ȡ0.1 */
const float T0 = 0.1f;

/*------------- ���ܺ��� -----------------------*/
#define MYMIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MYMAX(a, b) (((a) >= (b)) ? (a) : (b))
/* [0,255]�߽��ж� */
#define CLIP(x) ((x)<(0)?0:((x)>(255)?(255):(x)))
/* ͸����t����ֵ������ֹt̫С�����»ָ�������ͼ���� */
#define CLIP_T(x) ((x)<(T0)?T0:((x)>(1.0f)?(1.0f):(x)))

/* �����ļ�·������ȡ��ͨ��ͼ */
void readImage(Mat &output, char *img_name);
/* ����ԭͼsrc��͸����T��������Aȥ��gamma_tableΪȥ����٤��У�� */
int dehaze(Mat & hazefree, Mat & src, Mat & trans, Pixel & air_light_pixel, uchar *gamma_table);

/*------------- ��Сֵ�˲� ---------------------*/
int myMinfilter(Mat &dst, const Mat &src, int window);
/* �з��� ��ȡ������Сֵ */
int verticalMinfilter(Mat &dst, const Mat &src, int radius);
/* �з��� ��ȡ������Сֵ */
int horizonMinfilter(Mat &dst, const Mat &src, int radius);

/*------------- DCP-HE��SOFTMATTING & GUIDEFILTER�� -----------*/
/* ��ͨ��ȥ�� */
int dehaze_dcp(Mat & src);
/* ��ȡ��ͨ���Ķ�̬���ڵ��ڻص����� */
void CallBack_Dcp_by_Window(int window, void* userdata);
/* ����ԭ��ɫͼ��ȡ��Сֵ�Ҷ�ͼ */
int getGrayByMin(const Mat & src, Mat & gray);
/* ����ԭͼ��ĻҶ�ͼ��ȡ���Ӧ�İ�ͨ��ͼ */
int getDarkChannel(const Mat & gray, Mat & dc, int window);
/* ����ԭͼ����ͨ��ͼ����ȡ������ֵA��ratioΪ�������ᵽ�ļ��������İ�ͨ����������ǰ�ٷ�֮���ĵ� */
int getAirLightDcp(Mat &dark_img, Mat &src, float ratio, Pixel & air_light_pixel);
/* �������صĻҶ�ֵ�ıȽϺ�����������ȡAʱ���������� */
int cmpByPixel(const void *p1, const void *p2);
/* ���ݰ�ͨ��ͼdc�ʹ�����ֵA������͸����T */
int getTransDcp(Mat &trans, Mat & dc, Pixel & air_light_pixel);

/*--------------- DCP TEST ---------------------------*/
int dehaze_dcp_test1(Mat & src);
int dehaze_dcp_test2(Mat & src);

#endif