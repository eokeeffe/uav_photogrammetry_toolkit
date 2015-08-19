#ifndef __WRITE_MAT__
#define __WRITE_MAT__

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

void write_match_file(std::vector<std::string> filenames,
    std::vector<cv::DMatch>& matches);

#endif
