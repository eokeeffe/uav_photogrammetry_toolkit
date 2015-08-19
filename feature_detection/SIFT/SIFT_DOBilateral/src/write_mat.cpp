#include <iostream>
#include <fstream>
#include <string>
//custom headers first
#include <write_mat.h>
#include <write_sift.h>

using namespace std;

void write_match_file(std::vector<std::string> filenames,
    std::vector<cv::DMatch>& matches)
{
    ofstream outputFile;

    std::string match_name=filenames.at(0);
    std::string second_image_filename=filenames.at(1);

    match_name = remove_extension(SplitFilename(match_name));

    outputFile.open((match_name+".mat").c_str(), ofstream::out | ofstream::app);      // Open file in append mode if it exists.

    /* Printing the two images matched and no. matches. */
    outputFile << match_name
               << " " << second_image_filename
               << " " << matches.size() << endl;

    /* Print all keypoint indexes from image 1. */

    for (int i = 0; i < matches.size(); i++){
        outputFile << matches.at(i).queryIdx << " ";
    }
    outputFile << endl;

    /* Print keypoint indexes matching those listed above. */

    for (int i = 0; i < matches.size(); i++) {
        outputFile << matches.at(i).trainIdx << " ";
    }
    outputFile << endl;
    outputFile.close();
}
