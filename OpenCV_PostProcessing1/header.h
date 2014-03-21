#include <opencv2/opencv.hpp> 
#include <iostream> 
#include <stdio.h> 
#include <string> 
#include <stdlib.h> 
#include <Windows.h> 

#include <iomanip>

//#include <cvblob.h>



#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>

using namespace std; 
using namespace cv; 
//using namespace cvb;

#pragma omp parallel

string controls = "Controls";
string video = "Video1";
//string wname[10] = {"Type", "Size", "Precision", "Hessian", "ThrType", "ThrValue", "Thres", "Thres Max", "ThresType"};
string size_str = "Size";
string precision_str = "Precision";
string draw_str = "Paint";

//Location & ROI
Point point;
Point point_prev;
Scalar fillColor = Scalar(255, 255, 255); 
Rect roi; 
Rect object;

//
int CVal;
int CSat;

//Fun
vector<Point> drawing;
int draw = 1;

//Single color picker
double pcH, pcS, pcV; 
Scalar pointColor; 

vector<Point> rangePicker; //ubicaciones
Scalar pointColors[5]; //sus colores
//Detected points
vector<Point> detected;

int thl = 10;  //6
int rsize = 20; //15
int rsize_aux = 20; //controla el cambio gradual

int width, height; 

Mat s[3]; // 3-layer color array

//Tracking

//Falta implementar deteccion de multiples objetos y ROI

static void matching(Scalar HSV)
{
	int matches;
	int cant = 0;
	int sumX = 0, sumY = 0;
	CVal = 0;
	CSat = 0;

	for (int x = roi.x; x <= roi.x + roi.width; x++) { 
		for (int y = roi.y; y <= roi.y + roi.height; y++) {
			
			CVal += (int) s[2].at<uchar>(Point(x, y));
			CSat += (int) s[1].at<uchar>(Point(x, y));
		}
	}
	CVal = CVal / (roi.width * roi.height);
	CSat = CSat / (roi.width * roi.height);


	for (int x = roi.x; x <= roi.x + roi.width; x++) { 
		for (int y = roi.y; y <= roi.y + roi.height; y++) {

			int th_h = (int) s[0].at<uchar>(Point(x, y));
			int th_s = (int) s[1].at<uchar>(Point(x, y));
			int th_v = (int) s[2].at<uchar>(Point(x, y));

						//cout << roi.y << " " << y << " " << roi.y + roi.height << endl;

			//avg(contours); 

			//Deteccion por matcheo(puede extenderse)
			matches = 0;
			for(int r = 0; r < rangePicker.size() - 1; r++) {
				HSV = pointColors[r];

				pcH = HSV[0], pcS = HSV[1], pcV = HSV[2];
				pcV = (pcV + CVal) / 2;
				pcS = (pcS + CSat) / 2;


				//cout << "H: << " << th_h << " S: << " << th_s << " V: << " << th_v << endl;
				if((th_h > pcH - (thl/2) && th_h < pcH + (thl/2)) &&
					(th_s > pcS - (thl*2) && th_s < pcS + (thl*2)) &&
					(th_v > pcV - (thl*2) && th_v < pcV + (thl*2))) {  
						matches++;
						break;
				}
			}

			if(matches >= 1)
			{
				cant++;
				sumX += x;
				sumY += y;
				//matches = 0;


				//Implementacion inicial de limpieza
				detected.push_back(Point(x, y));

				if(x < object.x) { object.x = x; }
				if(y < object.y) { object.y = y; }

				if(x > object.width ) { object.width = x; }
				if(y > object.height ) { object.height = y; }
				//cout << roi.x << " " << roi.y << " " << x << " " << y << roi.x + roi.width << " " << roi.y + roi.height << endl;
				//frame.at<Vec3b>(Point(x, y))[2] = 255; //saturate_cast<uchar>(10 * (frame.at<Vec3b>(x, y)[0]) + 1.5);
				//imshow(video, frame);
				//Sleep(1);
			}
		} 
		
		if(cant > 0) {
			//cout << ((rightX - leftX) * (rightY - leftY)) / cant << endl;
			point_prev = point;
			point = Point(sumX / cant, sumY / cant);
			
			if( roi.x - 1 > 0 &&
				roi.y - 1 > 0 && 
				roi.x + roi.width + 1 < width && 
				roi.y + roi.height + 1 < height) 
			{
				if(rsize < rsize_aux)
				{
					rsize+=1;
				} else if(rsize > rsize_aux) {
					rsize-=1;
				}

				if(rsize_aux < object.width - object.x)
				{
					//rsize_aux++;
					//cout << "+";
				} 
				else if(rsize_aux > rsize_aux)
				{
					//cout << "-";
					//rsize_aux--;
				}
			}

		} else {

			//roi = Rect(1, 1, width, height);
			//point = point + (point-point_prev);
			//rsize_aux+= 5;
		}
		
	}
}

static void tracking(Mat frame)
{
	split(frame, s);

	//Ajuste dinamico de roi
	if(point.x > 0 && point.y > 0)
	{ 
		//Fun
		if(draw == 1) {
			drawing.push_back(point);
			for(vector<Point>::iterator cd = drawing.begin() + 1;  cd != drawing.end(); cd++)
			{
				Point pb = *(cd - 1);
				Point pc = *cd;
				//circle(frame, p, 2, Scalar(0, 0, 255), 4, 8);
				cv::line(frame, pb, pc, Scalar(0, 0, 255), 3, 8);
			}
		}

		roi.height = rsize * 2;
		roi.width = rsize * 2;

		//Verificando si esta en el marco
		if( point.x > 0 + rsize && point.x < width - rsize) {
			roi.x = point.x - rsize;
		} 

		if( point.y > 0 + rsize && point.y < height - rsize) {
			roi.y = point.y - rsize;
		}


		/*if( point.y > 0 + rsize && point.y < height - rsize) {
		roi.y = point.y - rsize;
		}
		if( point.x > 0 + rsize && point.x < width - rsize) {
		roi.x = point.x - rsize;
		}*/

		Scalar HSV;

		object.x = point.x; object.y = point.y; object.width = point.x; object.height = point.y;

		matching(HSV);

		//Cambiar esto para que este acorde a la vericicacion de los 5 puntos

		detected.clear();
	}

	cv::rectangle(frame, Rect(object.x - 10, object.y - 10, object.width - object.x + 20, object.height - object.y + 20), Scalar(0, 0, 255), 2, 4, 0); 

	//cout << leftX << " " << rightX << " " << leftY << " " << rightY << endl;
	//cout << leftX - rightX << " " << leftY - rightY << endl;
	cv::rectangle(frame, roi, Scalar(255, 100, 100), 3, 8, 0);
	cv::circle(frame, point, 2, Scalar(0, 255, 0), 2, 2);
	//cout << leftY - rightY << " " << rightX - leftX << endl;
	object.x = point.x; object.y = point.y; object.width = point.x; object.height = point.y;
	//cv::circle(frame, point, 15, Scalar(100, 255, 100), 2, 8); 
}
