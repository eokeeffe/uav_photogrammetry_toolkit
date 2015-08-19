#include <iostream>
#include <cstdio>
#include <ctime>

#include <vector>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <opencv2/features2d/features2d.hpp>

#include "SIFT.h"
#include "ASIFT.h"

using namespace cv;

int main(int argc,char *argv[])
{
    std::cout << "ASIFT Working" << std::endl;

    cv::Mat img = imread(std::string(argv[1])),mask,desc;
    std::vector<cv::KeyPoint> keypoints;

    std::clock_t start;
    double duration;

    start = std::clock();

    ASiftDetector detector;
    detector.detectAndCompute(img,keypoints,mask);

    duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;

    std::cout << "ASIFT Time taken:" << duration << " seconds" <<std::endl;

    // Add results to image and save.
    cv::Mat output,mask2;
    drawKeypoints(img, keypoints, output);
    imwrite("asift_result.jpg", output);

    start = std::clock();

    SIFTDetector sift;
    sift(img, mask2, keypoints, desc);

    duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;

    std::cout << "SIFT Time taken:" << duration << " seconds" <<std::endl;

    //Apply Hellinger kernel to feature descriptors
    //apply the Hellinger kernel by first L1-normalizing and taking the
    //square-root
    float eps = 1e-7f;
    desc /= sum(desc)[0] + eps; // same as: normalize(descr,descr,1,eps,NORM_L1);
    sqrt(desc,desc);
    descr /= norm(desc) + eps;

    // Add results to image and save.
    drawKeypoints(img, keypoints, output);
    imwrite("sift_result.jpg", output);

    return 0;
}
