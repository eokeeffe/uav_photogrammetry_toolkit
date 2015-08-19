/*
*   A fast semi-inverse approach to detect and
*   remove the haze from a single image
*/
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <vector>
#include <queue>
#include <limits>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define DEBUG
#define DEBUG_DIFFERENCE
#define DEBUG_ALE

using namespace std;
using namespace cv;

//start of utilities
//minMaxLoc was acting the magot, kept giving assertion errors
void maxLocs(const Mat& src, queue<Point>& dst, size_t size)
{
    uchar maxValue = numeric_limits<uchar>::min();
    uchar* srcData = reinterpret_cast<uchar*>(src.data);

    for(int i = 0; i < src.rows; i++)
    {
        for(int j = 0; j < src.cols; j++)
        {
            //cout<<i<<","<<j<<endl;
            if(srcData[i*src.rows + j] > maxValue)
            {
                maxValue = srcData[i*src.rows + j];

                dst.push(Point(j, i));

                // pop the smaller one off the end if we reach the size threshold.
                if(dst.size() > size)
                {
                    dst.pop();
                }
            }
        }
    }
}

double* getRange(cv::Mat& input)
{
    /*
        Max and min value of matrix
        More robust
    */
    double *vals = new double[2];
    cv::Mat gray=input.clone();
    GaussianBlur(gray,gray,Size(5,5),1,1,0);
    minMaxLoc(gray,&vals[1],&vals[2]);
    return vals;
}

cv::Point getMax(cv::Mat& input)
{
    /*
        Max and min value of matrix
        More robust
    */
    cv::Mat gray;
    cvtColor(input,gray,CV_BGR2GRAY);
    GaussianBlur(gray,gray,Size(11,11),1,1,0);
    double *vals = new double[2];
    cv::Point min,max;
    minMaxLoc(gray,&vals[1],&vals[2],&min,&max);
    return max;
}

template <class ForwardIterator,class Generator>
void generate(ForwardIterator first,ForwardIterator last,Generator g)
{
    while(first!=last)
    {
        *first++ = g();
    }
}

struct Generator
{
    Generator():ci(0.0){}
    Generator(float c_init):c(c_init){}
    float operator()(){ci+=c;return ci;}
    float operator()(float c_init){c=c_init;return c;}
    float ci=0.0;
    float c=0.0;
};

//end of Utilities
cv::Mat inverseImage(cv::Mat& image)
{
    cv::Mat output = cv::Mat(image.rows,image.cols,image.type());
    vector<cv::Mat> channels(3);
    split(image,channels);
    //cout<< channels[0] <<endl;
    for(int i=0;i<image.rows;i++)
    {
        for(int j=0;j<image.cols;j++)
        {
            float v1=channels[0].at<float>(i,j),
                  v2=channels[1].at<float>(i,j),
                  v3=channels[2].at<float>(i,j);
            float b=saturate_cast<float>(1.0-channels[0].at<float>(i,j)),
            g=saturate_cast<float>(1.0-channels[1].at<float>(i,j)),
            r=saturate_cast<float>(1.0-channels[2].at<float>(i,j));
            channels[0].at<float>(i,j) = max(v1,b);
            channels[1].at<float>(i,j) = max(v2,g);
            channels[2].at<float>(i,j) = max(v3,r);
        }
    }
    #ifdef DEBUG_INV
        double *a=getRange(channels[0]),
        *b=getRange(channels[1]),
        *c=getRange(channels[2]);
        cout<<fixed<<setprecision(10)<<a[0]<<","<<a[1]<<endl;
        cout<<fixed<<setprecision(10)<<b[0]<<","<<b[1]<<endl;
        cout<<fixed<<setprecision(10)<<c[0]<<","<<c[1]<<endl;
    #endif
    merge(channels,output);
    cv::normalize(output, output, 0.0, 1.0, NORM_MINMAX, CV_32FC3);
    return output;
}

