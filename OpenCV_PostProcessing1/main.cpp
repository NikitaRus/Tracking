//#include "class.h"
#include <opencv.hpp>

//mp4
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/features2d/features2d.hpp>

#pragma omp parallel
#pragma omp parallel for

using namespace::cv;

//Images arrays
Mat frame;
Mat s[3];

//Colors
Scalar blue = Scalar(255, 0, 0);
Scalar green = Scalar(0, 255, 0);
Scalar red = Scalar(0, 0, 255);

//Dimensions
int width, height;

//Switches
bool tracking = false;
bool drawobject = false;

//Object
Rect object;

//ROI
Rect ROI;
int ROIside = 90;

//Threshold limiter
int minDistance = 9;
int minThreshold = 3;
int tHue = 3;
double tSat = 0.35;
double tVal = 0.35;

//Colors
vector<Scalar> trackColors;
vector<Point> detectedPixels;
vector<bool> detectedBool;

//Names
const cv::string wnVideo = "Video1";
const cv::string wnControls = "Controls";
const cv::string msg1 = "Activos: ";
const cv::string msg2 = " / ";
const cv::string msg3 = " RPS: ";

#include <time.h>
//FPS
clock_t bench_start = clock(), bench_finish;
double bench_result;
int fps=0, tfps=0;

//Tracker
int totalDetected;
vector<Point> points;

void processFrame();
void setFrame(Mat src);
void setROI(Point point);
//void drawEverything(Point point);
void findObject();
void meassureDistances();
void processROI();
void getColor(Point point);
void setPoint(int x, int y);
Mat getFrame();

void setFrame(Mat isrc)
{
	frame = isrc;

	width = frame.cols;
	height = frame.rows;

	Mat HSV;

	cvtColor(frame, HSV, CV_BGR2HSV);
	split(HSV, s);
}

Point point;
void setPoint(int x, int y)
{
	point = Point(x, y);

	getColor(point);
	setROI(point);
	tracking = true;
}

void getColor(Point point)
{
	//Getting similar colors from pixels around the selected point
	/*	
	h s v
	s P s
	v s h 
	*/
	trackColors.clear();
	for(int x = point.x-1; x <= point.x+1; x++)
		for(int y = point.y-1; y <= point.y+1; y++)
		{
			int hue = s[0].at<uchar>(Point(x, y));
			int sat = s[1].at<uchar>(Point(x, y));
			int val = s[2].at<uchar>(Point(x, y));
			trackColors.push_back(Scalar(hue, sat, val));
		}
}

void setROI(Point point)
{
	//Excepciones a los clicks en los bordes
	/*

	*/

	if( point.x - ROIside > 0 &&
		point.y - ROIside > 0 && 
		point.x + ROIside < width && 
		point.y + ROIside < height) 
	{
		ROI = Rect(point.x - ROIside, point.y - ROIside, ROIside * 2, ROIside * 2); 
	}

}

void processROI()
{
	if(tracking)
	{
		totalDetected = 0;
		//Draw working square
		rectangle(frame, ROI, blue, 2, 3);

		detectedPixels.clear();
		detectedBool.clear();
		for(int x = ROI.x; x <= ROI.x + ROI.width; x+= 2)
			for(int y = ROI.y; y <= ROI.y + ROI.height; y+= 2)
			{
				//Get pixel HSV values 0-255  0.0-1.0  0.0-1.0
				int cHue = s[0].at<uchar>(Point(x, y));
				double cSat = (double) s[1].at<uchar>(Point(x, y)) / 255;
				double cVal = (double) s[2].at<uchar>(Point(x, y)) / 255;

				int matchesFromColors = 0;

				for(vector<Scalar>::iterator it = trackColors.begin(); it != trackColors.end(); it++)
				{
					Scalar trackColor = *it;

					if((cHue-tHue < trackColor[0] && cHue+tHue > trackColor[0]) &&
						(cSat-tSat < (double) trackColor[1] / 255 && cSat+tSat > (double) trackColor[1] / 255) && 
						(cVal-tVal < (double) trackColor[2] / 255 && cVal+tVal > (double) trackColor[2] / 255))
					{
						matchesFromColors++;

						if(matchesFromColors >= minThreshold)
						{
							//frame.at<Vec3b>(Point(x, y))[1] = 255;
							totalDetected++;
							detectedPixels.push_back(Point(x, y));
							detectedBool.push_back(false);
							break;
						}
					}
				}
			}

			meassureDistances();
			findObject();
	}
}

