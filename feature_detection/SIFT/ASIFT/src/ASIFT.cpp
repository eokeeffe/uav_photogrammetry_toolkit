#include <iostream>
#include <cmath>
#include <limits>

#include "ASIFT.h"
#include "SIFT.h"

#include <omp.h>

#include <opencv2/features2d/features2d.hpp>
//#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/utility.hpp>

#include <opencv2/opencv_modules.hpp>
// OpenCV GPU
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudawarping.hpp>


using namespace cv;
using namespace cv::cuda;

const double epsilon = std::numeric_limits<double>::epsilon();

bool notEqualDouble(double a,double b)
{
    if(fabs(a - b) <= epsilon * fabs(b))
    // if(a != b)
    {
        return true;
    }
    return false;
}

ASiftDetector::ASiftDetector()
{
    cv::cuda::setDevice(0);
    //cv::cuda::printShortCudaDeviceInfo(cv::cuda::getDevice());
}

ASiftDetector::~ASiftDetector()
{

}

void ASiftDetector::detectAndCompute(const Mat& img, std::vector< cv::KeyPoint >& keypoints, cv::Mat& descriptors)
{
    keypoints.clear();
    descriptors = cv::Mat(0, 128, CV_32F);
    //private()
    int h=img.rows;
    int w=img.cols;
    #pragma omp parallel for shared(img,keypoints,descriptors,h,w)
    for(int tl = 1; tl < 7; tl++)
    {
        double t = ((0.5*tl)*(0.5*tl));
        //#pragma omp parrallel for private(t) shared(keypoints,desciptors,img,h,w)
        for(int phi = 0; phi < 180; phi += 72.0/t)
        {
            std::vector<cv::KeyPoint> kps;
            cv::Mat timg,desc, mask, Ai;

            img.copyTo(timg);
            //timg = img;
            affineSkew(t, phi, timg, mask, Ai);

            SIFTDetector sift;
            sift(timg, mask, kps, desc);

            #pragma omp parrallel for shared(kps,Ai)
            for(unsigned int i = 0; i < kps.size(); i++)
            {
                cv::Mat kpt_t = Ai * cv::Mat(cv::Point3f(kps[i].pt.x, kps[i].pt.y, 1));
                kps[i].pt.x = kpt_t.at<float>(0,0);
                kps[i].pt.y = kpt_t.at<float>(1,0);
            }
            keypoints.insert(keypoints.end(), kps.begin(), kps.end());
            descriptors.push_back(desc);
        }
    }
}

void ASiftDetector::affineSkew(double tilt, double phi, cv::Mat& img, cv::Mat& mask, cv::Mat& Ai)
{
    int h = img.rows;
    int w = img.cols;

    mask = cv::Mat(h, w, CV_8UC1, cv::Scalar(255));
    cv::Mat A = cv::Mat::eye(2,3, CV_32F);

    if(notEqualDouble(phi,0.0))
    //if(phi != 0.0)
    {
        phi *= M_PI/180.;
        double s = sin(phi);
        double c = cos(phi);

        A = (Mat_<float>(2,2) << c, -s, s, c);

        Mat corners = (Mat_<float>(4,2) << 0, 0, w, 0, w, h, 0, h);
        Mat tcorners = corners*A.t();

        std::vector<Mat> channels;
        channels.push_back(tcorners.col(0));
        channels.push_back(tcorners.col(1));
        merge(channels, tcorners);

        Rect rect = boundingRect(tcorners);

        //std::cout<< rect.x << "," << rect.y << std::endl;

        A =  (Mat_<float>(2,3) << c, -s, -rect.x, s, c, -rect.y);
        cv::warpAffine(img, img, A, cv::Size(rect.width, rect.height), INTER_NEAREST, BORDER_REPLICATE);
    }
    if(notEqualDouble(tilt, 1.0))
    {
        GaussianBlur(img, img, cv::Size(1,3), (0.8*sqrt(tilt*tilt-1)), 0.01);
        cv::resize(img, img, cv::Size(0,0), 1.0/tilt, 1.0, INTER_NEAREST);
        A.row(0) = A.row(0)/tilt;
    }
    if(notEqualDouble(tilt,1.0) || notEqualDouble(phi,0.0))
    {
        //cv::cuda::GpuMat gimg = GpuMat(img);
        //cv::cuda::warpAffine(gimg, gimg, A, gimg.size(), INTER_LINEAR, BORDER_REPLICATE);
        //img = cv::Mat(gimg);
        cv::warpAffine(mask, mask, A, cv::Size(w,h), INTER_NEAREST);
    }
    invertAffineTransform(A, Ai);
}
