#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <deque>

using namespace std;
using namespace cv;

inline double maxnum(double a,double b);
Mat boxfilter(Mat imSrc, int r);
Mat guidedfilter(Mat I,Mat p,int r,double eps);
void MakeMapping(int* Histgram,float CutLimit=0.01);
IplImage* dehaze(IplImage *src,int block,double w);
Mat dehaze_cplusplus(Mat& img,double kenlRatio,int minAtomsLight);
