#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>

#include <opencv2/opencv.hpp>

#include "beholdhelper.h"
#include "networkhelper.h"
#include "opencv_surf/surf.h"
#include "utils.h"

namespace fs = std::filesystem;

namespace DataCore {

// function to retrieve the image as cv::Mat data type
cv::Mat curlImg(const char *img_url)
{
	static NetworkHelper networkHelper;

	cv::Mat query;
	networkHelper.downloadUrl(img_url, [&](std::vector<uint8_t> &&v) -> bool {
		query = cv::imdecode(v, cv::IMREAD_UNCHANGED);
		return true;
	});

	return query;
}

bool fileExists(const std::string &fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

void matwrite(const std::string &filename, const cv::Mat &mat)
{
	std::ofstream fs(filename, std::fstream::binary);

	// Header
	int type = mat.type();
	int channels = mat.channels();
	fs.write((char *)&mat.rows, sizeof(int)); // rows
	fs.write((char *)&mat.cols, sizeof(int)); // cols
	fs.write((char *)&type, sizeof(int));	  // type
	fs.write((char *)&channels, sizeof(int)); // channels

	// Data
	if (mat.isContinuous()) {
		fs.write(mat.ptr<char>(0), (mat.dataend - mat.datastart));
	} else {
		int rowsz = CV_ELEM_SIZE(type) * mat.cols;
		for (int r = 0; r < mat.rows; ++r) {
			fs.write(mat.ptr<char>(r), rowsz);
		}
	}
}

cv::Mat matread(const std::string &filename)
{
	std::ifstream fs(filename, std::fstream::binary);

	// Header
	int rows, cols, type, channels;
	fs.read((char *)&rows, sizeof(int));	 // rows
	fs.read((char *)&cols, sizeof(int));	 // cols
	fs.read((char *)&type, sizeof(int));	 // type
	fs.read((char *)&channels, sizeof(int)); // channels

	// Data
	cv::Mat mat(rows, cols, type);
	fs.read((char *)mat.data, CV_ELEM_SIZE(type) * rows * cols);

	return mat;
}

class Descriptor
{
  public:
	Descriptor()
	{
		_detector = cv::xxfeatures2d::SURF::create(1200, 4 /*nOctaves*/, 3 /*nOctaveLayers*/, false /*extended*/, true /*upright*/);
	}

	cv::Mat Describe(cv::InputArray image)
	{
		std::vector<cv::KeyPoint> keypoints;
		cv::Mat descriptors;

		_detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

		return descriptors;
	}

  private:
	cv::Ptr<cv::xxfeatures2d::SURF> _detector;
};

class Searcher
{
  public:
	Searcher()
	{
		Clear();
	}

	void Clear()
	{
		_matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
	}

	bool Add(cv::Mat features, const char *symbol)
	{
		_matcher->add(features);
		_symbols.push_back(symbol);
		return true;
	}

	MatchResult Match(cv::Mat image)
	{
		cv::Mat features = _descriptor.Describe(image);

		std::vector<cv::DMatch> matches;
		_matcher->match(features, matches);

		if (matches.size() == 0) {
			// No matches
			return {"NO_MATCH", 0};
		}

		// group by image index
		std::map<int, int> occurences;
		for (const auto &match : matches) {
			occurences[match.imgIdx]++;
		}

		auto max = std::max_element(
			begin(occurences), end(occurences),
			[](const decltype(occurences)::value_type &p1, const decltype(occurences)::value_type &p2) { return p1.second < p2.second; });

		return {_symbols[max->first], max->second};
	}

  private:
	Descriptor _descriptor;
	cv::Ptr<cv::DescriptorMatcher> _matcher;

	// Order here must match order of training in matcher (as it deals in indices
	// only)
	std::vector<std::string> _symbols;
};

class Trainer
{
  public:
	Trainer(const char *trainPath) : _trainPath(trainPath)
	{
	}

	cv::Mat Read(const char *symbol)
	{
		std::stringstream outPath;
		outPath << _trainPath << symbol << ".bin";

		cv::Mat result = matread(fs::path(outPath.str()).make_preferred().string());

		return result;
	}

	bool Train(const char *imgUrl, const char *symbol, bool forceReTraining)
	{
		std::stringstream outPath;
		outPath << _trainPath << symbol << ".bin";

		// if file already exists, bail
		if (!forceReTraining && fileExists(fs::path(outPath.str()).make_preferred().string()))
			return true;

		cv::Mat image = curlImg(imgUrl);

		return TrainInternal(image, symbol, forceReTraining);
	}

	bool TrainInternal(cv::Mat image, const char *symbol, bool forceReTraining)
	{
		std::stringstream outPath;
		outPath << _trainPath << symbol << ".bin";

		// if file already exists, bail
		if (!forceReTraining && fileExists(fs::path(outPath.str()).make_preferred().string()))
			return true;

		if (image.empty())
			return false;

		// crop the image
		image = image(cv::Rect(0, 0, image.cols, image.rows * 7 / 10));

		auto features = _descriptor.Describe(image);

		if (features.cols > 0 && features.rows > 0) {
			matwrite(outPath.str(), features);
		}

		return true;
	}

  private:
	Descriptor _descriptor;
	std::string _trainPath;
};

class BeholdHelper : public IBeholdHelper
{
  public:
	BeholdHelper(const char *trainPath, const char *dataPath) : _dataPath(dataPath), _trainer(trainPath)
	{
	}

	bool ReInitialize(bool forceReTraining, const std::string &jsonpath, const std::string &asseturl) override;
	SearchResults AnalyzeBehold(const char *url) override;
	SearchResults AnalyzeBehold(cv::Mat query, size_t fileSize) override;

  private:
	int CountFullStars(cv::Mat refMat, cv::Mat tplMat, double threshold = 0.8) noexcept;

	Trainer _trainer;
	Searcher _searcher;
	NetworkHelper _networkHelper;

	cv::Mat _starFull;
	cv::Mat _closeButton;
	cv::Mat _beholdTitle;

	std::string _dataPath;
};

int BeholdHelper::CountFullStars(cv::Mat refMat, cv::Mat tplMat, double threshold) noexcept
{
	try {
		cv::Mat res(refMat.rows - tplMat.rows + 1, refMat.cols - tplMat.cols + 1, CV_32FC1);

		// Threshold out the faded stars
		cv::threshold(refMat, refMat, 100, 1.0, cv::ThresholdTypes::THRESH_TOZERO);
		cv::matchTemplate(refMat, tplMat, res, cv::TM_CCORR_NORMED);
		cv::threshold(res, res, threshold, 1.0, cv::ThresholdTypes::THRESH_TOZERO);

		int numStars = 0;
		while (true) {
			double minval, maxval;
			cv::Point minloc, maxloc;
			cv::minMaxLoc(res, &minval, &maxval, &minloc, &maxloc);

			if (maxval >= threshold) {
				numStars++;
				// Fill in the res Mat so we don't find the same area again in the
				// MinMaxLoc
				cv::Rect outRect;
				cv::floodFill(res, maxloc, cv::Scalar(0), &outRect, cv::Scalar(0.1), cv::Scalar(1.0));
			} else {
				break;
			}
		}

		return numStars;
	} catch (...) {
		return 0;
	}
}

bool BeholdHelper::ReInitialize(bool forceReTraining, const std::string &jsonpath, const std::string &asseturl)
{
	_starFull = cv::imread(fs::path(_dataPath + "starfull.png").make_preferred().string());
	_closeButton = cv::imread(fs::path(_dataPath + "closeButton.png").make_preferred().string());
	_beholdTitle = cv::imread(fs::path(_dataPath + "behold_title.png").make_preferred().string());

	_searcher.Clear();

	if (!_trainer.TrainInternal(_beholdTitle, "behold_title", forceReTraining))
		return false;

	cv::Mat tr = _trainer.Read("behold_title");
	_searcher.Add(tr, "behold_title");

	std::ifstream assetStream(fs::path(jsonpath + "crew.json").make_preferred().string());
	nlohmann::json j;
	assetStream >> j;

	for (auto &element : j) {
		if (element["max_rarity"].get<int>() >= 4) {
			std::string symbol = element["symbol"].get<std::string>();
			std::string url = asseturl + element["imageUrlFullBody"].get<std::string>();
			std::cout << "Reading " << symbol << "..." << std::endl;

			if (!_trainer.Train(url.c_str(), symbol.c_str(), forceReTraining))
				return false;

			cv::Mat tr = _trainer.Read(symbol.c_str());
			_searcher.Add(tr, symbol.c_str());
		}
	}

	std::ifstream assetStream2(fs::path(jsonpath + "ship_schematics.json").make_preferred().string());
	nlohmann::json j2;
	assetStream >> j2;

	for (auto &element : j2) {
		std::string symbol = element.at("ship")["model"].get<std::string>();
		std::string image = symbol;
		image = image.replace("model_", "schematics_");

		std::string url = asseturl + image + ".png";
		std::cout << "Reading " << symbol << "..." << std::endl;

		if (!_trainer.Train(url.c_str(), symbol.c_str(), forceReTraining))
			return false;

		cv::Mat tr = _trainer.Read(symbol.c_str());
		_searcher.Add(tr, symbol.c_str());
	}

	return true;
}

SearchResults BeholdHelper::AnalyzeBehold(const char *url)
{
	size_t fileSize;
	cv::Mat query;
	_networkHelper.downloadUrl(url, [&](std::vector<uint8_t> &&v) -> bool {
		query = cv::imdecode(v, cv::IMREAD_UNCHANGED);
		fileSize = v.size();
		return true;
	});

	// imwrite("temp.png", query);

	return AnalyzeBehold(query, fileSize);
}

SearchResults BeholdHelper::AnalyzeBehold(cv::Mat query, size_t fileSize)
{
	// Some images are encoded with 2 bytes per channel, scale down for template matching to work
	if (query.depth() == CV_16U) {
		// convert to 1-byte
		query.convertTo(query, CV_8U, 0.00390625);
	}

	// If the image has an alpha channel, remove it
	if (query.type() == CV_8UC4) {
		cv::Mat dst;
		cv::cvtColor(query, dst, cv::COLOR_BGRA2BGR);
		query = dst;
	}

	SearchResults results;
	results.fileSize = fileSize;
	results.input_height = query.rows;
	results.input_width = query.cols;

	cv::Mat top = SubMat(query, 0, std::min(query.rows / 13, 80), query.cols / 3, query.cols * 2 / 3);
	if (top.empty()) {
		results.error = "Top row was empty";
		return results;
	}

	if (top.rows < 48) {
		const auto topScale = 1.5;
		cv::resize(top, top, cv::Size((int)(top.cols * topScale), (int)(top.rows * topScale)), 0, 0, cv::INTER_AREA);
	}

	results.top = _searcher.Match(top);

	if (results.top.symbol != "behold_title") {
		results.error = "Top row doesn't look like a behold title"; // ignorable if other
																	// heuristics are high
	}

	// split in 3, search for each separately
	cv::Mat crew1 = SubMat(query, query.rows * 2 / 8, (int)(query.rows * 4.5 / 8), 30, query.cols / 3);
	cv::Mat crew2 = SubMat(query, query.rows * 2 / 8, (int)(query.rows * 4.5 / 8), query.cols * 1 / 3 + 30, query.cols * 2 / 3);
	cv::Mat crew3 = SubMat(query, query.rows * 2 / 8, (int)(query.rows * 4.5 / 8), query.cols * 2 / 3 + 30, query.cols - 30);

	results.crew1 = _searcher.Match(crew1);
	results.crew2 = _searcher.Match(crew2);
	results.crew3 = _searcher.Match(crew3);

	// imwrite("temp.png", crew1);

	// TODO: only do this part if valid (to not waste time)
	results.crew1.starcount = 0;
	results.crew2.starcount = 0;
	results.crew3.starcount = 0;
	int closebuttons = 0;
	if ((results.crew1.score > 0) && (results.crew2.score > 0) && (results.crew3.score > 0)) {
		int starScale = 72;
		float scale = (float)query.cols / 100;
		cv::Mat stars1 = SubMat(query, (int)(scale * 9.2), (int)(scale * 12.8), 30, query.cols / 3);
		cv::Mat stars2 = SubMat(query, (int)(scale * 9.2), (int)(scale * 12.8), query.cols * 1 / 3 + 30, query.cols * 2 / 3);
		cv::Mat stars3 = SubMat(query, (int)(scale * 9.2), (int)(scale * 12.8), query.cols * 2 / 3 + 30, query.cols - 30);

		cv::resize(stars1, stars1, cv::Size(stars1.cols * starScale / stars1.rows, starScale), 0, 0, cv::INTER_AREA);
		cv::resize(stars2, stars2, cv::Size(stars2.cols * starScale / stars2.rows, starScale), 0, 0, cv::INTER_AREA);
		cv::resize(stars3, stars3, cv::Size(stars3.cols * starScale / stars3.rows, starScale), 0, 0, cv::INTER_AREA);

		results.crew1.starcount = CountFullStars(stars1, _starFull);
		results.crew2.starcount = CountFullStars(stars2, _starFull);
		results.crew3.starcount = CountFullStars(stars3, _starFull);

		// If there's a close button, this isn't a behold
		int upperRightCorner = (int)(std::min(query.rows, query.cols) * 0.11);
		cv::Mat corner = SubMat(query, 0, upperRightCorner, query.cols - upperRightCorner, query.cols);
		cv::resize(corner, corner, cv::Size(78, 78), 0, 0, cv::INTER_AREA);
		results.closebuttons = CountFullStars(corner, _closeButton, 0.7);

		// TODO: If it kind-of looks like a behold (we get 2 valid crew out of 3),
		// special-case the "hidden / crouching" characters by looking at their
		// names
		cv::Mat name1 = SubMat(query, (int)(scale * 5.8), (int)(scale * 9.1), 30, query.cols / 3);
		cv::Mat name2 = SubMat(query, (int)(scale * 5.8), (int)(scale * 9.1), query.cols * 1 / 3 + 30, query.cols * 2 / 3);
		cv::Mat name3 = SubMat(query, (int)(scale * 5.8), (int)(scale * 9.1), query.cols * 2 / 3 + 30, query.cols - 30);
		// cv::imwrite("name1.png", name1);

		// TODO: OCR
	}

	return results;
}

std::shared_ptr<IBeholdHelper> MakeBeholdHelper(const std::string &trainPath, const std::string &dataPath)
{
	return std::make_shared<BeholdHelper>(trainPath.c_str(), dataPath.c_str());
}

} // namespace DataCore