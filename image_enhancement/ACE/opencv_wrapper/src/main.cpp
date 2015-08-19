/*
*   Simple Wrapper for ACE
*/
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <cmath>
#include <vector>
#include <queue>
#include <limits>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;

void ACE(cv::Mat& input,cv::Mat& output,int a,int interp)
{
    output=input.clone();
    imwrite("in.jpg",input);
    string base = "./ace -a "+ to_string(a) +" -m interp:"+ to_string(interp) +" in.jpg out.jpg";
    int s1 = system(base.c_str());
    output=imread("out.jpg");
    int s2 = system("rm in.jpg out.jpg");
    return;
}

int main(int argc,char *argv[])
{
    //inputs
    cv::Mat input;
    input = imread(argv[1]);
    cv::Mat ace;
    ACE(input,ace,8,12);

    imwrite("ace.jpg",ace);
    return 0;
}
