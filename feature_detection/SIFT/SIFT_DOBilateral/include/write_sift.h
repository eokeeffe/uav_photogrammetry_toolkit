#ifndef __WRITE_SIFT__
#define __WRITE_SIFT__

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

std::string SplitFilename (const std::string& str);
std::string remove_extension(const std::string& filename);

void write_sift_ascii(char *filename,std::vector<cv::KeyPoint>& keypoints,cv::Mat& desc);
void write_sift_binary(char *filename,std::vector<cv::KeyPoint>& keypoints,cv::Mat& desc);

#endif
