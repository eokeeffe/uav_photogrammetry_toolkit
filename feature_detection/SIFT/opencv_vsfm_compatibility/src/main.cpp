#include <iostream>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <cstdlib>
#include <string.h>
#include <fstream>
#include <dirent.h>
#include <stdio.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <sift.h>
#include <vsfm_sift.h>
#include <vsfm_mat.h>

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;



void apply_OpponentSIFT(cv::Mat& img,
    std::vector<cv::KeyPoint> keypoints,
    cv::Mat& mask,cv::Mat& desc,
    struct SIFTDetector sift)
{
    cv::Mat channel[3];
    split(img, channel);
    //BGR

    cv::Mat o1,o2,o3;
    o1 = channel[2]-channel[1]/sqrt(2.);
    o2 = channel[2]+channel[1]-2*channel[0]/sqrt(6.);
    o3 = channel[2]+channel[1]+channel[0]/sqrt(3.);

    #ifdef DEBUG
    imwrite("o1.jpg", o1);
    imwrite("o2.jpg", o2);
    imwrite("o3.jpg", o3);
    #endif

    // Opponent-SIFT Apply SIFT to each opponent channel
    // Works pretty well
    sift(o1,mask,keypoints,desc);
    sift(o2,mask,keypoints,desc);
    sift(o3,mask,keypoints,desc);
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
    "{directory      |.     | directory of images to read     }"
    "{@list          |      | file with list of images to read}"
    "{match          |.     | match directory                 }"
    ;

void work_directory(char *argv,std::vector<std::string> filenames)
{
    listFile(argv,filenames);

    std::clock_t start;
    double duration;

    start = std::clock();

    SIFTDetector sift(7192,8,0.04,10,1.6);

    for(int i=0;i<filenames.size();i++)
    {
        std::string filename;
        filename+=argv[1];
        filename+="/";
        filename+=filenames.at(i);

        cv::Mat img = imread(filename),mask,desc,output;
        std::vector<cv::KeyPoint> keypoints;

        apply_OpponentSIFT(img,keypoints,mask,desc,sift);

        write_sift_binary(const_cast<char*>(filenames.at(i).c_str()),keypoints,desc);

        imwrite(SplitFilename(filenames.at(i)), img);
    }
}

std::vector<std::string> tokenize(std::string line,std::string delimiter)
{
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = line.find(delimiter)) != std::string::npos)
    {
        token = line.substr(0, pos);
        //std::cout << token << std::endl;
        tokens.push_back(token);
        line.erase(0, pos + delimiter.length());
    }
    return tokens;
}

int main(int argc,char *argv[])
{
    std::vector<std::string> filenames;
    std::cout << "Reading match file Working" << std::endl;

    CommandLineParser parser(argc, argv, keys);
    parser.about("Read SIFT .mat file");

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    std::string directory = parser.get<std::string>("directory");
    std::string match_dir = parser.get<std::string>("match");
    std::string image1 = parser.get<std::string>("image1");
    std::string image2 = parser.get<std::string>("image2");

    std::string s,garbage;
    int number_of_files=0;

    //std::cout<<directory<<","<<match_dir<<std::endl;

    FILE *file = fopen("c0.mat","rb");
    while(!feof(file))
    {
        int numf;
        char file1[200],file2[200];
        char buffer[200];
        fscanf(file, "%s,%s,%d%*s", file1,file2, &numf);
        fscanf(file,"%*s%*s",buffer);
        fscanf(file,"%*s%*s",buffer);
        cout<<string(file1)<<","<<string(file2)<<endl;
    }
    fclose(file);

    return 0;
}
