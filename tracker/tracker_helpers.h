#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include <time.h>

#include "psmove.h"

#ifndef TRACKER_HELPERS_H
#define TRACKER_HELPERS_H

#define th_odd(x) (((x)/2)*2+1)
#define th_black cvScalarAll(0x0)
#define th_white cvScalar(0xFF,0xFF,0xFF,0)
#define th_green cvScalar(0,0xff,0,0)
#define th_red cvScalar(0,0,0xff,0)
#define th_blue cvScalar(0xff,0,0,0)
#define th_min(x,y) ((x)<(y)?(y):(x))
#define th_max(x,y) ((x)<(y)?(x):(y))
#define th_scalar_to_rgb_int(c)(((int)(c).val[2])<<16 && ((int)(c).val[1])<<8 && ((int)(c).val[0]))
#define th_PI 3.14159265358979

// some basic statistical functions on arrays
double th_var(double* src, int len);
double th_avg(double* src, int len);
double th_magnitude(double* src, int len);
void th_minus(double* l, double* r, double* result, int len);
void th_plus(double* l, double* r, double* result, int len);
void th_mul(double* array, double f, double* result, int len);

// only creates the image/storage, if it does not already exist, or has different properties (size, depth, channels, blocksize ...)
// returns (1: if image/storage is created) (0: if nothing has been done)
int th_create_image(IplImage** img, CvSize s, int depth, int channels);
int th_create_mem_storage(CvMemStorage** stor, int block_size);
int th_create_hist(CvHistogram** hist, int dims, int* sizes, int type,
		float** ranges, int uniform);

void th_put_text(IplImage* img, const char* text, CvPoint p, CvScalar color, float scale);
IplImage* th_plot_hist(CvHistogram* hist, int bins, const char* windowName, CvScalar lineColor);

// simly saves a CvArr* on the filesystem
int th_save_jpg(const char* path, const CvArr* image, int quality);
int th_save_jpgEx(const char* folder, const char* filename, int prefix, const CvArr* image, int quality);

// prints a array to system out ala {a,b,c...}
void th_print_array(double* src, int len);

// converts HSV color to a BGR color and back
CvScalar th_hsv2bgr(CvScalar hsv);
CvScalar th_brg2hsv(CvScalar bgr);

// waits until the uses presses ESC (only works if a windo is visible)
void th_wait_esc();
void th_wait(char c);
void th_wait_move_button(PSMove* move, int button);
int th_move_button(PSMove* move, int button);

void th_equalize_image(IplImage* img);
#endif // TRACKER_HELPERS_H

