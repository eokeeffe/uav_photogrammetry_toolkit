#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
//c header files
#include <fcntl.h>
#include <errno.h>
//custom headers files
#include <vsfm_mat.h>
#include <vsfm_sift.h>

using namespace std;
/*
void write_match_file(std::string first_file,
    std::string second_file,
    std::vector<cv::DMatch>& matches)
{
    ofstream outputFile;
    std::string match_name = remove_extension(SplitFilename(first_file));
    // Open file in append mode if it exists.
    outputFile.open((match_name+".mat").c_str(), ofstream::out | ofstream::app | std::ifstream::binary);

    // Printing the two images matched and no. matches.
    outputFile << first_file
               << " " << second_file
               << " " << matches.size() << endl;
    // Print all keypoint indexes from image 1.
    for (int i = 0; i < matches.size(); i++)
    {
        outputFile << matches.at(i).queryIdx << " ";
    }
    outputFile << endl;
    // Print keypoint indexes matching those listed above.
    for (int i = 0; i < matches.size(); i++)
    {
        outputFile << matches.at(i).trainIdx << " ";
    }
    outputFile << endl;
    outputFile.close();
}
*/

int file_exist (char *filename)
{
  struct stat   buffer;
  return (stat (filename, &buffer) == 0);
}

std::vector<std::string> tokenize(std::string line,std::string delimiter)
{
    std::vector<std::string> tokenize;

    size_t pos = 0;
    while ((pos = line.find(delimiter)) != std::string::npos)
    {
        std::string token = line.substr(0, pos);
        //std::cout << token << std::endl;
        tokenize.push_back(token);
        line.erase(0, pos + delimiter.length());
    }

    return tokenize;
}

template<typename T>
T StringToNumber(const std::string& numberAsString)
{
    T valor;
    std::stringstream stream(numberAsString);
    stream >> valor;
    if (stream.fail())
    {
        std::runtime_error e(numberAsString);
        throw e;
    }
    return valor;
}

void write_match_file(std::string first_file,
    std::string second_file,
    std::vector<cv::DMatch>& matches)
{
    std::string match_name = remove_extension(SplitFilename(first_file));
    FILE *outputFile;

    if(!file_exist(const_cast<char*>((match_name+".mat").c_str())))
    {
        outputFile=fopen((match_name+".mat").c_str(),"wb+");
        char header[] = "#This file contains the pairwise feature matches between many images\n";
        char header1[]= "# path1 path2 number\n";
        char header2[]= "# feature index in image1\n";
        char header3[]= "# feature index in image2\n\n";
        fprintf(outputFile,"%s",header);
        fprintf(outputFile,"%s",header1);
        fprintf(outputFile,"%s",header2);
        fprintf(outputFile,"%s",header3);
    }
    else
    {
        //Open file in append mode if it exists.
        //ofstream outputFile;
        //outputFile.open((match_name+".mat").c_str(), ofstream::out | ofstream::app | std::ifstream::binary);
        outputFile=fopen((match_name+".mat").c_str(),"ab+");
    }

    // Printing the two images matched and no. matches.
    fprintf(outputFile,"%s %s %d\n",first_file.c_str(),second_file.c_str(),(int)matches.size());

    // Print all keypoint indexes from image 1.
    for (int i = 0; i < matches.size(); i++)
    {
        //outputFile << matches.at(i).queryIdx << " ";
        fprintf(outputFile,"%d ",matches.at(i).queryIdx);
    }
    fprintf(outputFile,"\n");
    // Print keypoint indexes matching those listed above.
    for (int i = 0; i < matches.size(); i++)
    {
        //outputFile << matches.at(i).trainIdx << " ";
        fprintf(outputFile,"%d ",matches.at(i).trainIdx);
    }
    fprintf(outputFile,"\n\n");
    fclose(outputFile);
}

void read_match_file(std::string filename,
    std::map<std::string,std::map<std::string,std::vector<cv::DMatch> > >& file_matches)
{
    //file_matches
    std::string match_name = remove_extension(SplitFilename(filename));

    if(file_exist(const_cast<char*>((match_name+".mat").c_str())))
    {
        std::string line ;
        std::map<std::string,std::vector<cv::DMatch> > compare_matches;

        std::ifstream infile((match_name+".mat").c_str());
        if ( infile.is_open() )
        {
            std::string file1,file2;
            int num_matches=0;
            std::vector<cv::DMatch> matches;
            bool first=true;
            while ( getline( infile , line ) )
            {
    	           if (line.find(std::string("#")) != std::string::npos)
                   {//continue if comment symbol appears
                       continue;
                   }
                   if(count_if(line.begin(), line.end(), (int(*)(int))isalnum) == line.size())
                   {//filename line
                       std::vector<std::string> tokens = tokenize(line," ");
                       file1 = tokens.at(0);
                       file2 = tokens.at(1);
                       num_matches = StringToNumber<int>(tokens.at(2));
                   }
                   else if(count_if(line.begin(), line.end(), (int(*)(int))isdigit) == line.size())
                   {//feature match line
                       std::vector<std::string> tokens = tokenize(line," ");
                       if(first)
                       {
                           for(int i=0;i<tokens.size();i++)
                           {
                               matches.at(i).queryIdx = StringToNumber<int>(tokens.at(i));
                           }
                       }
                       else
                       {
                           for(int i=0;i<tokens.size();i++)
                           {
                               matches.at(i).trainIdx = StringToNumber<int>(tokens.at(i));
                           }
                       }
                       //enter the comparison filenames and corresponding matches
                       //to the map
                       if(file1 == filename)
                       {
                           std::vector<cv::DMatch> copied;
                           std::copy(matches.begin(), matches.end(), copied.begin());
                           compare_matches[file2] = copied;
                           matches.clear();
                       }
                       else
                       {
                           std::vector<cv::DMatch> copied;
                           std::copy(matches.begin(), matches.end(), copied.begin());
                           compare_matches[file1] = copied;
                           matches.clear();
                       }
                   }
            }
            file_matches[filename] = compare_matches;
            infile.close( );
        }
    //all done
    }
    return;
}
