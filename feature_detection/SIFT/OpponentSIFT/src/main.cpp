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
#include <sift.h>
#include <vsfm_sift.h>
#include <vsfm_mat.h>

using namespace cv;
using namespace cv::xfeatures2d;

#define DEBUG

void apply_OpponentSIFT(cv::Mat& img,
    std::vector<cv::KeyPoint>& keypoints,
    cv::Mat& mask,cv::Mat& desc,
    struct SIFTDetector sift)
{
    cv::Mat channel[3];
    split(img, channel);
    //split the BGR channels

    cv::Mat o1,o2,o3;
    o1 = channel[2]-channel[1]/sqrt(2.);
    o2 = channel[2]+channel[1]-2*channel[0]/sqrt(6.);
    o3 = channel[2]+channel[1]+channel[0]/sqrt(3.);

    //salient colour boosting
    o1 = o1/(o1+o2+o3)*0.850;
    o2 = o2/(o1+o2+o3)*0.524;
    o3 = o3/(o1+o2+o3)*0.065;

    #ifdef DEBUG
    imwrite("o1.jpg", o1);
    imwrite("o2.jpg", o2);
    imwrite("o3.jpg", o3);
    #endif

    // Opponent-SIFT Apply SIFT to each opponent channel
    // Works pretty well
    sift(o1,mask,keypoints,desc);
    sift(o2,mask,keypoints,desc,true);
    sift(o3,mask,keypoints,desc,true);
}

bool has_suffix(const std::string& s, const std::string& suffix)
{
    return (s.size() >= suffix.size()) && equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

void listFile(char *argv,std::vector<std::string>& filenames)
{
        std::vector<std::string> extensions =
        {
            "ppm","PPM",
            "pbm","PBM",
            "pgm","PGM",
            "png","PNG",
            "jpg","JPG",//jpeg group extensions
            "jpeg","JPEG",
            "jpe","JPE",
            "tiff","TIFF",
            "tif","TIF",
            "bmp","BMP",
            "sr","SR",//Sun raster format
            "ras","RAS",
            "jp2","JP2",//Jasper images
        };

        DIR *pDIR;
        struct dirent *entry;
        if( pDIR=opendir(argv) ){
                while(entry = readdir(pDIR)){
                        if( strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 )
                        //std::cout << entry->d_name << std::endl;
                        //if file has image extension, add it
                        for(int i=0;i<extensions.size();i++)
                        {
                            if(has_suffix(entry->d_name,extensions.at(i)))
                            {
                                filenames.push_back(std::string(entry->d_name));
                                break;
                            }
                        }
                }
                closedir(pDIR);
        }
        #ifdef DEBUG
        for(int i=0;i<filenames.size();i++)
        {
            std::cout<<"Loading:"<<filenames.at(i)<<std::endl;
        }
        #endif
        return;
}

const String keys =
    "{help h usage ? |      | print this message              }"
    "{@image1        |      | image1 for compare              }"
    "{@image2        |      | image2 for compare              }"
    "{@directory     |      | directory of images to read     }"
    "{@list          |      | file with list of images to read}"
    "{@match         |      | match directory                 }"
    ;

void work_directory(char *argv,std::vector<std::string> filenames)
{
    listFile(argv,filenames);

    std::clock_t start;
    double duration;

    start = std::clock();

    SIFTDetector sift(7192,8,0.04,10,1.6);
    std::map<std::string,cv::Mat> file_descs;

    for(int i=0;i<filenames.size();i++)
    {
        std::string filename;
        filename+=argv;
        filename+="/";
        filename+=filenames.at(i);

        std::cout<<"Loading:"<<filename<<std::endl;

        cv::Mat img = imread(filename),mask,desc,output;
        std::vector<cv::KeyPoint> keypoints;

        apply_OpponentSIFT(img,keypoints,mask,desc,sift);

        //write_sift_binary(const_cast<char*>(filenames.at(i).c_str()),keypoints,desc);
        write_sift_ascii(const_cast<char*>(filenames.at(i).c_str()),keypoints,desc);
        file_descs[filenames.at(i)] = desc;

        imwrite(SplitFilename(filenames.at(i)), img);
    }

    //global matching, compare each file to every
    //other file
    for(int i=0;i<filenames.size();i++)
    {
        for(int j=0;j<filenames.size();j++)
        {
            if(j!=i)
            {
                //do the matching
                //BFMatcher matcher;
                FlannBasedMatcher matcher;
                std::vector<DMatch> matches;
                std::string current_file = filenames.at(i);
                std::string compare_file = filenames.at(j);
                cv::Mat desc1 = file_descs[current_file];
                cv::Mat desc2 = file_descs[compare_file];
                matcher.match(desc1,desc2,matches);

                write_match_file(current_file,compare_file,matches);
            }
        }
    }
}

int main(int argc,char *argv[])
{
    std::vector<std::string> filenames;
    CommandLineParser parser(argc, argv, keys);
    parser.about("OpponentSIFT");

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
        directory="/home/evan/Pictures/Vierzehnheiligen2/Images";
        //work on the directories
        work_directory(const_cast<char*>(directory.c_str()),filenames);
    }
    if(image1.size()>0&&image2.size()>0)
    {
        filenames.push_back(image1);
        filenames.push_back(image2);
    }

    return 0;
}
