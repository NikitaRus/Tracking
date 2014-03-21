//Homography
Mat object;// = imread("C:\\Users\\N1\\Pictures\\box.jpg", CV_LOAD_IMAGE_GRAYSCALE);
Mat frameg;
int minHessian = 2500;

//Threshold
vector< vector<Point> > contours; 

//Seleccion de tracking
int type = 0;

//Scalar loVal, hiVal; //continuar con ajustes de similitud de pixeles 
int loThreVal =100, loThreTyp =2; 
int hiThreVal =255, hiThreTyp = 3; 

//Thresholding
static void thresholding(Mat frame)
{
	Mat framehsv, framegray, edges;
	cv::cvtColor(frame, framehsv, CV_BGR2HSV); 
	cv::cvtColor(frame, framegray, CV_BGR2GRAY); 

	GaussianBlur(framegray, framegray, Size(5, 5), 1.5);
	cv::threshold(framegray, framehsv, loThreVal, hiThreVal, loThreTyp); 
	framehsv.copyTo(edges); 

	//Canny(edges, edges, 0.0, 255.0, 3, frameProcessing); // encuentra los bordes << framehsv 

	findContours(edges, contours, 3, 4, Point(0, 0)); /*CV_RETR_CCOMP CV_CHAIN_APPROX_SIMPLE*/
	/*cv::floodFill(framehsv, point, fillColor);*/ //, roi 
	//drawContours(framehsv, contours, CV_FILLED, Scalar(255, 100, 123), 2, 1); //verificar si es solo visual o si sirve para el threshold 

	imshow("Video2", framehsv);
}

//Haar cascade classifier
static void haarcascade(Mat frame)
{
	std::vector<Rect> faces;
	Mat frame_gray;

	frameg.copyTo(frame_gray);
	equalizeHist( frame_gray, frame_gray );

	//-- Detect faces
	face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

	//-- 1. Load the cascades
	if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading\n");  };
	if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("--(!)Error loading\n");  };


	for( size_t i = 0; i < faces.size(); i++ )
	{
		Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
		ellipse( frame, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );

		Mat faceROI = frame_gray( faces[i] );
		std::vector<Rect> eyes;

		//-- In each face, detect eyes
		eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE, Size(30, 30) );

		for( size_t j = 0; j < eyes.size(); j++ )
		{
			Point center( faces[i].x + eyes[j].x + eyes[j].width*0.5, faces[i].y + eyes[j].y + eyes[j].height*0.5 );
			int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
			circle( frame, center, radius, Scalar( 255, 0, 0 ), 4, 8, 0 );
		}
	}
}

//Haar Globals
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;
RNG rng(12345);

//Homography
static void homography()
{
	try
	{
		std::vector<KeyPoint> keypoints_object, keypoints_scene;
		SurfFeatureDetector detector( minHessian );

		detector.detect( object, keypoints_object );
		detector.detect( frameg, keypoints_scene );
		if(keypoints_scene.size() > 0)
		{
			//-- Step 2: Calculate descriptors (feature vectors)
			SurfDescriptorExtractor extractor;

			Mat descriptors_object, descriptors_scene;

			extractor.compute( object, keypoints_object, descriptors_object );
			extractor.compute( frameg, keypoints_scene, descriptors_scene );

			//-- Step 3: Matching descriptor vectors using FLANN matcher
			FlannBasedMatcher matcher;
			std::vector< DMatch > matches;
			matcher.match( descriptors_object, descriptors_scene, matches );

			double max_dist = 0; double min_dist = 100;

			//-- Quick calculation of max and min distances between keypoints
			for( int i = 0; i < descriptors_object.rows; i++ )
			{ double dist = matches[i].distance;
			if( dist < min_dist ) min_dist = dist;
			if( dist > max_dist ) max_dist = dist;
			}

			printf("-- Max dist : %f \n", max_dist );
			printf("-- Min dist : %f \n", min_dist );

			//-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
			std::vector< DMatch > good_matches;

			for( int i = 0; i < descriptors_object.rows; i++ )
			{ if( matches[i].distance < 3*min_dist )
			{ good_matches.push_back( matches[i]); }
			}

			Mat img_matches;
			drawMatches( object, keypoints_object, frameg, keypoints_scene,
				good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
				vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

			//-- Localize the object
			std::vector<Point2f> obj;
			std::vector<Point2f> scene;

			for( int i = 0; i < good_matches.size(); i++ )
			{
				//-- Get the keypoints from the good matches
				obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
				scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
			}

			if(scene.size() >= 4)
			{
				Mat H = findHomography( obj, scene, CV_RANSAC );

				//-- Get the corners from the image_1 ( the object to be "detected" )
				std::vector<Point2f> obj_corners(4);
				obj_corners[0] = cvPoint(0,0); obj_corners[1] = cvPoint( object.cols, 0 );
				obj_corners[2] = cvPoint( object.cols, object.rows ); obj_corners[3] = cvPoint( 0, object.rows );
				std::vector<Point2f> scene_corners(4);

				perspectiveTransform( obj_corners, scene_corners, H);

				//-- Draw lines between the corners (the mapped object in the scene - image_2 )
				line( img_matches, scene_corners[0] + Point2f( object.cols, 0), scene_corners[1] + Point2f( object.cols, 0), Scalar(0, 255, 0), 4 );
				line( img_matches, scene_corners[1] + Point2f( object.cols, 0), scene_corners[2] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );
				line( img_matches, scene_corners[2] + Point2f( object.cols, 0), scene_corners[3] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );
				line( img_matches, scene_corners[3] + Point2f( object.cols, 0), scene_corners[0] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );


				imshow("Matches", img_matches);
			}
		}
	}
	catch(const cv::Exception& ex)
	{
		cout << "Error: " << ex.what() << endl;
	}

}

static void avg (vector< vector<Point> > contours) 
{ 
	int vx = 0; 
	int vy = 0; 
	int cont = 0; 

	if(contours.size() != 0) 
	{ 
		for(std::vector<vector<Point>>::iterator it = contours.begin(); it != contours.end(); ++it) { 
			vector<Point> contour = *it; 
			for(std::vector<Point>::iterator it2 = contour.begin(); it2 != contour.end(); ++it2) { 
				Point p = *it2; 
				vx += p.x; 
				vy += p.y; 
				cont++; 
				//cout << "R: " << p.x << ", " << p.y << "\n"; 
			} 
		} 

		//point.x = vx/cont; 
		//point.y = vy/cont; 
		cout << "AVG: " << point.x << ", " << vy/cont << "\n"; 
	} 
} 

static void frameProcessing(int, void*, Mat frame) 
{ 
	
}
