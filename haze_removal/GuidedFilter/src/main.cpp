#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <string>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <deque>
#include <guided_dehazing.hpp>

std::string remove_extension(const std::string& filename) 
{
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == std::string::npos) return filename;
    return filename.substr(0, lastdot); 
}

using namespace std;
using namespace cv;

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
    
    string output = remove_extension(argv[1]);
    output+="_dehazed.jpg";
    imwrite(output,dst);

    return 0;
}
