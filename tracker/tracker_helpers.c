#include <stdio.h>
#ifdef WIN32
#    include <windows.h>
#endif
#include <unistd.h>
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/core/core_c.h"
#include "tracker_helpers.h"
#include "iniparser.h"
#include "dictionary.h"

double th_var(double* src, int len) {
	double f = 1.0 / (len - 1);
	int i;
	double sum = 0;
	double d = 0;
	double avg = th_avg(src, len);
	for (i = 0; i < len; i++) {
		d = src[i] - avg;
		sum = sum + d * d;
	}
	return sum * f;
}

double th_avg(double* src, int len) {
	double sum = 0;
	int i = 0;
	for (i = 0; i < len; i++)
		sum = sum + src[i];
	return sum / len;
}

double th_magnitude(double* src, int len) {
	double sum = 0;
	int i;
	for (i = 0; i < len; i++)
		sum = sum + src[i] * src[i];

	return sqrt(sum);
}

void th_minus(double* l, double* r, double* result, int len) {
	int i;
	for (i = 0; i < len; i++)
		result[i] = l[i] - r[i];
}

void th_plus(double* l, double* r, double* result, int len) {
	int i;
	for (i = 0; i < len; i++)
		result[i] = l[i] + r[i];
}

void th_mul(double* array, double f, double* result, int len) {
	int i;
	for (i = 0; i < len; i++)
		result[i] = array[i] *f;
}

int th_create_image(IplImage** img, CvSize s, int depth, int channels) {
	int R = 0;
	IplImage* src = *img;
	if (src == 0x0 || src->depth != depth || src->width != s.width || src->height != s.height || src->nChannels != channels) {
		// yes it exists, but has wrong properties -> delete it!
		if (src != 0x0)
			cvReleaseImage(img);
		// now the new one can safely be created
		*img = cvCreateImage(s, depth, channels);
		R = 1;
	}
	return R;
}
int th_create_hist(CvHistogram** hist, int dims, int* sizes, int type, float** ranges, int uniform) {
	int R = 0;
	CvHistogram* src = *hist;
	if (src == 0x0) {
		// yes it exists, but has wrong properties -> delete it!
		if (src != 0x0)
			cvReleaseHist(hist);
		// now the new one can safely be created
		*hist = cvCreateHist(dims, sizes, type, ranges, uniform);
		R = 1;
	}
	return R;
}

int th_create_mem_storage(CvMemStorage** stor, int block_size) {
	int R = 0;
	CvMemStorage* src = *stor;
	if (src == 0x0 || src->block_size != block_size) {
		// yes it exists, but has wrong properties -> delete it!
		if (src != 0x0)
			cvReleaseMemStorage(stor);
		// now the new one can safely be created
		*stor = cvCreateMemStorage(block_size);
		R = 1;
	}
	return R;
}

void th_put_text(IplImage* img, const char* text, CvPoint p, CvScalar color, float scale) {
	CvFont font = cvFont(scale,1);
	cvPutText(img, text, p, &font, color);
}

IplImage* histImg;
IplImage* th_plot_hist(CvHistogram* hist, int bins, const char* windowName, CvScalar lineColor) {
	th_create_image(&histImg, cvSize(512, 512 * 0.6), 8, 3);
	float xStep = histImg->width / bins;
	CvPoint p0, p1;
	cvSet(histImg, th_black, 0x0);

	float max_value = 0;
	cvGetMinMaxHistValue(hist, 0, &max_value, 0, 0);

	int i;
	for (i = 1; i < bins; i++) {
		float v0 = cvGetReal1D(hist->bins, i - 1) / max_value * histImg->height;
		float v1 = cvGetReal1D(hist->bins, i) / max_value * histImg->height;
		p0 = cvPoint((i - 1) * xStep, histImg->height - v0);
		p1 = cvPoint(i * xStep, histImg->height - v1);
		cvLine(histImg, p0, p1, lineColor, 2, 8, 0);
	}

	//cvhPutText(histImg, "hello", cvPoint(33, 33), cvScalar(0xff, 0, 0xff, 0));
	cvShowImage(windowName, histImg);
	return histImg;
}

