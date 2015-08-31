#include<opencv2/core.hpp>

///Compute the RootSIFT from SIFT according to Arandjelovic and Zisserman
void rootSIFT(cv::Mat& descriptors)
{
    // Compute sums for L1 Norm
    cv::Mat sums_vec;
    descriptors = cv::abs(descriptors); //otherwise we draw sqrt of negative vals
    cv::reduce(descriptors, sums_vec, 1 /*sum over columns*/, CV_REDUCE_SUM, CV_32FC1);
    for(unsigned int row = 0; row < descriptors.rows; row++)
    {
        int offset = row*descriptors.cols;
        for(unsigned int col = 0; col < descriptors.cols; col++)
        {
            descriptors.at<float>(offset + col) = 
                sqrt(descriptors.at<float>(offset + col) / sums_vec.at<float>(row) /*L1-Normalize*/);
        }
    }
}
