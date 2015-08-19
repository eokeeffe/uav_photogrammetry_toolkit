#ifndef __VSFM_MAT__
#define __VSFM_MAT__

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

/*
    Reading and writing VSFM binary matching files
*/

void write_match_file(std::vector<std::string> filenames,
    std::vector<cv::DMatch>& matches);
void read_match_file(std::string filename,
    std::vector<cv::DMatch>& matches);

#endif
