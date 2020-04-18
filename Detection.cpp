//
// Created by elsa on 2020/3/18.
//

#include "Detection.h"

////色相（黄色）
Scalar hsvMin = Scalar(26, 43, 46);
Scalar hsvMax = Scalar(34, 255, 255);
// cv::Mat hsvMin_mat = cv::Mat(hsvMin); //将vector变成单列的mat
// cv::Mat hsvMax_mat = cv::Mat(hsvMax); //将vector变成单列的mat

Detection::Detection(Mat &image) {
    vertex2D.reserve(4); // 分配空间
    fallPoint2D.reserve(6);
    edgePointsUp2D.reserve(6);
    edgePointsDown2D.reserve(6);
    srcImage = image.clone();
    dstImage = image;
}

void Detection::process() {
    Mat mask;
    HSVFilter(srcImage, mask);
    borderHough(mask, dstImage);
    fallPointFind();
    drawArmRange();
}

// 4个角点+6个落点作为关键点
vector<Point2f> Detection::getKeyPoints() {
    // 将vec1和vec2的内容合并到keyPoints中
    keyPoints.clear();
    keyPoints.insert(keyPoints.end(), vertex2D.begin(), vertex2D.end());
    keyPoints.insert(keyPoints.end(), fallPoint2D.begin(), fallPoint2D.end());
    return keyPoints;
}

vector<Point2f> Detection::getEdgePoints() {
    edgePoints.clear();
    edgePoints.insert(edgePoints.end(), vertex2D.begin(), vertex2D.end());
    edgePoints.insert(edgePoints.end(), fallPoint2D.begin(), fallPoint2D.end());
    edgePoints.insert(edgePoints.end(), edgePointsUp2D.begin(), edgePointsUp2D.end());
    edgePoints.insert(edgePoints.end(), edgePointsDown2D.begin(), edgePointsDown2D.end());
    return edgePoints;
}

// HSV预处理
void Detection::HSVFilter(Mat inputImage, Mat &outputImage) {
    Mat hsvImage;
    // bgr转hsv
    cvtColor(inputImage, hsvImage, CV_BGR2HSV);

    Mat mask;

    // scalar不是bgr吗，为什么可以限定上下限，Scalar只是一个不超过4维的向量
    inRange(hsvImage, hsvMin, hsvMax, mask);

    //形态学运算
    Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));
    erode(mask, mask, element); //腐蚀
    dilate(mask, mask, element); //膨胀

    outputImage = mask;
}

