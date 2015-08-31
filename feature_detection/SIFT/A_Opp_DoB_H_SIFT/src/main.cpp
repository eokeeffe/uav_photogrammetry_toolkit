#include <iostream>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <cstdlib>
#include <string.h>
#include <fstream>
#include <map>
//c headers
#include <dirent.h>
//opencv headers
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
//custom headers
#include <ASIFT.h>
#include <sift.h>
#include <vsfm_sift.h>
#include <vsfm_mat.h>
#include <my_sift.h>

using namespace cv;
using namespace cv::xfeatures2d;

int main(int argc,char *argv[])
{
    std::vector<std::string> filenames;
    CommandLineParser parser(argc, argv, keys);
    parser.about("Affine-Opponent-DoB-Hellinger-SIFT");

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    std::string directory = parser.get<std::string>("directory");
    std::string match_dir = parser.get<std::string>("match");
    std::string image1 = parser.get<std::string>("image1");
    std::string image2 = parser.get<std::string>("image2");;
    if(match_dir.size()>0&&
        image1.size()>0&&image2.size()>0)
    {

    }
    if(directory.size()<=0)
    {
        directory="/home/evan/UAV_Photogrammetry_toolkit/feature_detection/SIFT/A_Opp_DoB_H_SIFT/lena";
        //work on the directories
        work_directory(const_cast<char*>(directory.c_str()),filenames);
    }
    if(image1.size()>0&&image2.size()>0)
    {
        filenames.push_back(image1);
        filenames.push_back(image2);
    }
    if(image1.size()>0&&image2.size()<=0)
    {
        filenames.push_back(image1);
    }
    if(image1.size()<=0&&image2.size()>0)
    {
        filenames.push_back(image2);
    }

    return 0;
}
