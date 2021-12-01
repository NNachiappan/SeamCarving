#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;



Mat energy(Mat input) {

	input.convertTo(input, CV_32F);

	cvtColor(input, input, COLOR_BGR2GRAY);

	Mat sobeldx = (Mat_<float>(3, 3) << 1, 0, -1,
		2, 0, -2,
		1, 0, -1);

	Mat sobeldy = (Mat_<float>(3, 3) << 1, 2, 1,
		0, 0, 0,
		-1, -2, -1);

	Mat dx, dy, dst, output;

	filter2D(input, dx, -1, sobeldx);
	filter2D(input, dy, -1, sobeldy);

	pow(dx, 2, dx);
	pow(dy, 2, dy);

	add(dx, dy, dst);

	sqrt(dst, output);

	return output;
}

tuple<Mat, Mat> minEnergy(Mat input) {

	int rows = input.rows;
	int cols = input.cols;

	Mat output = input.clone();
	Mat dirs = Mat::zeros(rows, cols, CV_8S);


	for (int i = 0; i < rows - 1; i++) {
		for (int j = 0; j < cols; j++) {

			//find min of the 3 pixels below
			int cRow = rows - 1 - i;
			float minVal = numeric_limits<float>::max();
			signed char dirn = 0;

			////get values of these pixels
			//all values will be less than 1000 in the energy array
			float pixels[3] = { minVal, output.at<float>(cRow, j), minVal };

			if (j == 0) {
				pixels[2] = output.at<float>(cRow, j + 1);
			}
			else if (j == cols - 1) {
				pixels[0] = output.at<float>(cRow, j - 1);
			}
			else {
				pixels[0] = output.at<float>(cRow, j - 1);
				pixels[2] = output.at<float>(cRow, j + 1);
			}

			//get min value and index
			for (int k = 0; k < 3; k++) {
				if (minVal > pixels[k]) {
					minVal = pixels[k];
					dirn = k;
				}
			}

			//convert to -1, 0, 1
			dirn -= 1;

			output.at<float>(cRow - 1, j) = output.at<float>(cRow - 1, j) + minVal;
			dirs.at<schar>(cRow - 1, j) = dirn;
		}
	}

	return { output, dirs };
}

vector<tuple<short, short>> findPath(Mat input, Mat dirs) {
	vector<tuple<short, short>> path;

	//sort top row ascending
	Mat topRow = input.row(0);
	sortIdx(topRow, topRow, SORT_EVERY_ROW + SORT_ASCENDING);

	short int nextJ = topRow.at<int>(0, 0);
	for (int i = 0; i < dirs.rows; i++) {
		path.emplace_back(i, nextJ);
		nextJ = nextJ + dirs.at<schar>(i, nextJ);
	}

	return path;
}

Mat drawPath(Mat img, vector<tuple<short, short>> path) {

	for (int i = 0; i < path.size(); i++) {
		short int row = get<0>(path[i]);
		short int col = get<1>(path[i]);

		Rect r = Rect(col, row, 1, 1);
		rectangle(img, r, Scalar(255, 0, 0), 1, 8, 0);
	}

	return img;
}

Mat removePath(Mat img, vector<tuple<short, short>> path) {

	int rows = img.rows;
	int cols = img.cols;

	Mat out = Mat::zeros(rows, cols - 1, CV_8UC3);

	for (int i = 0; i < path.size(); i++) {
		int pRow = get<0>(path[i]);
		int pCol = get<1>(path[i]);

		Mat leftSide = img(Rect(0, i, pCol, 1)).clone();
		Mat newLeftSide = out(Rect(0, i, pCol, 1));
		leftSide.copyTo(newLeftSide);

		//If the path is not on the rightmost side.
		if (pCol != cols - 1) {
			Mat rightSide = img(Rect(pCol + 1, i, cols - pCol - 1, 1)).clone();
			Mat newRightSide = out(Rect(pCol, i, cols - pCol - 1, 1));
			rightSide.copyTo(newRightSide);
		}
	}

	return out;
}

int main()
{
	std::string image_path = samples::findFile("C:/Users/Nachiappan/Desktop/squirrel.jpg");
	Mat img = imread(image_path, IMREAD_COLOR);
	imshow("Display Window", img);


	//Mat out = energy(img);
	//normalize(out, out, 0, 1, NORM_MINMAX, -1);
	//imshow("Display Window", out);
	//waitKey(0);

	float scale = 0;
	cout << "Enter scale factor as decimal:" << endl;
	cin >> scale;

	int noPixels = img.cols - ceil(img.cols * scale);

	Mat out = img.clone();


	//Remove noPixels paths from image.
	for (int i = 0; i < noPixels; i++) {
		Mat minEnergyImg, dirs;
		tie(minEnergyImg, dirs) = minEnergy(energy(out));
		vector<tuple<short, short>> path = findPath(minEnergyImg, dirs);
		out = removePath(out, path);
	}

	imshow("Output", out);
	waitKey(0);


	return 0;
}