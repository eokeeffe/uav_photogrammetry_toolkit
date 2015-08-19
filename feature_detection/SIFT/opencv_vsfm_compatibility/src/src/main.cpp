#include <iostream>
#include <cstdio>
#include <ctime>

#include <vector>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <sift.h>
#include <write_sift.h>
#include <write_mat.h>
#include <parser.h>

using namespace cv;
using namespace cv::xfeatures2d;

void parseCMD(int argc,char *argv[])
{
    CommandLineParser parser(argc, argv, keys);
    parser.about("Application name v1.0.0");

    if (parser.has("help"))
    {
        parser.printMessage();
        exit(0);
    }

    int N = parser.get<int>("N");
    double fps = parser.get<double>("fps");
    String path = parser.get<String>("path");

    String img1 = parser.get<String>(0);
    String img2 = parser.get<String>(1);

    int repeat = parser.get<int>(2);

    if (!parser.check())
    {
        parser.printErrors();
        exit(0);
    }
}

int main(int argc,char *argv[])
{
    std::cout << "SIFT DOB Working" << std::endl;

    cv::Mat img = imread(std::string(argv[1])),mask,desc,output;
    cv::Mat img2 = imread(std::string(argv[2])),mask2,desc2,output2;
    std::vector<cv::KeyPoint> keypoints,keypoints2;

    std::clock_t start;
    double duration;

    start = std::clock();

    cv::Mat channel[3];
    split(img, channel);

    SIFTDetector sift(500,8,0.04,10,1.6);
    sift(img, mask, keypoints, desc);
    sift(img2, mask2, keypoints2, desc2);

    duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;

    std::cout << "SIFT Time taken:" << duration << " seconds" <<std::endl;

    write_sift_ascii(argv[1],keypoints,desc);
    write_sift_ascii(argv[2],keypoints2,desc2);
    //write_sift_binary(argv,keypoints,desc);



    // Add results to image and save.
    //drawKeypoints(img, keypoints, output);
    //imwrite("sift_output_dob.jpg", output);

    //-- Step 3: Matching descriptor vectors using BFMatcher :
    BFMatcher matcher;
    std::vector<DMatch> matches;
    matcher.match(desc,desc2,matches);

    std::vector<std::string> strings;
    strings.push_back(std::string(argv[1]));
    strings.push_back(std::string(argv[2]));
    write_match_file(strings,matches);

    std::cout<<"All feature detection and matching files created"<<std::endl;

    return 0;
}