// 霍夫直线检测
void Detection::borderHough(Mat inputImage, Mat &outputImage) {
    vector<Vec4f> lines;
    HoughLinesP(inputImage, lines, 1, CV_PI / 180, 900, 500, 10); //第五个参数：超过这个值才被检测出直线

    int imgW = inputImage.cols;
    int imgH = inputImage.rows;

    cout << "图片的宽为" << imgW << endl;
    cout << "图片的高为" << imgH << endl;
    cout << "************************************" << endl;

    // TODO 如何精简
    int gap = 50;
    //上下左右四条边界线端点
    Vec4f lineUp(gap, gap, imgW - gap, gap); // up
    Vec4f lineDown(gap, imgH - gap, imgW - gap, imgH - gap); // down
    Vec4f lineLeft(gap + 10, gap, gap, imgH - gap); // left 注意不要完全垂直
    Vec4f lineRight(imgW - gap - 10, gap, imgW - gap, imgH - gap); // right

    //将画面上下两条边加进去
    int linesNum = 20; // TODO 参数linesNum
    vector<Vec4f> lines3SidesUD(lines);
    lines3SidesUD.insert(lines3SidesUD.end(), linesNum, lineUp);
    lines3SidesUD.insert(lines3SidesUD.end(), linesNum, lineDown);
    //将画面左右两条边加进去
    vector<Vec4f> lines3SidesLR(lines);
    lines3SidesLR.insert(lines3SidesLR.end(), linesNum, lineLeft);
    lines3SidesLR.insert(lines3SidesLR.end(), linesNum, lineRight);
    //将画面上下左右都加进去
    vector<Vec4f> lines2Sides(lines3SidesUD);
    lines2Sides.insert(lines2Sides.end(), linesNum, lineLeft);
    lines2Sides.insert(lines2Sides.end(), linesNum, lineRight);

    Mat tempImage = outputImage.clone();

    vector<Vertex> top4vertexSet;
    mostIntersections(lines, top4vertexSet, 4, imgW, imgH);

    //判断有几条直线
    if (top4vertexSet[0].crossTimes > 4 * top4vertexSet[1].crossTimes) { // TODO 参数4
        vector<Vertex> top9vertexSet;
        cout << "只有两条直线，并相交" << endl; // TODO 只有三个交点的情况要忽略
        mostIntersections(lines2Sides, top9vertexSet, 9, imgW, imgH);
        vector<Vertex> vertexResult;
        pointColor(outputImage, top9vertexSet, vertexResult);

        for (int k = 0; k < vertexResult.size(); ++k) {
            vertex2D[k] = Point2f(vertexResult[k].x, vertexResult[k].y);
        }

        int findPoint4 = 0;
        int draw = 0;
        for (int i = 0; i < vertexResult.size(); ++i) {
            if (vertexResult[i].x < 2 * gap) {
                for (int j = 0; j < vertexResult.size(); ++j) {
                    if (j != i) {
                        if (vertexResult[j].x < 2 * gap) {
                            drawBox(vertexResult, outputImage);
                            findPoint4 = 1;
                            draw = 1;
                            break;
                        }
                        if (vertexResult[j].y < 2 * gap) {
                            Vertex newVertex(gap, gap);
                            vertexResult.push_back(newVertex);
                            findPoint4 = 1;
                            break;
                        }
                        if (vertexResult[j].y > imgH - 2 * gap) {
                            Vertex newVertex(gap, imgH - gap);
                            vertexResult.push_back(newVertex);
                            findPoint4 = 1;
                            break;
                        }
                    }
                }
            }
            if (vertexResult[i].x > imgW - 2 * gap) {
                for (int j = 0; j < vertexResult.size(); ++j) {
                    if (j != i) {
                        if (vertexResult[j].x > imgW - 2 * gap) {
                            drawBox(vertexResult, outputImage);
                            findPoint4 = 1;
                            draw = 1;
                            break;
                        }
                        if (vertexResult[j].y < 2 * gap) {
                            Vertex newVertex(imgW - gap, gap);
                            vertexResult.push_back(newVertex);
                            findPoint4 = 1;
                            break;
                        }
                        if (vertexResult[j].y > imgH - 2 * gap) {
                            Vertex newVertex(imgW - gap, imgH - gap);
                            vertexResult.push_back(newVertex);
                            findPoint4 = 1;
                            break;
                        }
                    }
                }
            }
            if ((vertexResult[i].y > imgH - 2 * gap) &&
                (vertexResult[(i + 1) % vertexResult.size()].y > imgH - 2 * gap)) {
                drawBox(vertexResult, outputImage);
                draw = 1;
                break;
            }

            if ((vertexResult[i].y < 2 * gap) && (vertexResult[(i + 1) % vertexResult.size()].y < 2 * gap)) {
                drawBox(vertexResult, outputImage);
                draw = 1;
                break;
            }
            if (findPoint4 == 1)
                break;
        }
        if (draw == 0) {
            drawLines(vertexResult, outputImage);
            drawPoints(vertexResult, outputImage);
        }
    } else if (top4vertexSet[0].crossTimes < 1000) { // TODO 参数1000
        cout << "只有两条直线，平行" << endl;
        vector<Vertex> top4vertexSet_2;
        mostIntersections(lines2Sides, top4vertexSet_2, 4, imgW, imgH);
        if (top4vertexSet_2[0].crossTimes <= 4 * top4vertexSet_2[2].crossTimes) { //排除只有一条边的情况

            for (int k = 0; k < top4vertexSet_2.size(); ++k) {
                vertex2D[k] = Point2f(top4vertexSet_2[k].x, top4vertexSet_2[k].y);
            }

            drawLines(top4vertexSet_2, outputImage);
            drawPoints(top4vertexSet_2, outputImage);
        }

    } else if (top4vertexSet[0].crossTimes > 4 * top4vertexSet[2].crossTimes) { // TODO 参数4
        cout << "只有三条直线" << endl;
        vector<Vertex> top6vertexSet;
        int yGap = 500; // TODO 参数yGap
        if (abs(top4vertexSet[0].y - top4vertexSet[1].y) < yGap) //若两交点y很接近，则取上下边界
            mostIntersections(lines3SidesUD, top6vertexSet, 6, imgW, imgH);
        else
            mostIntersections(lines3SidesLR, top6vertexSet, 6, imgW, imgH);

        vector<Vertex> vertexResult;
        pointColor(outputImage, top6vertexSet, vertexResult);

        for (int k = 0; k < vertexResult.size(); ++k) {
            vertex2D[k] = Point2f(vertexResult[k].x, vertexResult[k].y);
        }

        cout << "交点个数为：" << vertexResult.size() << endl;
        //绘制直线
        drawLines(vertexResult, outputImage);
        //绘制交点
        drawPoints(vertexResult, outputImage);
    } else {
        cout << "四条直线" << endl;
        for (int k = 0; k < top4vertexSet.size(); ++k) {
            vertex2D[k] = Point2f(top4vertexSet[k].x, top4vertexSet[k].y);
        }
        //绘制直线
        drawLines(top4vertexSet, outputImage);
        //绘制最终的四个交点
        drawPoints(top4vertexSet, outputImage);
    }
}

