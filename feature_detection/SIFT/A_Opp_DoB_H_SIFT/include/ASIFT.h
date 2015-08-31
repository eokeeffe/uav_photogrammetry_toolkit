#ifndef ASIFT_H_ // beginning
#define ASIFT_H_

//standard headers
#include <iostream>
#include <cmath>
#include <limits>
#include <vector>

//openmp
#include <omp.h>
//custom sift
#include <sift.h>

//#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/opencv_modules.hpp>

class ASiftDetector
{
    public:
        ASiftDetector();
        ASiftDetector(int nfeatures,int nOctaveLayers,
            double contrastThreshold,double edgeThreshold,
            double sigma);
        ~ASiftDetector();
        void detectAndCompute(const cv::Mat& img, std::vector< cv::KeyPoint >& keypoints, cv::Mat& descriptors);
    private:
        int nfeatures=0;
        int nOctaveLayers=3;
        double contrastThreshold=0.04;
        double edgeThreshold=10;
        double sigma=1.6;
        void affineSkew(double& tilt, double phi, cv::Mat& img, cv::Mat& mask, cv::Mat& Ai);
};
// end of ASIFT_H_
#endif
