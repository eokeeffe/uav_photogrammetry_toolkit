#include <iostream>
#include <cstdio>
#include <ctime>

#include <vector>
#include <string>
#include <write_sift.h>

std::string SplitFilename (const std::string& str) {
  //std::cout << "Splitting: " << str << '\n';
  unsigned found = str.find_last_of("/\\");
  //std::cout << " path: " << str.substr(0,found) << '\n';
  //std::cout << " file: " <<  << '\n';
  return str.substr(found+1);
}

std::string remove_extension(const std::string& filename) {
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == std::string::npos) return filename;
    return filename.substr(0, lastdot);
}

void write_sift_ascii(char *filename,std::vector<cv::KeyPoint>& keypoints,cv::Mat& desc)
{
    int n=keypoints.size();
    int d=desc.cols;

    std::string sift_file = SplitFilename(remove_extension(filename));
    sift_file += ".sift";

    std::cout<<"SIFT FIle:"<<sift_file<<std::endl;
    FILE *file;
    file=fopen(sift_file.c_str(),"w+");

    int name = ('S' + ('I'<<8) + ('F'<<16) + ('T'<<24));
    int version = ('V' + ('4'<<8) + ('.'<<16)+ ('0'<<24));//Without color
    //int version = ('V' + ('5'<<8) + ('.'<<16)+ ('0'<<24));//With coloer
    int npoint = keypoints.size();

    int header[5] = {name, version, npoint, 5, 128};
    fwrite(header, sizeof(int), 5, file);

    for(int i = 0; i < n; i++ )
    {
        cv::KeyPoint kpt = keypoints.at(i);
        //std::cout<<kpt.pt.x<<","<<kpt.pt.y<<","<<scl<<","<<kpt.angle<<std::endl;
        fprintf( file, "%f %f 1 %f %f", kpt.pt.y, kpt.pt.x,
           kpt.size, kpt.angle );
        for(int j = 0; j < d; j++ )
        {
               // write 20 descriptor values per line
               if( j % 20 == 0 )
               {
                   fprintf( file, "\n" );
               }
               //(x,y) is computed as cv::Mat (cols,rows)
               //mat.at(i, j) = mat.at(row, col) = mat.at(y, x)
               fprintf( file, " %d",desc.at<int>(j,i));
        }
        fprintf( file, "\n" );
    }
    fclose(file);
    return;
}

void write_sift_binary(char *filename,std::vector<cv::KeyPoint>& keypoints,cv::Mat& desc)
{
    FILE *sift1;
    std::string sift_file = SplitFilename(remove_extension(filename));
    sift_file += ".sift";
    sift1 = fopen(sift_file.c_str(),"wb");

    int name = ('S' + ('I'<<8) + ('F'<<16) + ('T'<<24));
    int version = ('V' + ('4'<<8) + ('.'<<16)+ ('0'<<24));//Without color
    //int version = ('V' + ('5'<<8) + ('.'<<16)+ ('0'<<24));//With coloer
    int npoint = keypoints.size();

    int header[5] = {name, version, npoint, 5, 128};
    fwrite(header, sizeof(int), 5, sift1);

    //Location
    for (int i = 0; i < keypoints.size(); ++i)
    {
        float x = keypoints.at(i).pt.x;
        float y = keypoints.at(i).pt.y;
        //Color SOON
        float col = 1;
        float scale = keypoints.at(i).size;
        float orientation = keypoints.at(i).angle;
        float loc[5] = {x, y, 1, scale,orientation};
        fwrite(loc, sizeof(float), 5, sift1);
    }

    for (int i = 0; i < desc.rows; ++i)
    {
        const uchar* row = desc.ptr<uchar>(i);
        fwrite(&row, sizeof(uchar[128]), 1,sift1);
    }

    int eof_marker = (0xff+('E'<<8)+('O'<<16)+('F'<<24));
    fwrite(&eof_marker, sizeof(int), 1, sift1);
    fclose(sift1);
    return;
}
