#include <iostream>
#include <cstdio>
#include <ctime>

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
    std::cout << "C-SIFT Working" << std::endl;

    cv::Mat img = imread(std::string(argv[1])),mask,desc,output,output2;
    std::vector<cv::KeyPoint> keypoints,keypoints2;

    std::clock_t start;
    double duration;

    start = std::clock();

    cv::Mat channel[3];
    split(img, channel);

    SIFTDetector sift(450);

    // C-SIFT Apply SIFT to each RGB colour channel
    // Works pretty well
    sift(channel[0],mask,keypoints,desc);
    sift(channel[1],mask,keypoints,desc);
    sift(channel[2],mask,keypoints,desc);

    duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;

    std::cout << "C-SIFT Time taken:" << duration << " seconds" <<std::endl;

    // Add results to image and save.
    drawKeypoints(img, keypoints, output);
    imwrite("sift_output.jpg", output);

    return 0;
}
