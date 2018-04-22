#pragma once

/*
处理图像
*/
void image_processing() {
	const Mat &front_image = imread("E:/OpenCV_Img/male.png");
	const Mat &back_image = imread("E:/OpenCV_Img/male_back.png");
	//定义前景物体的包围盒
	cv::Rect rectangle(front_image.cols*3/14, front_image.rows/8, front_image.cols*4/7, front_image.rows*3/4);
	Mat result;
	Mat bgModel, fgModel;//模型（供内部使用）
	//GrabCut分割
	cv::grabCut(front_image,		
		result,					
		rectangle,				
		bgModel, fgModel,		
		4,						
		cv::GC_INIT_WITH_RECT);	
	cv::compare(result, cv::GC_PR_FGD, result, cv::CMP_EQ);

	Mat element(3, 3, CV_8U, cv::Scalar(1));
	cv::morphologyEx(result, result, cv::MORPH_OPEN, element);

	int minX = 0, maxX = 0, minY = 0, maxY = 0;
	int nr = front_image.rows;
	int nc = front_image.cols;
	uchar* data = result.ptr<uchar>(0);
	
	for (int i = 3; i < nr; i++) {
		data = result.ptr<uchar>(i);
		for (int j = 0; j < nc; j++) {
			if (data[j]) {
				minY = i;
				goto lastMaxY;
			}
		}
	}
lastMaxY:
	for (int i = nr - 3; i > 0; i--) {
		data = result.ptr<uchar>(i);
		for (int j = 0; j < nc; j++) {
			if (data[j]) {
				maxY = i;
				goto lastMinX;
			}
		}
	}
lastMinX:
	for (int j = 3; j < nc; j++) {
		for (int i = 3; i < nr - 3; i++) {
			data = result.ptr<uchar>(i);
			if (data[j]) {
				minX = j;
				goto lastMaxX;
			}
		}
	}
lastMaxX:
	for (int j = nc - 3; j > 0; j--) {
		for (int i = 3; i < nr - 3; i++) {
			data = result.ptr<uchar>(i);
			if (data[j]) {
				maxX = j;
				goto lastEnd;
			}
		}
	}
lastEnd:
	cout << minX << endl << maxX << endl << minY << endl << maxY << endl;

	Mat mask = result & 1;
	Mat result2(front_image.size(), CV_8UC3, cv::Scalar(0, 0, 0));
	Mat result2_back(back_image.size(), CV_8UC3, cv::Scalar(0, 0, 0));
	front_image.copyTo(result2, mask);
	back_image.copyTo(result2_back, mask);
	
	// 裁剪
	int dx, dy;
	dx = maxX - minX;
	dy = maxY - minY;
	Mat result3, result3_back;
	cv::Mat front_male, back_male;
	if (dx > dy) {
		result3.create(dx, dx, result2.type());
		Mat imageROI = result2(Rect(minX, minY - ((dx - dy) / 2), dx, dx));
		addWeighted(imageROI, 1.0, result3, 0., 0., result3);
		cv::resize(result3, front_male, cv::Size(result3.cols * 512 / dx, result3.rows * 512 / dx));

		result3_back.create(dx, dx, result2_back.type());
		Mat imageROI2 = result2_back(Rect(minX, minY - ((dx - dy) / 2), dx, dx));
		addWeighted(imageROI2, 1.0, result3_back, 0., 0., result3_back);
		cv::resize(result3_back, back_male, cv::Size(result3_back.cols * 512 / dx, result3_back.rows * 512 / dx));
	}
	else {
		result3.create(dy, dy, result2.type());
		Mat imageROI = result2(Rect(minX-((dy-dx)/2), minY, dy, dy));
		addWeighted(imageROI, 1.0, result3, 0., 0., result3);
		cv::resize(result3, front_male, cv::Size(result3.cols * 512 / dy, result3.rows * 512 / dy));

		result3_back.create(dy, dy, result2_back.type());
		Mat imageROI2 = result2_back(Rect(minX - ((dy - dx) / 2), minY, dy, dy));
		addWeighted(imageROI2, 1.0, result3_back, 0., 0., result3_back);
		cv::resize(result3_back, back_male, cv::Size(result3_back.cols * 512 / dy, result3_back.rows * 512 / dy));
	}

	cout << front_male.cols << endl << back_male.cols << endl << front_male.rows;
	
	Mat dilated;
	cv::dilate(back_male, dilated, Mat());

	Mat all_male;
	all_male.create(front_male.rows,front_male.cols + dilated.cols,  front_male.type());
	Mat ROI_1 = all_male(Rect(0, 0, front_male.cols, front_male.rows));
	addWeighted(ROI_1, 0., front_male, 1.0, 0., ROI_1);
	Mat ROI_2 = all_male(Rect(front_male.cols, 0, dilated.cols, dilated.rows));
	addWeighted(ROI_2, 0., dilated, 1.0, 0., ROI_2);


	//vector<Mat> planes;
	//split(all_male, planes);
	//vector<Mat> planes2;
	//for (int i = 0; i < 3; i++) {
	//	Mat temp;
	//	temp.create(planes[0].rows, planes[0].cols, planes[0].type());
	//	planes2.push_back(temp);
	//}
	//for (int i = 0; i < 3; i++) {
	//	Change2Max(planes[i], planes2[i]);
	//}
	//Mat end;
	//end.create(all_male.rows, all_male.cols, all_male.type());
	//merge(planes2, end);

	imwrite("E:/My_3DsMax/FirstProject/male_all.png", all_male);

	
}


/*
5x5的图元中，像素值替换成5x5的图元中的最大值
*/
void Change2Max(Mat &image, Mat &end) {
	int ncols = image.cols;
	int nrows = image.rows;
	for (int j = 2; j < nrows - 2; j++) {
		const uchar* current = image.ptr<const uchar>(j); //当前行
		const uchar* up1 = image.ptr<const uchar>(j - 1); //上一行
		const uchar* up2 = image.ptr<const uchar>(j - 2); //上二行
		const uchar* down1 = image.ptr<const uchar>(j + 1); //下一行
		const uchar* down2 = image.ptr<const uchar>(j + 2); //下二行
		uchar* output = end.ptr<uchar>(j); // 输出行
		for (int i = 2; i < ncols - 2; i++) {
			if (current[i] == 0) {
				vector<uchar> temp;
				for (int k = -2; k <= 2; k++) {
					temp.push_back(current[i + k]);
					temp.push_back(up1[i + k]);
					temp.push_back(up2[i + k]);
					temp.push_back(down1[i + k]);
					temp.push_back(down2[i + k]);
				}
				uchar max;
				FindMax(temp, max);
				output[i] = max;
			}
			else {
				output[i] = current[i];
			}
		}
	}
}

void FindMax(vector<uchar> vec, uchar &max) {
	max = vec[0];
	for (int i = 1; i < vec.size(); i++) {
		if (vec[i] >= max) max = vec[i];
	}
}