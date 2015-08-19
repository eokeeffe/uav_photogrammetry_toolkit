#include <iostream>
#include <fstream>
#include <string>
#include <map>
//custom headers first
#include <vsfm_mat.h>
#include <vsfm_sift.h>

using namespace std;

class MatchInformation
{
public:
    //constructors
    MatchInformation(){}
    MatchInformation(std::string source)
    {
        this->source_name = source;
    }
    //
    void append(std::string filename,std::vector<cv::DMatch> matches)
    {
        this->matches[filename] = matches;
    }
    void erase_match(std::string filename)
    {
        this->matches.erase(filename);
    }
    //
    std::vector<cv::DMatch> getMatches(std::string filename)
    {
        return this->matches[filename];
    }
    std::map<std::string, std::vector<cv::DMatch>> getMap()
    {
        return this->matches;
    }
    bool haveMatches()
    {
        bool haveMatch=this->matches.empty();
        if(haveMatch){return false;}
        return true;
    }

private:
    std::string source_name;
    std::map<std::string, std::vector<cv::DMatch>> matches;
};


void write_match_file(std::string first_file,
    std::string second_file,
    std::vector<cv::DMatch>& matches)
{
    ofstream outputFile;
    first_file = remove_extension(SplitFilename(first_file));
    outputFile.open((first_file+".mat").c_str(), ofstream::out | ofstream::app | std::ifstream::binary);      // Open file in append mode if it exists.

    for(int i=0;i<matches.size();i++)
    {
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
    }
    outputFile << endl;
    outputFile.close();
}
/*
void read_first_line(std::ifstream& is,
    std::string& filename1,std::string& filename2,int& number_of_matches)
{
    //Format:
    //filename1 filename2 number_of_matches

    std::string item,buf;
    std::vector<std::string> tokens;
    getline(is, item);
    std::stringstream ss(item);
    while(ss>>buf)
    {
        tokens.push_back(buf);
    }
    filename1 = tokens.at(0);
    filename2 = tokents.at(1);
    number_of_matches = std::stoi (tokens.at(2),nullptr,10);
}

void read_second_third_lines(std::ifstream& is,
std::vector<int>& first_locations,
std::vector<int>& second_locations)
{
    //Format:
    //filename1 locations, length of number_of_matches
    //filename2 locations, length of number_of_matches

    std::string item,buf;
    getline(is, item);//read second line
    std::stringstream ss(item);
    while(ss>>buf)
    {
        //tokens.push_back(buf);
        first_locations.push_back(std::stoi (buf,nullptr,10));
    }

    getline(is, item);//read the third line
    ss(item);
    while(ss>>buf)
    {
        //tokens.push_back(buf);
        second_locations.push_back(std::stoi (buf,nullptr,10));
    }
}
*/
void read_match_file(std::string filename,
    std::vector<std::string,std::vector<cv::DMatch>>& matches)
{
    /*
    match_name = remove_extension(SplitFilename(filename));

    ifstream is((filename+".mat").c_str());
	if(!is.is_open()){return 0;}
    while(is)
    {
        int matches
        read_first_line(is,filename1,filename2,matches);
        read_second_third_lines();
        matches.append();
    }
    is.close();
    */
}
