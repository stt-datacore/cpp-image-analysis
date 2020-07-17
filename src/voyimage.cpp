#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include <leptonica/allheaders.h>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>

#include "networkhelper.h"
#include "utils.h"
#include "voyimage.h"

namespace fs = std::filesystem;

namespace DataCore {

class VoyImageScanner : public IVoyImageScanner
{
  public:
	VoyImageScanner(const char *basePath) : _basePath(basePath)
	{
	}

	~VoyImageScanner();

	bool ReInitialize(bool forceReTraining) override;
	VoySearchResults AnalyzeVoyImage(const char *url) override;
	VoySearchResults AnalyzeVoyImage(cv::Mat query, size_t fileSize) override;

  private:
	int MatchTop(cv::Mat top);
	bool MatchBottom(cv::Mat bottom, VoySearchResults *result);
	int OCRNumber(cv::Mat SkillValue, const std::string &name = "");
	int HasStar(cv::Mat skillImg, const std::string &skillName = "");

	NetworkHelper _networkHelper;
	std::shared_ptr<tesseract::TessBaseAPI> _tesseract;

	cv::Mat _skill_cmd;
	cv::Mat _skill_dip;
	cv::Mat _skill_eng;
	cv::Mat _skill_med;
	cv::Mat _skill_sci;
	cv::Mat _skill_sec;
	cv::Mat _antimatter;

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
	_skill_cmd = cv::imread(fs::path(_basePath + "data/cmd.png").make_preferred().string());
	_skill_dip = cv::imread(fs::path(_basePath + "data/dip.png").make_preferred().string());
	_skill_eng = cv::imread(fs::path(_basePath + "data/eng.png").make_preferred().string());
	_skill_med = cv::imread(fs::path(_basePath + "data/med.png").make_preferred().string());
	_skill_sci = cv::imread(fs::path(_basePath + "data/sci.png").make_preferred().string());
	_skill_sec = cv::imread(fs::path(_basePath + "data/sec.png").make_preferred().string());
	_antimatter = cv::imread(fs::path(_basePath + "data/antimatter.png").make_preferred().string());

	_tesseract = std::make_shared<tesseract::TessBaseAPI>();

	if (_tesseract->Init(fs::path(_basePath + "data/tessdata").make_preferred().string().c_str(), "Eurostile")) {
		// "Could not initialize tesseract"
		return false;
	}

	//_tesseract->DefaultPageSegMode = PageSegMode.SingleWord;

	_tesseract->SetVariable("tessedit_char_whitelist", "0123456789");
	_tesseract->SetVariable("classify_bln_numeric_mode", "1");

