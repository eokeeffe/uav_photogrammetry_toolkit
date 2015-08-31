#include <iostream>
#include <cstdio>
#include <ctime>

#include <vector>
#include <string>
#include <vsfm_sift.h>

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
    /*
        Write the ascii SIFT format described in
        http://ccwu.me/vsfm/doc.html#customize
    */
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
    /*
        Write the SIFT format described in
        http://ccwu.me/vsfm/doc.html#customize
    */
    FILE *sift1;
    std::string sift_file = SplitFilename(remove_extension(filename));
    sift_file += ".sift";
    sift1 = fopen(sift_file.c_str(),"wb");

    int name = ('S' + ('I'<<8) + ('F'<<16) + ('T'<<24));
    int version = ('V' + ('4'<<8) + ('.'<<16)+ ('0'<<24));//Without colour
    //int version = ('V' + ('5'<<8) + ('.'<<16)+ ('0'<<24));//With colour
    int npoint = keypoints.size();

    int header[5] = {name, version, npoint, 5, 128};
    fwrite(header, sizeof(int), 5, sift1);

    //Location
    for (int i = 0; i < keypoints.size(); ++i)
    {
        float x = keypoints.at(i).pt.x;
        float y = keypoints.at(i).pt.y;
        float colour = 1;//if this values changes, pleaes use the version 5.0 SIFT
        float scale = keypoints.at(i).size;
        float orientation = keypoints.at(i).angle;
        float loc[5] = {x, y, colour, scale,orientation};
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

void read_sift_binary(std::string file,std::vector<cv::KeyPoint>& keypoints,cv::Mat& desc)
{
    /*
        Following the same format as the VSFM sift format

        [Header] = int[5] = {name, version, npoint, 5, 128};
        name = ('S'+ ('I'<<8)+('F'<<16)+('T'<<24));
        version = ('V'+('4'<<8)+('.'<<16)+('0'<<24)); or ('V'+('5'<<8)+('.'<<16)+('0'<<24)) if containing color info
        npoint = number of features.

        [Location Data]  is a npoint x 5 float matrix and each row  is [x, y, color, scale, orientation].
        Write color by casting the float to unsigned char[4]
        scale & orientation are only used for visualization, so you can simply write 0 for them

        * Sort features in the order of decreasing importance, since VisualSFM may use only part of those features.
        * VisualSFM sorts the features in the order of decreasing scales.

        [Descriptor Data] is a npoint x 128 unsigned char matrix. Note the feature descriptors are normalized to 512.

        [EOF]  int eof_marker = (0xff+('E'<<8)+('O'<<16)+('F'<<24));
    */
    int num_features,feature_dimensions;
    FILE* f = fopen(file.c_str(), "r");
    int name,version,npoint,size_of_loc_data,colour;

    fscanf(f, "%d %d %d %d %d", &name,&version,
        &num_features,&size_of_loc_data,&feature_dimensions);
    //fscanf(f, "%d %f", &num_features, &feature_dimensions);
    cv::Mat descriptors = cv::Mat(num_features, 128, CV_32F);
	cv::KeyPoint key;
    float *pter;

    for(int i = 0; i < num_features; i++)
	{
		fscanf(f, "%f %f %d %f %f", &key.pt.x, &key.pt.y, &colour, &key.size, &key.angle);

		keypoints.push_back(key);

		pter = descriptors.ptr<float>(i);

		for(int j = 0; j < feature_dimensions; j++)
		{
			fscanf(f, "%f", &pter[j]);
		}
	}
    desc = descriptors;
    fclose(f);
}

void read_sift_ascii(char *filename,std::vector<cv::KeyPoint>& keypoints,cv::Mat& desc)
{
    /*
        Not implemented yet
    */
    return;
}
