
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "Util.h"
#include "cvlib.h"
#include "cmdparser.h"


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

// 画像から切り抜き領域を計算する
void CalcTrimRegion(
    cv::Rect *pOutRegion,
    std::vector<cv::Rect> *pOutFaceRegionsList,
    std::vector<double> *pOutLevelWeightList,
    const cv::Mat &image,
    cv::CascadeClassifier &classifier,
    double cascadeScaleFactor=1.1,
    int cascadeNumNeighbor=3,
    double minSizeRatio=0.0)
{
    
    cv::Mat imgDetect = image.clone();
    cv::cvtColor(imgDetect, imgDetect, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(imgDetect, imgDetect);
    
    int width = image.cols;
    int height = image.rows;
    int shorter = std::min(width, height);
    
    cv::Size minSize;
    if( 0.0 < minSizeRatio ) {
        // minSizeRatioが0.0より大きかった場合は，
        // 最小領域制限を行う
        minSize.width = (int)(shorter * minSizeRatio);
        minSize.height = (int)(shorter * minSizeRatio);
    }
    
    // 顔検出
    std::vector<cv::Rect> faceList;
    std::vector<int> rejectLevelList;
    std::vector<double> levelWeightList;
    classifier.detectMultiScale(
        imgDetect, faceList,
        rejectLevelList, levelWeightList,
        cascadeScaleFactor, cascadeNumNeighbor, 0,
        minSize, cv::Size(), true);
    
    NormalizeList(std::begin(levelWeightList), std::end(levelWeightList));
    
    // 切り抜き領域探索
    cv::Rect trimRgn = SearchMaxFaceAreaTrimmingRegion(faceList, width, height);
    
    if( pOutRegion ) *pOutRegion = trimRgn;
    if( pOutFaceRegionsList ) *pOutFaceRegionsList = faceList;
    if( pOutLevelWeightList ) *pOutLevelWeightList = levelWeightList;
    
}

int main(int argc, char *argv[]) {
    
    std::string myPath = GetMyDir();
    std::string myDir = myPath.substr(0, myPath.rfind('\\'));
    std::string defaultCascadePath = myDir + "\\lbpcascade_animeface.xml";
    
    CmdParser parser;
    parser.Add("cascade-xml", 'x', "OpenCV cascade xml file", true, defaultCascadePath);
    parser.Add("cascade-scale-factor", 's', "Cascade scale factor", true, "1.01");
    parser.Add("cascade-neighbor", 'n', "Cascade num neighbors", true, "3");
    parser.Add("cascade-min-ratio", 'm', "Cascade min size ratio", true, "0.05");
    parser.Add("input", 'i', "Input image", true);
    parser.Add("output", 'o', "Output trimmed image", true);
    parser.Add("help", 'h', "Show this help", false);
    parser.Parse(argc, argv);
    
    if( parser.Get<bool>("help") ) {
        std::cout << parser.GetHelp() << std::endl;
        return 0;
    }
    
    std::string inImage = parser.Get<std::string>("input");
    std::string outImage = parser.Get<std::string>("output");
    
    if( inImage.empty() || outImage.empty() ) {
        std::cerr << "--input and --output are must be specified!";
        return 1;
    }
    
    cv::Mat img = cv::imread(inImage);
    
    cv::CascadeClassifier classifier(
        parser.Get<std::string>("cascade-xml"));
    
    // 切り抜き領域計算
    cv::Rect trimRegion;
    std::vector<cv::Rect> vecFaceList;
    std::vector<double> levelWeightList;
    CalcTrimRegion(
        &trimRegion, &vecFaceList, &levelWeightList,
        img, classifier,
        parser.Get<double>("cascade-scale-factor"),
        parser.Get<int>("cascade-neighbor"),
        parser.Get<double>("cascade-min-ratio"));
    
    if( outImage == "#" ) {
        // ウィンドウに出力
        for( size_t i = 0; i < vecFaceList.size(); i++ ) {
            const cv::Rect &rgn = vecFaceList[i];
            double weight = levelWeightList[i];
            cv::rectangle(img, rgn, cv::Scalar(255 * weight, 0, 0), 2);
        }
        cv::rectangle(img, trimRegion, cv::Scalar(0, 0, 255), 4);
        cv::resize(img, img, cv::Size(img.cols / 2, img.rows / 2));
        
        cv::imshow("img", img);
        cv::waitKey();
        
    } else {
        // ファイルに出力
        cv::Mat trimmedImage(img, trimRegion);
        cv::imwrite(outImage, trimmedImage);
    }
    
    return 0;
    
}