// 求直线交点
void Detection::getCrossPointAndIncrement(Vec4f LineA, Vec4f LineB, vector<Vertex> &vertexSet, int imgW, int imgH) {
    float ka, kb;
    ka = (LineA[3] - LineA[1]) / (LineA[2] - LineA[0]); //求出LineA斜率
    kb = (LineB[3] - LineB[1]) / (LineB[2] - LineB[0]); //求出LineB斜率

    Point2f crossPoint;
    crossPoint.x = (ka * LineA[0] - LineA[1] - kb * LineB[0] + LineB[1]) / (ka - kb);
    crossPoint.y = (ka * kb * (LineA[0] - LineB[0]) + ka * LineB[1] - kb * LineA[1]) / (ka - kb);

    int x = (int) (round)(crossPoint.x);
    int y = (int) (round)(crossPoint.y);

    int VertexGap = 40000; // TODO VerTexGap

    if (x >= -imgW / 2 && x <= imgW * 1.5 && y >= -imgH / 2 && y <= imgH * 1.5) { //在图像区域内
        int i = 0;
        for (i = 0; i < vertexSet.size(); i++) { //与已有的点靠近，可以合并
            int oldX = vertexSet[i].x;
            int oldY = vertexSet[i].y;

            //附近有特别靠近的点，可以合并
            if ((oldX - x) * (oldX - x) + (oldY - y) * (oldY - y) <= VertexGap) {
                vertexSet[i].addCrossTimes();
                break;
            }
        }

        if (i == vertexSet.size()) { //如果该点附近没有距离特别近的点，自身作为一个新点
            Vertex newVertex(x, y);
            vertexSet.push_back(newVertex);
        }
    }
}

// 直线相交次数最多的几个点
void Detection::mostIntersections(vector<Vec4f> lines, vector<Vertex> &topVertexSet, int topVertexNum, int imgW,
                                  int imgH) {
    //获取所有直线的交点和相交次数
    vector<Vertex> vertexSet;
    for (unsigned int i = 0; i < lines.size(); i++) {
        for (unsigned int j = i + 1; j < lines.size(); j++) {
            getCrossPointAndIncrement(lines[i], lines[j], vertexSet, imgW, imgH);
        }
    }

    //找相交次数最多的topVertexNum个点
    // vertexSet 按照crossTimes排序，取前topVertexNum个
    sort(vertexSet.begin(), vertexSet.end(),
         [](const Vertex &vt1, const Vertex &vt2) { return vt1.crossTimes > vt2.crossTimes; }); // 降序
    topVertexSet.assign(vertexSet.begin(), vertexSet.begin() + topVertexNum); //取前topVertexNum个
}

