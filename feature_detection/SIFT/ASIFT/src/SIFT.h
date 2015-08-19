#ifndef SIFT_H_
#define SIFT_H_

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv::xfeatures2d;

struct SIFTDetector
{
    cv::Ptr<cv::Feature2D> sift;
    SIFTDetector(double hessian = 800.0)
    {
        sift = cv::xfeatures2d::SIFT::create(hessian);
    }
    template<class T>
    void operator()(const T& in, const T& mask, std::vector<cv::KeyPoint>& pts, T& descriptors, bool useProvided = false)
    {
        sift->detectAndCompute(in, mask, pts, descriptors, useProvided);
    }
};
//end of SIFT detector
#endif
