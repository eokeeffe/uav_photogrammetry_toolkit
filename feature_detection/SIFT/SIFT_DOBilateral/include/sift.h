#ifndef SIFT_H_
#define SIFT_H_


#include <opencv2/core/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv::xfeatures2d;

struct SIFTDetector
{
    cv::Ptr<cv::Feature2D> sift;
    SIFTDetector(int _nfeatures, int _nOctaveLayers,
                         double _contrastThreshold, double _edgeThreshold, double _sigma)
    {
        sift = cv::xfeatures2d::SIFT::create(_nfeatures,_nOctaveLayers,
                             _contrastThreshold,_edgeThreshold,_sigma);
    }
    template<class T>
    void operator()(const T& in, const T& mask, std::vector<cv::KeyPoint>& pts, T& descriptors, bool useProvided = false)
    {
        sift->detectAndCompute(in, mask, pts, descriptors, useProvided);
    }
};
//end of SIFT detector
#endif
