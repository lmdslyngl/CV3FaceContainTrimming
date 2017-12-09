
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/saliency.hpp>

#include "Util.h"
#include "cvlib.h"


// 指定された領域内に含まれる顔領域の面積を計算
void CalcFaceArea(
    double *pOutArea, cv::Point2i *pOutCenter,
    const std::vector<cv::Rect> &facesList, const cv::Rect &trimRegion)
{
    int trimRgnLeft = trimRegion.x;
    int trimRgnRight = trimRegion.x + trimRegion.width;
    int trimRgnTop = trimRegion.y;
    int trimRgnBottom = trimRegion.y + trimRegion.height;
    
    double areaSum = 0;
    cv::Point2i trimmedCenter(0, 0);
    int numCenterContribArea = 0;
    
    for( const auto &face : facesList ) {
        int faceLeft = face.x;
        int faceRight = face.x + face.width;
        int faceTop = face.y;
        int faceBottom = face.y + face.height;
        
        // 顔領域の面積
        int orgArea = (faceRight - faceLeft) * (faceBottom - faceTop);
        
        // 顔領域を切り取り範囲内に押し込める
        faceLeft = Clamp(faceLeft, trimRgnLeft, trimRgnRight);
        faceRight = Clamp(faceRight, trimRgnLeft, trimRgnRight);
        faceTop = Clamp(faceTop, trimRgnTop, trimRgnBottom);
        faceBottom = Clamp(faceBottom, trimRgnTop, trimRgnBottom);
        
        // 範囲内に押し込められた顔領域の面積
        int trimmedArea = (faceRight - faceLeft) * (faceBottom - faceTop);
        
        // 範囲内に入ってる顔領域の比率を加算
        areaSum += ((double)trimmedArea / (double)orgArea);
        
        if( 0 < trimmedArea ) {
            // 範囲内に入っている顔領域の中心座標を計算
            trimmedCenter.x += (faceLeft + faceRight) / 2;
            trimmedCenter.y += (faceTop + faceBottom) / 2;
            numCenterContribArea++;
        }
        
    }
    
    if( 0 < numCenterContribArea ) {
        // 範囲内に入っている顔領域の中心座標の重心を計算
        trimmedCenter.x /= numCenterContribArea;
        trimmedCenter.y /= numCenterContribArea;
    }
    
    *pOutArea = areaSum;
    *pOutCenter = trimmedCenter;
    
}

// 顔領域が最大の切り取り方を探索
cv::Rect SearchMaxFaceAreaTrimmingRegion(
    const std::vector<cv::Rect> &facesList,
    int width, int height)
{
    
    // スライドする軸，長辺，短辺
    int axis, longer, shorter;
    if( width < height ) {
        axis = 1;
        longer = height;
        shorter = width;
    } else {
        axis = 0;
        longer = width;
        shorter = height;
    }
    
    // 注目する領域
    cv::Rect region = cv::Rect(0, 0, shorter, shorter);
    
    if( facesList.empty() ) {
        // 顔が検出されなかったときは，画像中央の領域を返す
        if( axis == 0 )
            region.x = longer / 2 - shorter / 2;
        else if( axis == 1 )
            region.y = longer / 2 - shorter / 2;
        return region;
    }
    
    double maxArea = 0;
    cv::Rect maxAreaRgn;
    cv::Point2i maxAreaCenter;
    
    // 領域をスライドして面積が最大となる領域を探索
    for( int i = 0; i < longer - shorter; i++ ) {
        if( axis == 0 )
            region.x = i;
        else if( axis == 1 )
            region.y = i;
        
        double area;
        cv::Point2i center;
        CalcFaceArea(&area, &center, facesList, region);
        if( maxArea < area ) {
            maxArea = area;
            maxAreaRgn = region;
            maxAreaCenter = center;
        }
        
    }
    
    // 検出された顔領域の中心を切り抜くようにする
    if( axis == 0 ) {
        maxAreaRgn.x = Clamp(maxAreaCenter.x - shorter / 2, 0, shorter);
    } else if( axis == 1 ) {
        maxAreaRgn.y = Clamp(maxAreaCenter.y - shorter / 2, 0, shorter);
    }
    
    // 切り抜き範囲が元画像をはみ出さないようにする
    maxAreaRgn.x = RangedClamp(maxAreaRgn.x, shorter, 0, width - 1);
    maxAreaRgn.y = RangedClamp(maxAreaRgn.y, shorter, 0, height - 1);
    
    return maxAreaRgn;
    
}

int main(int argc, char *argv[]) {
    
    if( argc < 2 ) {
        std::cerr << "Too less arguments!";
        std::cerr << "Usage: cv3facecontaintrimming <image>";
        return 1;
    }
    
    std::string inImage = argv[1];
    cv::Mat img = cv::imread(inImage);
    
    cv::Mat imgDetect = img.clone();
    cv::cvtColor(imgDetect, imgDetect, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(imgDetect, imgDetect);
    
    int width = img.cols;
    int height = img.rows;
    int shorter = std::min(width, height);
    
    // 顔検出
    cv::CascadeClassifier classifier("lbpcascade_animeface.xml");
    std::vector<cv::Rect> vecFaceList;
    std::vector<int> rejectLevelList;
    std::vector<double> levelWeightList;
    classifier.detectMultiScale(
        imgDetect, vecFaceList,
        rejectLevelList, levelWeightList,
        1.01, 3, 0, cv::Size(shorter / 20, shorter / 20), cv::Size(), true);
    NormalizeList(std::begin(levelWeightList), std::end(levelWeightList));
    
    for( size_t i = 0; i < vecFaceList.size(); i++ ) {
        const cv::Rect &rgn = vecFaceList[i];
        double weight = levelWeightList[i];
        cv::rectangle(img, rgn, cv::Scalar(255 * weight, 0, 0), 2);
    }
    
    // 切り抜き領域探索
    cv::Rect trimRgn = SearchMaxFaceAreaTrimmingRegion(vecFaceList, width, height);
    cv::rectangle(img, trimRgn, cv::Scalar(0, 0, 255), 4);
    
    cv::resize(img, img, cv::Size(img.cols / 2, img.rows / 2));
    
    cv::imshow("img", img);
    cv::waitKey();
    
    return 0;
    
}