	return true;
}

double ScaleInvariantTemplateMatch(cv::Mat refMat, cv::Mat tplMat, cv::Point *maxloc, double threshold = 0.8)
{
	cv::Mat res(refMat.rows - tplMat.rows + 1, refMat.cols - tplMat.cols + 1, CV_32FC1);

	// Threshold out the faded stars
	cv::threshold(refMat, refMat, 100, 1, cv::THRESH_TOZERO);

	cv::matchTemplate(refMat, tplMat, res, cv::TM_CCORR_NORMED);
	cv::threshold(res, res, threshold, 1, cv::THRESH_TOZERO);

	double minval, maxval;
	cv::Point minloc;
	cv::minMaxLoc(res, &minval, &maxval, &minloc, maxloc);

	return maxval;
}

int VoyImageScanner::OCRNumber(cv::Mat SkillValue, const std::string &name)
{
	_tesseract->SetImage((uchar *)SkillValue.data, SkillValue.size().width, SkillValue.size().height, SkillValue.channels(),
						 (int)SkillValue.step1());
	_tesseract->Recognize(0);
	const char *out = _tesseract->GetUTF8Text();

	// std::cout << "For " << name << "OCR got " << out << std::endl;

	return std::atoi(out);
}

int VoyImageScanner::HasStar(cv::Mat skillImg, const std::string &skillName)
{
	cv::Mat center =
		SubMat(skillImg, (skillImg.rows / 2) - 10, (skillImg.rows / 2) + 10, (skillImg.cols / 2) - 10, (skillImg.cols / 2) + 10);

	auto mean = cv::mean(center);

	if (mean.val[0] + mean.val[1] + mean.val[2] < 10) {
		return 0;
	} else if (mean.val[0] < 5) {
		return 1; // Primary
	} else if (mean.val[0] + mean.val[1] + mean.val[2] > 100) {
		return 2; // Secondary
	} else {
		// not sure... hmmm
		return -1;
	}
}

bool VoyImageScanner::MatchBottom(cv::Mat bottom, VoySearchResults *result)
{
	int minHeight = bottom.rows * 3 / 15;
	int maxHeight = bottom.rows * 5 / 15;
	int stepHeight = bottom.rows / 30;

	cv::Point maxlocCmd;
	cv::Point maxlocSci;
	int scaledWidth = 0;
	int height = minHeight;
	for (; height <= maxHeight; height += stepHeight) {
		cv::Mat scaledCmd;
		cv::resize(_skill_cmd, scaledCmd, cv::Size(_skill_cmd.cols * height / _skill_cmd.rows, height), 0, 0, cv::INTER_AREA);
		cv::Mat scaledSci;
		cv::resize(_skill_sci, scaledSci, cv::Size(_skill_sci.cols * height / _skill_sci.rows, height), 0, 0, cv::INTER_AREA);

		double maxvalCmd = ScaleInvariantTemplateMatch(bottom, scaledCmd, &maxlocCmd);
		double maxvalSci = ScaleInvariantTemplateMatch(bottom, scaledSci, &maxlocSci);

		if ((maxvalCmd > 0.9) && (maxvalSci > 0.9)) {
			scaledWidth = scaledSci.cols;
			break;
		}
	}

	if (scaledWidth == 0) {
		return false;
	}

	double widthScale = (double)scaledWidth / _skill_sci.cols;

	result->cmd.SkillValue = OCRNumber(
		SubMat(bottom, maxlocCmd.y, maxlocCmd.y + height, maxlocCmd.x - (scaledWidth * 5), maxlocCmd.x - (scaledWidth / 8)), "cmd");
	result->cmd.Primary = HasStar(
		SubMat(bottom, maxlocCmd.y, maxlocCmd.y + height, maxlocCmd.x + (scaledWidth * 9 / 8), maxlocCmd.x + (scaledWidth * 5 / 2)), "cmd");

	result->dip.SkillValue = OCRNumber(SubMat(bottom, maxlocCmd.y + height, maxlocSci.y, maxlocCmd.x - (scaledWidth * 5),
											  (int)(maxlocCmd.x - (_skill_dip.cols - _skill_sci.cols) * widthScale)),
									   "dip");
	result->dip.Primary = HasStar(
		SubMat(bottom, maxlocCmd.y + height, maxlocSci.y, maxlocCmd.x + (scaledWidth * 9 / 8), maxlocCmd.x + (scaledWidth * 5 / 2)), "dip");

	result->eng.SkillValue = OCRNumber(SubMat(bottom, maxlocSci.y, maxlocSci.y + height, maxlocCmd.x - (scaledWidth * 5),
											  (int)(maxlocCmd.x - (_skill_eng.cols - _skill_sci.cols) * widthScale)),
									   "eng");
	result->eng.Primary = HasStar(
		SubMat(bottom, maxlocSci.y, maxlocSci.y + height, maxlocCmd.x + (scaledWidth * 9 / 8), maxlocCmd.x + (scaledWidth * 5 / 2)), "eng");

	result->sec.SkillValue = OCRNumber(
		SubMat(bottom, maxlocCmd.y, maxlocCmd.y + height, (int)(maxlocSci.x + scaledWidth * 1.4), maxlocSci.x + (scaledWidth * 6)), "sec");
	result->sec.Primary = HasStar(
		SubMat(bottom, maxlocCmd.y, maxlocCmd.y + height, maxlocSci.x - (scaledWidth * 12 / 8), maxlocSci.x - (scaledWidth / 6)), "sec");

	result->med.SkillValue = OCRNumber(
		SubMat(bottom, maxlocCmd.y + height, maxlocSci.y, (int)(maxlocSci.x + scaledWidth * 1.4), maxlocSci.x + (scaledWidth * 6)), "med");
	result->med.Primary = HasStar(
		SubMat(bottom, maxlocCmd.y + height, maxlocSci.y, maxlocSci.x - (scaledWidth * 12 / 8), maxlocSci.x - (scaledWidth / 6)), "med");

	result->sci.SkillValue = OCRNumber(
		SubMat(bottom, maxlocSci.y, maxlocSci.y + height, (int)(maxlocSci.x + scaledWidth * 1.4), maxlocSci.x + (scaledWidth * 6)), "sci");
	result->sci.Primary = HasStar(
		SubMat(bottom, maxlocSci.y, maxlocSci.y + height, maxlocSci.x - (scaledWidth * 12 / 8), maxlocSci.x - (scaledWidth / 6)), "sci");

	return true;
}

int VoyImageScanner::MatchTop(cv::Mat top)
{
	int minHeight = top.rows / 4;
	int maxHeight = top.rows / 2;
	int stepHeight = top.rows / 32;

	cv::Point maxloc;
	int scaledWidth = 0;
	int height = minHeight;
	for (; height <= maxHeight; height += stepHeight) {
		cv::Mat scaled;
		cv::resize(_antimatter, scaled, cv::Size(_antimatter.cols * height / _antimatter.rows, height), 0, 0, cv::INTER_AREA);

		double maxval = ScaleInvariantTemplateMatch(top, scaled, &maxloc);

		if (maxval > 0.8) {
			scaledWidth = scaled.cols;
			break;
		}
	}

	if (scaledWidth == 0) {
		return 0;
	}

	top = SubMat(top, maxloc.y, maxloc.y + height, maxloc.x + scaledWidth, maxloc.x + (int)(scaledWidth * 6.75));
	// imwrite("temp.png", top);

	return OCRNumber(top);
}

VoySearchResults VoyImageScanner::AnalyzeVoyImage(const char *url)
{
	size_t fileSize;
	cv::Mat query;
	_networkHelper.downloadUrl(url, [&](std::vector<uint8_t> &&v) -> bool {
		query = cv::imdecode(v, cv::IMREAD_UNCHANGED);
		fileSize = v.size();
		return true;
	});

	return AnalyzeVoyImage(query, fileSize);
}

VoySearchResults VoyImageScanner::AnalyzeVoyImage(cv::Mat query, size_t fileSize)
{
	// Some images are encoded with 2 bytes per channel, scale down for template matching to work
	if (query.depth() == CV_16U) {
		// convert to 1-byte
		query.convertTo(query, CV_8U, 0.00390625);

		if (query.type() == CV_8UC4) {
			// If the image also has an alpha channel, remove it!
			cv::Mat dst;
			cv::cvtColor(query, dst, cv::COLOR_BGRA2BGR);
			query = dst;
		}
	}

	VoySearchResults result;
	result.fileSize = fileSize;
	result.input_height = query.rows;
	result.input_width = query.cols;

	try {
		// First, take the top of the image and look for the antimatter
		cv::Mat top = SubMat(query, 0, std::max(query.rows / 5, 80), query.cols / 3, query.cols * 2 / 3);
		cv::threshold(top, top, 100, 1, cv::THRESH_TOZERO);

		result.antimatter = MatchTop(top);

		if (result.antimatter == 0) {
			result.error = "Could not read antimatter";
			return result;
		}

		// Sometimes the OCR reads an extra 0 if there's a "particle" in exactly the
		// wrong spot
		if (result.antimatter > 8000) {
			result.antimatter = result.antimatter / 10;
		}

		double standardScale = (double)query.cols / query.rows;
		double scaledPercentage = query.rows * (standardScale * 1.2) / 9;

		cv::Mat bottom = SubMat(query, (int)(query.rows - scaledPercentage), query.rows, query.cols / 6, query.cols * 5 / 6);

		cv::threshold(bottom, bottom, 100, 1, cv::THRESH_TOZERO);

		if (!MatchBottom(bottom, &result)) {
			// Not found
			result.error = "Could not read skill values";
			return result;
		}

		result.valid = true;
	} catch (std::exception const &e) {
		result.error = std::string("Exception: ") + e.what();
		return result;
	}

	return result;
}

std::shared_ptr<IVoyImageScanner> MakeVoyImageScanner(const std::string &basePath)
{
	return std::make_shared<VoyImageScanner>(basePath.c_str());
}

} // namespace DataCore