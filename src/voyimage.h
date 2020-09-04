#include <memory>

#include "json.hpp"

namespace DataCore {

struct ParsedSkill
{
	int SkillValue{0};
	int Primary{0};
};

inline void to_json(nlohmann::json &j, const ParsedSkill &p)
{
	j = nlohmann::json{{"SkillValue", p.SkillValue}, {"Primary", p.Primary}};
}

inline void from_json(const nlohmann::json &j, ParsedSkill &p)
{
	j.at("SkillValue").get_to(p.SkillValue);
	j.at("Primary").get_to(p.Primary);
}

struct VoySearchResults
{
	int input_width;
	int input_height;
	std::string error;
	size_t fileSize;

	bool valid{false};
	int antimatter{0};
	ParsedSkill cmd;
	ParsedSkill dip;
	ParsedSkill eng;
	ParsedSkill med;
	ParsedSkill sci;
	ParsedSkill sec;
};

inline void to_json(nlohmann::json &j, const VoySearchResults &s)
{
	j = nlohmann::json{{"input_width", s.input_width},
					   {"input_height", s.input_height},
					   {"cmd", s.cmd},
					   {"dip", s.dip},
					   {"eng", s.eng},
					   {"med", s.med},
					   {"sci", s.sci},
					   {"sec", s.sec},
					   {"antimatter", s.antimatter},
					   {"error", s.error},
					   {"fileSize", s.fileSize},
					   {"valid", s.valid}};
}

inline void from_json(const nlohmann::json &j, VoySearchResults &s)
{
	j.at("input_width").get_to(s.input_width);
	j.at("input_height").get_to(s.input_height);
	j.at("cmd").get_to(s.cmd);
	j.at("dip").get_to(s.dip);
	j.at("eng").get_to(s.eng);
	j.at("med").get_to(s.med);
	j.at("sci").get_to(s.sci);
	j.at("sec").get_to(s.sec);
	j.at("antimatter").get_to(s.antimatter);
	j.at("error").get_to(s.error);
	j.at("fileSize").get_to(s.fileSize);
	j.at("valid").get_to(s.valid);
}

struct IVoyImageScanner
{
	virtual bool ReInitialize(bool forceReTraining) = 0;
	virtual VoySearchResults AnalyzeVoyImage(const char *url) = 0;
	virtual VoySearchResults AnalyzeVoyImage(cv::Mat query, size_t fileSize) = 0;
};

std::shared_ptr<IVoyImageScanner> MakeVoyImageScanner(const std::string &dataPath);

} // namespace DataCore
