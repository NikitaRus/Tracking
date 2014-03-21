#include "header.h"

int thres = 0, thres_max = 255, thres_type = 0;

Mat frameb;
Mat frameh;
static void onMouse(int event, int x, int y, int flags) 
{ 
	if(event != EVENT_LBUTTONDOWN) 
		return; 


	point = Point(x, y); 

	//Tracking
	if( point.x - rsize > 0 &&
		point.y - rsize > 0 && 
		point.x + rsize < width && 
		point.y + rsize < height) 
	{
		roi = Rect(point.x - rsize, point.y - rsize, rsize * 2, rsize * 2); 
	}

	//Llenar array de scalars +
	rangePicker.clear();
	rangePicker.push_back(Point(point.x - 1, point.y));
	rangePicker.push_back(Point(point.x + 1, point.y));
	rangePicker.push_back(Point(point.x, point.y));
	rangePicker.push_back(Point(point.x, point.y - 1));
	rangePicker.push_back(Point(point.x, point.y + 1));

	/*Mat sobject[3];
	split(tobject, sobject);

	for(int x = 1; x < tobject.cols; x+=3)
	{
		for(int y = 1; y < tobject.rows; y+= 3)
		{
			pointColors.push_back(Scalar(sobject[0].at<uchar>(Point(x, y)),	sobject[1].at<uchar>(Point(x, y)), sobject[2].at<uchar>(Point(x, y))));
		}
	}

	int repeated = 0;
	for(int x = 0; x != pointColors.size(); x++)
	{
		for(vector<Scalar>::iterator it2 = pointColors.begin(); it2 != pointColors.end() - repeated; it2++)
		{
			if(*it == *it2)
			{
				pointColors.erase(it2)
				it2--;
			}
		}
	}

	for(int r = pointColors.size(); r >= repeated; r--)
	{
	}*/

	int nColor = 0;
	pointColor = Scalar(0,0,0);

	for(vector<Point>::iterator cColor = rangePicker.begin(); cColor != rangePicker.end(); ++cColor)
	{
		Point cPoint = *cColor;
		pointColors[nColor] = Scalar(s[0].at<uchar>(cPoint), s[1].at<uchar>(cPoint), s[2].at<uchar>(cPoint));
		nColor++;

		//Promedio de colores (aproximacion a color similar)
		pointColor[0] += s[0].at<uchar>(cPoint);
		pointColor[1] += s[1].at<uchar>(cPoint);
		pointColor[2] += s[2].at<uchar>(cPoint);
	}

	for(int i =0; i <= 2; i++)
		pointColor[i] = pointColor[i] / 5;


	//Deteccion por promedio del color
	/*for (int c = 1; c < 5; c++) { 
	pcH += pointColors[c](0); 
	pcS += pointColors[c](1); 
	pcV += pointColors[c](2); 
	}

	pcH = pcH / 4;
	pcS = pcS / 4;
	pcV = pcV / 4;*/
} 

int main() 

{ 
	//With video
	string filename = "C:\\Users\\Gira\\Downloads\\MVI_5450.AVI"; 
	//With stream
	VideoCapture capture(filename); 

	Mat frame;// = Mat(160, 120, CV_8UC1); 

	namedWindow(video, CV_WINDOW_AUTOSIZE); 
	namedWindow(controls, 0);

	//createTrackbar(wname[0], controls, &type, 3);
	createTrackbar(size_str, controls, &rsize_aux, 200); 
	createTrackbar(precision_str, controls, &thl, 50); 
	createTrackbar(draw_str, controls, &draw, 1);
	//createTrackbar(wname[3], controls, &minHessian, 2000); 

	//createTrackbar(wname[4], controls, &loThreTyp, hiThreTyp);
	//createTrackbar(wname[5], controls, &loThreVal, hiThreVal); 

	//createTrackbar(wname[6], controls, &thres, 255); 
	//createTrackbar(wname[7], controls, &thres_max, 255); 
	//createTrackbar(wname[8], controls, &thres_type, 4); 

	setMouseCallback(video, *(MouseCallback) onMouse); 

	//Old style
	/*CvTracks tracks;
	CvCapture *capture = cvCaptureFromCAM(0);
	cvGrabFrame(capture);
	IplImage *img = cvRetrieveFrame(capture);

	CvSize imgSize = cvGetSize(img);

	IplImage *cvframe = cvCreateImage(imgSize, img->depth, img->nChannels);
	IplConvKernel* morphKernel = cvCreateStructuringElementEx(5, 5, 1, 1, CV_SHAPE_RECT, NULL);

	IplImage *cvframeg;


	unsigned int blobNumber = 0;*/

	while(1) 
	{ 
		/*IplImage *img = cvRetrieveFrame(capture);

		cvConvertScale(img, cvframe, 1, 0);
		//cvCvtColor(cvframe, cvframeg, CV_RGB2GRAY);

		IplImage *segmentated = cvCreateImage(imgSize, 8, 1);	

		for (int x=roi.x; x < roi.x + roi.width; x++)
			for (int y=roi.y; y < roi.y  + roi.height; y++)
			{
				CvScalar c = cvGet2D(cvframe, x, y);

				double b = ((double)c.val[0])/255.;
				double g = ((double)c.val[1])/255.;
				double r = ((double)c.val[2])/255.;
				unsigned char f = 255*((r>0.2+g)&&(r>0.2+b));

				cvSet2D(segmentated, x, y, CV_RGB(f, f, f));
			}

			cvMorphologyEx(segmentated, segmentated, NULL, morphKernel, CV_MOP_OPEN, 1);

			//cvShowImage("segmentated", segmentated);

			IplImage *labelImg = cvCreateImage(cvGetSize(cvframe), IPL_DEPTH_LABEL, 1);

			CvBlobs blobs;
			unsigned int result = cvLabel(segmentated, labelImg, blobs);
			cvFilterByArea(blobs, 500, 1000000);
			cvRenderBlobs(labelImg, blobs, cvframe, cvframe, CV_BLOB_RENDER_BOUNDING_BOX);
			cvUpdateTracks(blobs, tracks, 200., 5);
			cvRenderTracks(tracks, cvframe, cvframe, CV_TRACK_RENDER_ID|CV_TRACK_RENDER_BOUNDING_BOX);
			
			//Conversion a mat
			frame = cvarrToMat(cvframe);
			*/
			capture >> frame; 
			width = frame.cols;
			height = frame.rows;

			tracking(frame);//frameProcessing(0, 0, frame); 
			imshow(video, frame); 
			waitKey(1000); //16ms = 60 fps

			//if(waitKey(1) == 63) { draw = 1; } else { draw = 0; }
	} 

	//waitKey(0); 

	return 0; 
} 