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
#include <my_sift.h>
#include <vsfm_sift.h>
#include <vsfm_mat.h>
#include <ASIFT.h>

using namespace std;
using namespace cv;

void apply_hellinger(cv::Mat& desc)
{
    //Apply Hellinger kernel to feature descriptors
    //apply the Hellinger kernel by first L1-normalizing and taking the
    //square-root
    
    desc.convertTo(desc,CV_32F);
    float eps = 1e-7f;
    desc /= sum(desc)[0] + eps; // same as: normalize(descr,descr,1,eps,NORM_L1);
    sqrt(desc,desc);
    desc /= norm(desc) + eps;
    //desc.convertTo(desc,CV_8U);
}

static void convertBGRImageToOpponentColorSpace( const Mat& bgrImage,std::vector<cv::Mat>& opponentChannels)
{
    if( bgrImage.type() != CV_8UC3 )
    {CV_Error( Error::StsBadArg, "input image must be an BGR image of type CV_8UC3" );}

    // Prepare opponent color space storage matrices.
    opponentChannels[0] = cv::Mat(bgrImage.size(), CV_8UC1); // R-G RED-GREEN
    opponentChannels[1] = cv::Mat(bgrImage.size(), CV_8UC1); // R+G-2B YELLOW-BLUE
    opponentChannels[2] = cv::Mat(bgrImage.size(), CV_8UC1); // R+G+B

    const float s2=sqrt(2.), s3=sqrt(3.),s6=sqrt(6.);
    
    for(int y = 0; y < bgrImage.rows; ++y)
    {
        for(int x = 0; x < bgrImage.cols; ++x)
        {
            Vec3b v = bgrImage.at<Vec3b>(y, x);
            uchar& b = v[0];
            uchar& g = v[1];
            uchar& r = v[2];
            
            // (R - G)/sqrt(2), but converted to the destination data type
            opponentChannels[0].at<uchar>(y, x) = saturate_cast<uchar>(0.5f    * (255 + g - r));       
            // (R + G - 2B)/sqrt(6), but converted to the destination data type
            opponentChannels[1].at<uchar>(y, x) = saturate_cast<uchar>(0.25f   * (510 + r + g - 2*b)); 
            // (R + G + B)/sqrt(3), but converted to the destination data type
            opponentChannels[2].at<uchar>(y, x) = saturate_cast<uchar>(1.f/3.f * (r + g + b));         
        }
    }
}

struct KP_LessThan
{
    KP_LessThan(const std::vector<KeyPoint>& _kp) : kp(&_kp) {}
    bool operator()(int i, int j) const
    {
        return (*kp)[i].class_id < (*kp)[j].class_id;
    }
    const std::vector<KeyPoint>* kp;
};

