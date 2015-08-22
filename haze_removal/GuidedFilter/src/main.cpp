#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <deque>
#include <guided_dehazing.hpp>

int main(int argc,char *argv[])
{
	    double kenlRatio = 0.01;
		int minAtomsLight = 220;//max
		Mat img=imread(argv[1]);

		double t = (double)cvGetTickCount();
		Mat dst=dehaze_cplusplus(img,kenlRatio,minAtomsLight);

		t = (double)cvGetTickCount() - t;
		cout<< "cost time: " << t / ((double)cvGetTickFrequency()*1000000.) <<" s"<< endl;
		cout<<cvRound(((double)cvGetTickFrequency()*1000000.)/t)<<"FPS"<<endl;

		imwrite("input.jpg",img);
		imwrite("out.jpg",dst);

		return 0;
}