void meassureDistances()
{

	if(detectedPixels.size() > 2)
	{
		int detectedNum = 0;
		int erased = 0;

		/*
		Removing detected pixels that are close to the center of the object
		and those which are far out of the bounds of the object
		*/
		for(vector<Point>::iterator it = detectedPixels.begin(); it != detectedPixels.end(); ++it)
		{
			Point P = point;
			Point C = *it;

			double dstX = 0, dstY = 0;

			if(C.x < P.x) {
				dstX = P.x - C.x;
			} 
			else if(C.x > P.x) {
				dstX = C.x - P.x;
			}

			if(C.y < P.y) {
				dstY = P.y - C.y;
			}
			else if(C.y > P.y) {
				dstY = C.y - P.y;
			}

				//Distance between two points
				if(dstX != 0 && dstY != 0)
				{
					double distanceFromP = sqrt(pow(dstX, 2) + pow(dstY, 2));

					if(distanceFromP < (object.width+object.height)/2/2.4)
					{
						
						if(detectedNum < detectedPixels.size() - erased)
						{
							erased++;
							std::vector<bool>::iterator it2 = detectedBool.begin();
							std::advance(it2, detectedNum);

							it2 = detectedBool.erase(it2);
							it = detectedPixels.erase(it);
						}
					}
					else if(distanceFromP > (object.width+object.height)/2*1.3)
					{
						
						if(detectedNum < detectedPixels.size() - erased)
						{
							erased++;
							std::vector<bool>::iterator it2 = detectedBool.begin();
							std::advance(it2, detectedNum);

							it2 = detectedBool.erase(it2);
							it = detectedPixels.erase(it);
						}
					}

					detectedNum++;
				}

		}

		//Checking distances between two points, with each point... The game of life
		for(vector<Point>::iterator it = detectedPixels.begin(); it != detectedPixels.end(); it++)
		{
			Point P = *it;
			int detectedNum = 0;
			for(vector<Point>::iterator it2 = detectedPixels.begin(); it2 != detectedPixels.end(); it2++)
			{
				Point C = *it2;

				double dstX = 0, dstY = 0;

				if(C.x < P.x) {
					dstX = P.x - C.x;
				} 
				else if(C.x > P.x) {
					dstX = C.x - P.x;
				}

				if(C.y < P.y) {
					dstY = P.y - C.y;
				}
				else if(C.y > P.y) {
					dstY = C.y - P.y;
				}

				//Distance between two points
				if(dstX != 0 && dstY != 0)
				{
					double distanceFromP = sqrt(pow(dstX, 2) + pow(dstY, 2));

					if(distanceFromP < minDistance)
					{
						detectedBool.at(detectedNum) = true;
					}

					detectedNum++;
				}
			}
		}
		//La funcion va a medir las distancias entre puntos descartando los menos precisos

		//findObject();
	}
}

void findObject()
{
	unsigned int sumX = 0, sumY = 0, cant = 0;
	int detectedNum = 0;
	object = Rect(point.x,point.y,ROIside/2,ROIside/2);

	for(vector<bool>::iterator it = detectedBool.begin(); it != detectedBool.end(); it++)
	{
		bool enabled = *it;

		//After measuring distances, we check which pixels are "close to eachother;"
		if(enabled) //enabled
		{
			int x = detectedPixels.at(detectedNum).x;
			int y = detectedPixels.at(detectedNum).y;

			//bounds of the object
			if(x < object.x) { object.x = x; }
			if(y < object.y) { object.y = y; }

			if(x-object.x > object.width ) { object.width = x-object.x; }
			if(y-object.y > object.height ) { object.height = y-object.y; }

			//frame.at<Vec3b>(detectedPixels.at(detectedNum))[1] = 255;

			cant++;

			sumX += x;
			sumY += y;
		}
		detectedNum++;
	}


#ifdef _DEBUG
		//Prueba de FPS
		std::stringstream cant_stream;
		cant_stream << msg1 << cant << msg2 << detectedPixels.size() << msg2 << totalDetected;
		string cantName = cant_stream.str();
		
		std::stringstream rps_stream;
		rps_stream << msg3 << fps;
		string fpsName = rps_stream.str();
		
		int fontFace = CV_FONT_HERSHEY_SIMPLEX;

		cv::putText(frame, cantName, Point(15, 15), fontFace, 0.6,
			green, 2, 8);
		cv::putText(frame, fpsName, Point(450, 15), fontFace, 0.6,
			green, 2, 8);

		double current_clock = clock();

		if(current_clock - bench_start >= CLOCKS_PER_SEC)
		{
			fps = tfps;
			tfps = 0;
			current_clock = 0;
			bench_start = clock();
		}
#endif

	if(cant > 0)
	{
		
		point = Point(sumX / cant, sumY / cant);

		//Smoothing point
		points.push_back(point);
		if(points.size() > 2)
			points.erase(points.begin());

		Point avgPoint;
		for(vector<Point>::iterator p = points.begin(); p != points.end(); p++)
		{
			avgPoint += *p;
		}

		avgPoint.x /= points.size();
		avgPoint.y /= points.size();

		setROI(avgPoint);

		object = Rect(object.x - 10, object.y - 10, object.width + 10, object.height + 10);
		rectangle(frame, object, red, 2, 3);
		circle(frame, avgPoint, 5, green, 2, 8);
	}
}

/*void drawEverything(Point point)
{
	//rectangle(frame, ROI, blue, 2, 3);
	rectangle(frame, object, red, 1, 3);
	circle(frame, point, 3, green, 3, 8);

	//return frame;
}*/

void onMouseClick(int event, int x, int y, int flags)
{
	if(event != EVENT_LBUTTONDOWN)
	return;

	setPoint(x, y);
}

int main()
{
	string filename = "C:\\Users\\N1\\Downloads\\MOV_00272.mp4";
	VideoCapture capture(-1);

	namedWindow(wnVideo, 1);

	setMouseCallback(wnVideo, *(MouseCallback) onMouseClick);

	Mat src;
	while(1)
	{
		capture >> src;

		if(src.empty())
		{
			capture.set(CV_CAP_PROP_POS_AVI_RATIO, 0);
			continue;
		}

		setFrame(src);
		processROI();
		imshow(wnVideo, src);
		tfps++;
		waitKey(16);
	}

	return 0;
}