void apply_SIFT(cv::Mat& img,
    std::vector<cv::KeyPoint> keypoints,
    cv::Mat& mask,cv::Mat& descriptors,
    ASiftDetector& detector)
{
    std::vector<cv::Mat> opponentChannels(3);
    convertBGRImageToOpponentColorSpace(img,opponentChannels);
    
    //salient colour boosting
    //o1 = o1/(o1+o2+o3)*0.850;
    //o2 = o2/(o1+o2+o3)*0.524;
    //o3 = o3/(o1+o2+o3)*0.065;
    
    #ifdef DEBUG
        imwrite("o1.jpg", opponentChannels[0]);
        imwrite("o2.jpg", opponentChannels[1]);
        imwrite("o3.jpg", opponentChannels[2]);
    #endif

    // Opponent-SIFT Apply SIFT to each opponent channel
    int N=3;
    std::vector<cv::KeyPoint> channelKeypoints[N];
    cv::Mat channelDescriptors[N];
    std::vector<int> idxs[N];
    
    // Compute descriptors three times, once for each Opponent channel to concatenate into a single color descriptor
    int maxKeypointsCount = 0;
    for( int ci = 0; ci < N; ci++ )
    {
        channelKeypoints[ci].insert( channelKeypoints[ci].begin(), keypoints.begin(), keypoints.end() );
        // Use class_id member to get indices into initial keypoints vector
        for( size_t ki = 0; ki < channelKeypoints[ci].size(); ki++ )
        {channelKeypoints[ci][ki].class_id = (int)ki;}

        detector.detectAndCompute( opponentChannels[ci], channelKeypoints[ci], channelDescriptors[ci] );
        idxs[ci].resize( channelKeypoints[ci].size() );
        for( size_t ki = 0; ki < channelKeypoints[ci].size(); ki++ ){idxs[ci][ki] = (int)ki;}
        std::sort( idxs[ci].begin(), idxs[ci].end(), KP_LessThan(channelKeypoints[ci]) );
        maxKeypointsCount = std::max( maxKeypointsCount, (int)channelKeypoints[ci].size());
    }
    
    std::vector<KeyPoint> outKeypoints;
    outKeypoints.reserve( keypoints.size() );

    int dSize = 128;
    Mat mergedDescriptors( maxKeypointsCount, 3*dSize, CV_32F );
    int mergedCount = 0;
    // cp - current channel position
    size_t cp[] = {0, 0, 0};
    while( cp[0] < channelKeypoints[0].size() &&
           cp[1] < channelKeypoints[1].size() &&
           cp[2] < channelKeypoints[2].size() )
    {
        const int maxInitIdx = std::max( 0, std::max( channelKeypoints[0][idxs[0][cp[0]]].class_id,
                                                      std::max( channelKeypoints[1][idxs[1][cp[1]]].class_id,
                                                                channelKeypoints[2][idxs[2][cp[2]]].class_id ) ) );
        std::cout<< maxInitIdx << std::endl;
        while( channelKeypoints[0][idxs[0][cp[0]]].class_id < maxInitIdx && cp[0] < channelKeypoints[0].size()-1 ) 
        { cp[0]++;}
        std::cout<< cp[0] << std::endl;
        while( channelKeypoints[1][idxs[1][cp[1]]].class_id < maxInitIdx && cp[1] < channelKeypoints[1].size() ) 
        { cp[1]++; }
        std::cout<< cp[1] << std::endl;
        while( channelKeypoints[2][idxs[2][cp[2]]].class_id < maxInitIdx && cp[2] < channelKeypoints[2].size() ) 
        { cp[2]++; }
        std::cout<< cp[2] << std::endl;
        if( cp[0] >= channelKeypoints[0].size() || 
            cp[1] >= channelKeypoints[1].size() || 
            cp[2] >= channelKeypoints[2].size() ){break;}

        if( channelKeypoints[0][idxs[0][cp[0]]].class_id == maxInitIdx &&
            channelKeypoints[1][idxs[1][cp[1]]].class_id == maxInitIdx &&
            channelKeypoints[2][idxs[2][cp[2]]].class_id == maxInitIdx )
        {
            
            outKeypoints.push_back( keypoints[maxInitIdx] );
            // merge descriptors
            for( int ci = 0; ci < N; ci++ )
            {
                Mat dst = mergedDescriptors(Range(mergedCount, mergedCount+1), Range(ci*dSize, (ci+1)*dSize));
                channelDescriptors[ci].row( idxs[ci][cp[ci]] ).copyTo( dst );
                std::cout<<channelDescriptors[ci].row( idxs[ci][cp[ci]] )<<std::endl;
                cp[ci]++;
            }
            mergedCount++;
        }
    }
    mergedDescriptors.rowRange(0, mergedCount).copyTo( descriptors );
    std::swap( outKeypoints, keypoints );
    std::cout<<descriptors<<std::endl;
    std::cout<<"H1"<<std::endl;
    //apply_hellinger(descriptors);
    std::cout<<"H2"<<std::endl;
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
        #ifdef D__EBUG
        for(int i=0;i<filenames.size();i++)
        {
            std::cout<<"Loading:"<<filenames.at(i)<<std::endl;
        }
        #endif
        return;
}

void work_directory(char *argv,std::vector<std::string> filenames)
{
    listFile(argv,filenames);

    std::clock_t start;
    double duration;

    start = std::clock();
    std::map<std::string,cv::Mat> file_descs;
    
    ASiftDetector detector(400,7,0.06,10,1.6);
    
    for(int i=0;i<filenames.size();i++)
    {
        std::string filename;
        filename+=argv;
        filename+="/";
        filename+=filenames.at(i);

        std::cout<<"Loading:"<<filename<<std::endl;

        cv::Mat img = imread(filename),mask,desc,output;
        std::vector<cv::KeyPoint> keypoints;

        apply_SIFT(img,keypoints,mask,desc,detector);

        //write_sift_binary(const_cast<char*>(filenames.at(i).c_str()),keypoints,desc);
        write_sift_ascii(const_cast<char*>(filenames.at(i).c_str()),keypoints,desc);
        file_descs[filenames.at(i)] = desc;

        imwrite(SplitFilename(filenames.at(i)), img);
    }
    /*
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
    */
}