cv::Mat inverseImageMaskd(cv::Mat& image,cv::Mat& difference,int rho,double xi)
{
    cv::Mat output = cv::Mat(image.rows,image.cols,image.type());
    vector<cv::Mat> channels(3);
    split(image,channels);
    for(int i=0;i<image.rows;i++)
    {
        for(int j=0;j<image.cols;j++)
        {
            if(difference.at<float>(i,j)>rho)
            {
                float v1=channels[0].at<float>(i,j),
                      v2=channels[1].at<float>(i,j),
                      v3=channels[2].at<float>(i,j);
                float b=saturate_cast<float>(1.0-channels[0].at<float>(i,j)),
                g=saturate_cast<float>(1.0-channels[1].at<float>(i,j)),
                r=saturate_cast<float>(1.0-channels[2].at<float>(i,j));
                float diff = difference.at<float>(i,j);

                channels[0].at<float>(i,j) = (diff*xi)+max(v1,b);
                channels[1].at<float>(i,j) = max(v2,g);
                channels[2].at<float>(i,j) = max(v3,r);
            }
            else if(difference.at<float>(i,j)<rho)
            {
                float v1=channels[0].at<float>(i,j),
                      v2=channels[1].at<float>(i,j),
                      v3=channels[2].at<float>(i,j);
                float b=saturate_cast<float>(1.0-channels[0].at<float>(i,j)),
                g=saturate_cast<float>(1.0-channels[1].at<float>(i,j)),
                r=saturate_cast<float>(1.0-channels[2].at<float>(i,j));

                channels[0].at<float>(i,j) = max(v1,b);
                channels[1].at<float>(i,j) = max(v2,g);
                channels[2].at<float>(i,j) = max(v3,r);
            }
        }
    }
    //output = xi+output;
    merge(channels,output);
    #ifdef DEBUG_INV_MASK
        double *a=getRange(channels[0]),
        *b=getRange(channels[1]),
        *c=getRange(channels[2]);
        cout<<fixed<<setprecision(10)<<a[0]<<","<<a[1]<<endl;
        cout<<fixed<<setprecision(10)<<b[0]<<","<<b[1]<<endl;
        cout<<fixed<<setprecision(10)<<c[0]<<","<<c[1]<<endl;
    #endif
    cv::normalize(output, output, 0.0, 1.0, NORM_MINMAX, CV_32FC3);
    return output;
}

cv::Mat inverseImageMask(cv::Mat& image,cv::Mat& difference,int rho,double xi)
{
    cv::Mat output = cv::Mat(image.rows,image.cols,image.type());
    vector<cv::Mat> channels(3);
    float *input=(float*)(image.data);
    split(image,channels);

    for(int i=0;i<image.rows;i++)
    {
        for(int j=0;j<image.cols;j++)
        {
            if(difference.at<float>(i,j)>=rho)
            {
                channels[0].at<float>(i,j) = 255.0;
                channels[1].at<float>(i,j) = 0;
                channels[2].at<float>(i,j) = 0;
            }
        }
    }
    //output = xi+output;
    merge(channels,output);
    #ifdef DEBUG_INV_MASK
        imwrite("before_weight.jpg",output);
    #endif
    cv::normalize(output, output, 0.0, 1.0, NORM_MINMAX, CV_32FC3);
    addWeighted(output,0.5,image,0.5,0,output);
    return output;
}

cv::Mat haze_difference(cv::Mat& image,cv::Mat& inverse)
{
    cv::Mat hsv1,hsv2;
    vector<cv::Mat> channels(3),channels2(3);

    cvtColor(image,hsv1,CV_BGR2HSV);
    cvtColor(inverse,hsv2,CV_BGR2HSV);

    split(hsv1,channels);
    split(hsv2,channels2);

    cv::Mat h1=Mat::zeros(image.rows,image.cols,image.type());
    cv::Mat h2=Mat::zeros(image.rows,image.cols,image.type());
    cv::Mat diff=Mat::zeros(image.rows,image.cols,image.type());
    /*
        L*A*B to L*C*H
        can be accomplished by
        L = L
        C = sqrt(pow(a,2)+pow(b,2))
        H = arctan(b,a)
    */
    #ifdef DEBUG_DIFFERENCE
        cv::normalize(channels[0], channels[0], 0, 255, NORM_MINMAX, CV_32FC3);
        cv::normalize(channels2[0], channels2[0], 0, 255, NORM_MINMAX, CV_32FC3);
        imwrite("h1.jpg",channels[0]);
        imwrite("h2.jpg",channels[0]);
    #endif

    //subtract(h2,h1,diff,noArray(),CV_32FC1);
    absdiff(channels2[0],channels[0],diff);

    return diff;
}

float detect_haze(cv::Mat difference,int rho)
{
    /*
        return percentage haze in image
    */
    int count=0;
    for(int i=0;i<difference.rows;i++)
    {
        for(int j=0;j<difference.cols;j++)
        {
            if(difference.at<float>(i,j) < rho)
            {
                count++;
            }
        }
    }

    return (float)count/(difference.rows*difference.cols)*100;
}

cv::Point airlight_estimation(cv::Mat& input,cv::Mat& mask)
{
    /*
        get the brightest pixel value and return it
    */
    cv::Point max = getMax(input);

    #ifdef DEBUG_ALE
        Vec3f ale = input.at<cv::Vec3f>(max);
        cout<<max<<endl;
        cout<<"ale:"<<ale<<endl;
        cv::Mat dst=Mat::ones(input.rows,input.cols,input.type());
        cv::Mat temp=input.clone();
        cv::normalize(temp, temp, 0, 255, NORM_MINMAX, CV_32FC3);
        circle(temp,max,25,(255,0,0),2);
        for(int i=0;i<dst.rows;i++)
        {
            for(int j=0;j<dst.cols;j++){dst.at<cv::Vec3f>(i,j)=ale;}
        }
        imwrite("ale.jpg",dst);
        imwrite("ale_location.jpg",temp);
    #endif
    return max;
}

