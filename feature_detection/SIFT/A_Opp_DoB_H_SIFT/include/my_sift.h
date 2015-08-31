#include <string>
#include <ASIFT.h>
const std::string keys =
    "{help h usage ? |      | print this message              }"
    "{@image1        |      | image1 for compare              }"
    "{@image2        |      | image2 for compare              }"
    "{@directory     |      | directory of images to read     }"
    "{@list          |      | file with list of images to read}"
    "{@match         |      | match directory                 }"
    ;

bool has_suffix(const std::string& s, const std::string& suffix);
void listFile(char *argv,std::vector<std::string>& filenames);
void work_directory(char *argv,std::vector<std::string> filenames);

void apply_hellinger(cv::Mat& desc);
static void convertBGRImageToOpponentColorSpace( const cv::Mat& bgrImage,std::vector<cv::Mat>& opponentChannels);
void apply_SIFT(cv::Mat& img,
    std::vector<cv::KeyPoint> keypoints,
    cv::Mat& mask,cv::Mat& descriptors,
    ASiftDetector& detector);