// 验证角点是否为黄色 verifyVertexColor
void Detection::pointColor(Mat image, vector<Vertex> inputVertexSet, vector<Vertex> &outputVertexSet) {
    int imgW = image.cols;
    int imgH = image.rows;

    Mat hsvImage;
    cvtColor(image, hsvImage, CV_BGR2HSV); // bgr转hsv

    for (int i = 0; i < inputVertexSet.size(); ++i) {
        int x = inputVertexSet[i].x;
        int y = inputVertexSet[i].y;
        // TODO 如何精简
        if (x < 0)
            inputVertexSet[i].x = 0;
        if (x > imgW)
            inputVertexSet[i].x = imgW;
        if (y < 0)
            inputVertexSet[i].y = 0;
        if (y > imgH)
            inputVertexSet[i].y = imgH;
        int range = 200; // TODO 参数range
        int flag = 0;
        //注意不要超出画面边界
        int xMin = x - range;
        int xMax = x + range;
        int yMin = y - range;
        int yMax = y + range;
        if (xMin < 0)
            xMin = 0;
        if (xMin > imgW)
            xMin = imgW - 10;
        if (xMax < 0)
            xMax = 10;
        if (xMax > imgW)
            xMax = imgW;
        if (yMin < 0)
            yMin = 0;
        if (yMin > imgH)
            yMin = imgH - 10;
        if (yMax < 0)
            yMax = 10;
        if (yMax > imgH)
            yMax = imgH;
        //看交点是否为黄色
        for (int j = xMin; j < xMax; j = j + 5) {
            for (int k = yMin; k < yMax; k = k + 5) {
                cv::Vec3b hsvPoint = hsvImage.at<Vec3b>(k, j);
                cv::Vec3b hsvInRangeRes;
                inRange(hsvPoint, hsvMin, hsvMax, hsvInRangeRes); // 在范围内255，否则0
                int InRangeNum = countNonZero(hsvInRangeRes); // 非零个数
                if (InRangeNum == 3) { //hsv 都在范围内 返回（255,255,255）
                    outputVertexSet.push_back(inputVertexSet[i]);
                    flag = 1;
                    break;
                }
            }
            if (flag == 1)
                break;
        }
    }
}

// 框内落点坐标
void Detection::fallPointFind() {
    sort(vertex2D.begin(), vertex2D.end(),
         [](const Point2f &pt1, const Point2f &pt2) { return pt1.y < pt2.y; }); // 按y值升序

    if (vertex2D[0].x > vertex2D[1].x) // 先比较y值最小的两个点的x值
        swap(vertex2D[0], vertex2D[1]);
    if (vertex2D[2].x > vertex2D[3].x)
        swap(vertex2D[2], vertex2D[3]);

    for (int i = 0; i < 4; i++)
        cout << "[ " << vertex2D[i].x << " , " << vertex2D[i].y << " ]" << endl;

    Point2f midPointL = Point2f((vertex2D[0].x + vertex2D[2].x) / 2, (vertex2D[0].y + vertex2D[2].y) / 2);
    Point2f midPointR = Point2f((vertex2D[1].x + vertex2D[3].x) / 2, (vertex2D[1].y + vertex2D[3].y) / 2);

    float midK = (midPointR.y - midPointL.y) / (midPointR.x - midPointL.x);
    float midB = midPointL.y - midK * midPointL.x;

    int space = (midPointR.x - midPointL.x) / 7; //间距
    int fallPointStart = midPointR.x - space; //第一个落点在最右
    int fallPointNum = 6;
    for (int i = 0; i < fallPointNum; ++i) {
        float x = fallPointStart - i * space; //落点从右往左数123456
        float y = midK * x + midB;
        fallPoint2D.push_back(Point2f(x, y));
        circle(dstImage, Point2f(x, y), 30, cv::Scalar(0, 255, 0), -1);
    }
}

void Detection::edgePointFind() {
    sort(vertex2D.begin(), vertex2D.end(),
         [](const Point2f &pt1, const Point2f &pt2) { return pt1.y < pt2.y; }); // 升序

    //左上0，右上1，左下2，右下3
    if (vertex2D[0].x > vertex2D[1].x) // 先比较y值最小的两个点的x值
        swap(vertex2D[0], vertex2D[1]);
    if (vertex2D[2].x > vertex2D[3].x)
        swap(vertex2D[2], vertex2D[3]);

    int gap = 20;
    int pointNum = 6;

    Point2f upPointL = Point2f(vertex2D[0].x + gap, vertex2D[0].y + gap);
    Point2f upPointR = Point2f(vertex2D[1].x - gap, vertex2D[1].y + gap);

    float upK = (upPointR.y - upPointL.y) / (upPointR.x - upPointL.x);
    float upB = upPointL.y - upK * upPointL.x;

    int upSpace = (upPointR.x - upPointL.x) / 7; //间距
    int upStart = upPointR.x - upSpace; //第一个点在最右

    for (int i = 0; i < pointNum; ++i) {
        float x = upStart - i * upSpace; //从右往左数123456
        float y = upK * x + upB;
        edgePointsUp2D.push_back(Point2f(x, y));
    }

    Point2f downPointL = Point2f(vertex2D[2].x + gap, vertex2D[2].y - gap);
    Point2f downPointR = Point2f(vertex2D[3].x - gap, vertex2D[3].y - gap);

    float downK = (downPointR.y - downPointL.y) / (downPointR.x - downPointL.x);
    float downB = downPointL.y - downK * downPointL.x;

    int downSpace = (downPointR.x - downPointL.x) / 7; //间距
    int downStart = downPointR.x - downSpace; //第一个点在最右

    for (int i = 0; i < pointNum; ++i) {
        float x = downStart - i * downSpace; //从右往左数123456
        float y = downK * x + downB;
        edgePointsDown2D.push_back(Point2f(x, y));
    }

}

