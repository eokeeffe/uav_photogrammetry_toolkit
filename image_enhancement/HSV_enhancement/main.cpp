/*ALL the necessary header files*/
/*If you want to enhance the Videos then instead of loading frames from an
Image you may load frames from a  camera.. by using the capture structures*/
 #include <stdio.h>
 #include <math.h>

 #include <opencv2/core/core.hpp>
 #include <opencv2/highgui.hpp>
 #include <stdio.h>
 
int main(int argc,char *argv[])
{

int i,j,k;
int heightc,widthc,stepc,channelsc;

int temp=0;
int units=0;
uchar *data,*datac;
i=j=k=0;

IplImage *frame=cvLoadImage(argv[1],1);
IplImage *convert=cvCreateImage( cvGetSize(frame), 8, 3 );
IplImage *result=cvCreateImage( cvGetSize(frame), 8, 3 );
printf("By what value Do you want to increase the strenght of the color..? ");
scanf("%d", &units);
if(frame==NULL ) {
puts("unable to load the frame");exit(0);}

cvNamedWindow("original",CV_WINDOW_AUTOSIZE);
 cvNamedWindow("Result",CV_WINDOW_AUTOSIZE);

heightc = convert->height;
widthc = convert->width;

stepc=convert->widthStep;
channelsc=convert->nChannels;
datac = (uchar *)convert->imageData;

cvCvtColor(frame,convert,CV_BGR2HSV);

for(i=0;i< (heightc);i++) for(j=0;j<(widthc);j++)
{/*Here datac means data of the HSV Image*/
/*Here i want to Increase the saturation or the strength of the Colors in the Image and
then I would be able to perform a good color detection*/

temp=datac[i*stepc+j*channelsc+1]+units;/*increas the saturaion component is the second arrray.*/

/*Here there is a small problem...when you add a value to the data and if it exceeds 255
it starts all over again from zero and hence some of the pixels might go to zero.
So to stop this we need to include this loop i would not try to explain the loop but
please try and follow it is easy to do so..*/
if(temp>255) datac[i*stepc+j*channelsc+1]=255;
else datac[i*stepc+j*channelsc+1]=temp;/*you may
please remove and see what is happening if the if else loop is not there*/}

cvCvtColor(convert, result, CV_HSV2BGR);
//cvShowImage("Result", result);
//cvShowImage("original", frame);

 cvSaveImage("enhanced.jpg",result);
 cvSaveImage("orig.jpg",frame);
 //cvWaitKey(0);
 cvDestroyWindow("original");
 cvDestroyWindow("Result");
 return 0;
 }