int th_save_jpg(const char* path, const CvArr* image, int quality) {
	int imgParams[] = { CV_IMWRITE_JPEG_QUALITY, quality, 0 };
	return cvSaveImage(path, image, imgParams);
}

int th_save_jpgEx(const char* folder, const char* filename, int prefix, const CvArr* image, int quality) {
	char path[512];
	sprintf(path, "%s/%d_%s", folder, prefix, filename);
	return th_save_jpg(path, image, quality);
}

void th_print_array(double* src, int len) {
	int i;
	printf("%s", "{");
	if (len > 0)
		printf("%.2f", src[0]);
	for (i = 1; i < len; i++)
		printf(", %.2f", src[i]);
	printf("%s", "}\n");
}

IplImage* pxHSV;
IplImage* pxBGR;
CvScalar th_hsv2bgr(CvScalar hsv) {
	if (pxHSV == 0x0) {
		pxHSV = cvCreateImage(cvSize(1, 1), IPL_DEPTH_8U, 3);
		pxBGR = cvCloneImage(pxHSV);
	}

	cvSet(pxHSV, hsv, 0x0);
	cvCvtColor(pxHSV, pxBGR, CV_HSV2BGR);
	return cvAvg(pxBGR, 0x0);

}
CvScalar th_brg2hsv(CvScalar bgr) {
	if (pxHSV == 0x0) {
		pxHSV = cvCreateImage(cvSize(1, 1), IPL_DEPTH_8U, 3);
		pxBGR = cvCloneImage(pxHSV);
	}
	cvSet(pxBGR, bgr, 0x0);
	cvCvtColor(pxBGR, pxHSV, CV_BGR2HSV);
	return cvAvg(pxHSV, 0x0);
}

CvScalar th_hsv2bgr_alt(float hue) {
	int rgb[3], p, sector;
	while ((hue >= 180))
		hue = hue - 180;

	while ((hue < 0))
		hue = hue + 180;

	int sectorData[][3] = { { 0, 2, 1 }, { 1, 2, 0 }, { 1, 0, 2 }, { 2, 0, 1 }, { 2, 1, 0 }, { 0, 1, 2 } };
	hue *= 0.033333333333333333333333333333333f;
	sector = cvFloor(hue);
	p = cvRound(255 * (hue - sector));
	p ^= sector & 1 ? 255 : 0;
	rgb[sectorData[sector][0]] = 255;
	rgb[sectorData[sector][1]] = 0;
	rgb[sectorData[sector][2]] = p;
	return cvScalar(rgb[2], rgb[1], rgb[0], 0);
}

int th_move_button(PSMove* move, int button) {
	int pressed;

	psmove_poll(move);
	pressed = psmove_get_buttons(move);
	return pressed & button;

}
void th_wait_move_button(PSMove* move, int button) {
	int pressed;
	while (1) {
		psmove_poll(move);
		pressed = psmove_get_buttons(move);
		if (pressed & button)
			break;
		usleep(10000);
	}
}

void th_wait_esc() {
	while (1) {
		//If ESC key pressed
		if ((cvWaitKey(10) & 255) == 27)
			break;
	}
}

void th_wait(char c) {
	while (1) {
		//If ESC key pressed
		if ((cvWaitKey(10) & 255) == c)
			break;
	}
}


IplImage* ch0 = 0x0;
IplImage* ch1 = 0x0;
IplImage* ch2 = 0x0;
void th_equalize_image(IplImage* img) {
	//return frame;
	th_create_image(&ch0, cvGetSize(img), img->depth, 1);
	th_create_image(&ch1, cvGetSize(img), img->depth, 1);
	th_create_image(&ch2, cvGetSize(img), img->depth, 1);

	cvSplit(img, ch0, ch1, ch2, 0x0);
	cvEqualizeHist(ch0, ch0);
	cvEqualizeHist(ch1, ch1);
	cvEqualizeHist(ch2, ch2);
	cvMerge(ch0, ch1, ch2, 0x0, img);
}

int th_file_exists(const char* file) {
	FILE *fp = fopen(file, "r");
	int ret = fp != 0x0;
	if (fp)
		fclose(fp);

	return ret;
}

