#include <iostream>
#include <cstdio>
#include <ctime>
#include <cmath>

#include <vector>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <opencv2/features2d/features2d.hpp>

#include "sift.h"

using namespace cv;
using namespace cv::xfeatures2d;

int main(int argc,char *argv[])
{
    std::cout << "Hue SIFT Working" << std::endl;

    cv::Mat img = imread(std::string(argv[1])),mask,desc,output,output2;
    std::vector<cv::KeyPoint> keypoints,keypoints2;

    std::clock_t start;
    double duration;

    start = std::clock();

    cv::Mat channel[3];
    split(img, channel);
    //BGR

    cv::Mat o1,o2,o3;
    o1 = channel[2]-channel[1]/sqrt(2.);
    o2 = channel[2]+channel[1]-2*channel[0]/sqrt(6.);
    o3 = channel[2]+channel[1]+channel[0]/sqrt(3.);

    SIFTDetector sift(7192,8,0.04,10,1.6);

    // Opponent-SIFT Apply SIFT to each opponent channel
    // Works pretty well
    sift(o1,mask,keypoints,desc);
    sift(o2,mask,keypoints,desc);
    sift(o3,mask,keypoints,desc);

    duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;

    std::cout << "C-SIFT Time taken:" << duration << " seconds" <<std::endl;

    // Add results to image and save.
    drawKeypoints(img, keypoints, output);
    imwrite("o1.jpg", o1);
    imwrite("o2.jpg", o2);
    imwrite("o3.jpg", o3);
    imwrite("sift_output.jpg", output);

    return 0;
}
