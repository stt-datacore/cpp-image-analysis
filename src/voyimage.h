#include <memory>

namespace DataCore {

struct ParsedSkill
{
	int skillValue;
	int primary;

	boost::property_tree::ptree toJson();
};

struct VoySearchResults
{
	int input_width;
	int input_height;
	std::string error;
	size_t fileSize;

	bool valid{false};
	int antimatter;
	ParsedSkill cmd;
	ParsedSkill dip;
	ParsedSkill eng;
	ParsedSkill med;
	ParsedSkill sci;
	ParsedSkill sec;

	boost::property_tree::ptree toJson();
};

struct IVoyImageScanner
{
	virtual bool ReInitialize(bool forceReTraining) = 0;
	virtual VoySearchResults AnalyzeVoyImage(const char *url) = 0;
};

std::shared_ptr<IVoyImageScanner> MakeVoyImageScanner(const std::string &basePath);

} // namespace DataCore