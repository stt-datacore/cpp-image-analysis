#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "networkhelper.h"
#include "voyimage.h"

using namespace cv;
using namespace cv::xfeatures2d;

class VoyImageScanner : IVoyImageScanner
{
public:
	VoyImageScanner(const char* basePath)
		: _basePath(basePath)
	{
	}

	~VoyImageScanner();

	bool ReInitialize(bool forceReTraining) override;
	bool AnalyzeVoyImage(const char* url) override;

private:
	NetworkHelper _networkHelper;
	std::shared_ptr<tesseract::TessBaseAPI> _tesseract;

	Mat _skill_cmd;
	Mat _skill_dip;
	Mat _skill_eng;
	Mat _skill_med;
	Mat _skill_sci;
	Mat _skill_sec;
	Mat _antimatter;

	std::string _basePath;
};

VoyImageScanner::~VoyImageScanner()
{
	if (_tesseract) {
		_tesseract->End();
		_tesseract.reset();
	}
}

bool VoyImageScanner::ReInitialize(bool forceReTraining)
{
	_skill_cmd = cv::imread(_basePath + "data\\cmd.png");
	_skill_dip = cv::imread(_basePath + "data\\dip.png");
	_skill_eng = cv::imread(_basePath + "data\\eng.png");
	_skill_med = cv::imread(_basePath + "data\\med.png");
	_skill_sci = cv::imread(_basePath + "data\\sci.png");
	_skill_sec = cv::imread(_basePath + "data\\sec.png");
	_antimatter = cv::imread(_basePath + "data\\antimatter.png");

	_tesseract = std::make_shared<tesseract::TessBaseAPI>();
	// Initialize tesseract-ocr with English, without specifying tessdata path
	if (_tesseract->Init((_basePath + "data\\tessdata").c_str(), "Eurostile")) {
		// "Could not initialize tesseract"
		return false;
	}

	//_tesseract->DefaultPageSegMode = PageSegMode.SingleWord;

	_tesseract->SetVariable("tessedit_char_whitelist", "0123456789");
	_tesseract->SetVariable("classify_bln_numeric_mode", "1");

	return true;
}

bool VoyImageScanner::AnalyzeVoyImage(const char* url)
{
	// TODO: implement me (see https://github.com/stt-datacore/image-analysis/blob/master/src/DataCore.Library/AIMagic/VoyImage.cs)
	return true;
}

