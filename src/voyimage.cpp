#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <map>

#include <opencv2/opencv.hpp>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "networkhelper.h"
#include "voyimage.h"

namespace fs = std::filesystem;
using namespace cv;

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
	_skill_cmd = cv::imread(fs::path(_basePath + "data\\cmd.png").make_preferred().string());
	_skill_dip = cv::imread(fs::path(_basePath + "data\\dip.png").make_preferred().string());
	_skill_eng = cv::imread(fs::path(_basePath + "data\\eng.png").make_preferred().string());
	_skill_med = cv::imread(fs::path(_basePath + "data\\med.png").make_preferred().string());
	_skill_sci = cv::imread(fs::path(_basePath + "data\\sci.png").make_preferred().string());
	_skill_sec = cv::imread(fs::path(_basePath + "data\\sec.png").make_preferred().string());
	_antimatter = cv::imread(fs::path(_basePath + "data\\antimatter.png").make_preferred().string());

	_tesseract = std::make_shared<tesseract::TessBaseAPI>();

	if (_tesseract->Init(fs::path(_basePath + "data\\tessdata").make_preferred().string().c_str(), "Eurostile")) {
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

