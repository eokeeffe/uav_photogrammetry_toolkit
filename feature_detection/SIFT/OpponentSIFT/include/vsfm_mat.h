#ifndef __VSFM_MAT__
#define __VSFM_MAT__

#include <map>
#include <string>
#include <vector>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

/*
    Reading and writing VSFM binary matching files
*/

void write_match_file(std::string first_file,
    std::string second_file,
    std::vector<cv::DMatch>& matches);
void read_match_file(std::string filename,
    std::map<std::string,std::map<std::string,std::vector<cv::DMatch> > >& file_matches);

#endif
