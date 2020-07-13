#include <memory>

struct MatchResult
{
	std::string symbol;
	int score;
	uint8_t starcount;

	boost::property_tree::ptree toJson();
};

struct SearchResults
{
	int input_width;
	int input_height;
	MatchResult top;
	MatchResult crew1;
	MatchResult crew2;
	MatchResult crew3;
	std::string error;
	int closebuttons;
	size_t fileSize;

	boost::property_tree::ptree toJson();
};

struct IBeholdHelper /* : std::enable_shared_from_this<IBeholdHelper>*/
{
	virtual bool ReInitialize(bool forceReTraining) = 0;
	virtual SearchResults AnalyzeBehold(const char* url) = 0;
};

std::shared_ptr<IBeholdHelper> MakeBeholdHelper(const std::string& basePath);
