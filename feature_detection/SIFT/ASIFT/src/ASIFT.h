#ifndef ASIFT_H_ // beginning
#define ASIFT_H_

#include <vector>

#include <opencv2/core/core.hpp>

class ASiftDetector
{
    public:
        ASiftDetector();
        ~ASiftDetector();
        void detectAndCompute(const cv::Mat& img, std::vector< cv::KeyPoint >& keypoints, cv::Mat& descriptors);
    private:
        void affineSkew(double tilt, double phi, cv::Mat& img, cv::Mat& mask, cv::Mat& Ai);
};
// end of ASIFT_H_
#endif