cv::Mat transmissionMap(cv::Mat& input,cv::Point max)
{
    double w=0.75;
    float ale= input.at<float>(max);
    Mat transmission=Mat::zeros(input.rows,input.cols,input.type());
    for(int i=0;i<transmission.rows;i++)
    {
        for(int j=0;j<transmission.cols;j++)
        {
            transmission.at<float>(i,j)=(1-w*input.at<float>(i,j)/ale)*255.0;
        }
    }
    cv::normalize(transmission, transmission, 0.0, 1.0, NORM_MINMAX, CV_32FC3);
    return transmission;
}

cv::Mat dehaze(cv::Mat& image,cv::Mat& difference,cv::Point ale,int k,int rho,double xi)
{
     cv::Mat output = cv::Mat(image.rows,image.cols,image.type());
     float c = 1.0/k;
     vector<float> ci(k);
     std::generate(ci.begin(),ci.end(),Generator(c));

     vector<cv::Mat> layers;
     vector<cv::Mat> mask_layers;
     vector<cv::Mat> diff_layers;
     Vec3f ale_temp= image.at<cv::Vec3f>(ale);

     for(int i=0;i<ci.size();i++)
     {
         cv::Mat layer=image.clone();

         layer-=(ci[i]*ale_temp);
         cv::Mat inverse = inverseImage(layer);
         cv::Mat diff = haze_difference(layer,inverse);
         cv::Mat mask = inverseImageMask(inverse,diff,rho,xi);
         //ale = airlight_estimation(layer,mask);
         //ale_temp= image.at<float>(ale);

         #ifdef DEBUG
             cv::Mat inv,m,l;
             cv::normalize(layer, l, 0, 255, NORM_MINMAX, CV_32FC3);
             cv::normalize(inverse, inv, 0, 255, NORM_MINMAX, CV_32FC3);
             cv::normalize(mask, m, 0, 255, NORM_MINMAX, CV_32FC3);
             imwrite("layer_"+to_string(i)+".jpg",l);
             imwrite("layer_inv_"+to_string(i)+".jpg",inv);
             imwrite("layer_mask_"+to_string(i)+".jpg",m);
         #endif
         layers.push_back(layer);
         mask_layers.push_back(mask);
         diff_layers.push_back(diff);
     }
     for(int count=layers.size()-1;count>-1;count--)
     {
        cv::Mat temp=mask_layers[count];
        //addWeighted(layers[count],1.0,mask_layers[count],1.0,0.0,temp);
        float weight=(float)count/10.0;
        addWeighted(output,weight,temp,1.0-weight,0.0,output);
     }
     return output;
}

int main(int argc,char *argv[])
{
    //inputs
    cv::Mat input = imread(argv[1]);
    int rho = atoi(argv[2]);
    double xi = atof(argv[3]);
    double k = atoi(argv[4]);

    cout<<rho<<","<<xi<<","<<k<<endl;

    cv::Mat input_float;
    input.convertTo(input_float,CV_32F);
    cv::normalize(input_float, input_float, 0, 1.0, NORM_MINMAX, CV_32FC3);

    cv::Mat inverse = inverseImage(input_float);
    cv::Mat difference = haze_difference(input_float,inverse);
    float haze = detect_haze(difference,rho);
    cv::Mat mask = inverseImageMask(inverse,difference,rho,xi);
    cv::Point ale = airlight_estimation(input_float,mask);
    cv::Mat dehazed = dehaze(input_float,difference,ale,k,rho,xi);
    cv::Mat transmission = transmissionMap(dehazed,ale);

    cout<<"Haze@:"<<haze<<"%"<<endl;

    cv::normalize(input_float, input_float, 0, 255, NORM_MINMAX, CV_32FC3);
    imwrite("reg.jpg",input);
    cv::normalize(inverse, inverse, 0, 255, NORM_MINMAX, CV_32FC3);
    imwrite("inverse.jpg",inverse);
    cv::normalize(difference, difference, 0, 255, NORM_MINMAX, CV_32FC3);
    imwrite("difference.jpg",difference);
    cv::normalize(mask, mask, 0, 255, NORM_MINMAX, CV_32FC3);
    imwrite("mask.jpg",mask);
    cv::normalize(dehazed, dehazed, 0, 255, NORM_MINMAX, CV_32FC3);
    imwrite("dehazed.jpg",dehazed);
    imwrite("transmission.jpg",transmission);

    return 0;
}