// 画角点 drawVertexPoints
void Detection::drawPoints(vector<Vertex> vertexSet, Mat &outputImage) {
    for (int i = 0; i < vertexSet.size(); i++) {
        //        cout << "(" << vertexSet[i].x << "," << vertexSet[i].y << ")" << vertexSet[i].crossTimes << endl;
        circle(outputImage, Point(vertexSet[i].x, vertexSet[i].y), 30, Scalar(0, 0, 255), -1);
    }
}

// 画边框线
void Detection::drawLines(vector<Vertex> top4vertexSet, Mat &outputImage) {
    int crossPoint = 0;
    for (int i = 1; i < 4; i++) { //第0个点与第i个点连线
        double temp_k =
                (double) (top4vertexSet[i].y - top4vertexSet[0].y) / (double) (top4vertexSet[i].x - top4vertexSet[0].x);
        double temp_b = (double) top4vertexSet[0].y - temp_k * (double) top4vertexSet[0].x;

        int flag = 0; //标志为正还是为负
        for (int j = 1; j < 4; j++) {
            if (j != i) {
                //第j个点的y坐标减线上坐标
                double diff = (double) top4vertexSet[j].y - (temp_k * (double) top4vertexSet[j].x + temp_b);
                if (flag == 0) {
                    flag = diff > 0 ? 1 : -1;
                } else {
                    if (flag == 1 && diff <= 0 || flag == -1 && diff > 0) {
                        crossPoint = i;
                        break;
                    }
                }
            }
        }
        if (crossPoint != 0)
            break;
    }

    for (int i = 1; i < 4; i++) {
        if (i != crossPoint) {
            line(outputImage, Point(top4vertexSet[i].x, top4vertexSet[i].y),
                 Point(top4vertexSet[0].x, top4vertexSet[0].y), Scalar(0, 255, 0), 30, LINE_AA);
            line(outputImage, Point(top4vertexSet[i].x, top4vertexSet[i].y),
                 Point(top4vertexSet[crossPoint].x, top4vertexSet[crossPoint].y), Scalar(0, 255, 0), 30, LINE_AA);
        }
    }
}

// 画三角形
void Detection::drawBox(vector<Vertex> vertexSet, Mat &outputImage) {
    cout << "-----------------" << endl;
    for (int i = 0; i < vertexSet.size(); i++) {
        Point pt = Point(vertexSet[i].x, vertexSet[i].y);
        Point pt1 = Point(vertexSet[(i + 1) % 3].x, vertexSet[(i + 1) % 3].y);
        line(outputImage, pt, pt1, Scalar(0, 255, 0), 30);
    }
    for (int i = 0; i < vertexSet.size(); ++i) {
        Point pt = Point(vertexSet[i].x, vertexSet[i].y);
        circle(outputImage, pt, 30, Scalar(0, 0, 255), -1);
    }
}

void Detection::drawArmRange() {
    int armHeight = srcImage.rows;
    int armL = srcImage.cols / 2 - srcImage.cols / 16; //左边界 //TODO 参数16
    int armR = srcImage.cols / 2 + srcImage.cols / 16; //右边界

    line(dstImage, Point(armL, 0), Point(armL, armHeight / 4), Scalar(0, 255, 0), 20, CV_AA); //画左边的饲料下落边界
    line(dstImage, Point(armR, 0), Point(armR, armHeight / 4), Scalar(0, 255, 0), 20, CV_AA); //画右边的饲料下落边界
}