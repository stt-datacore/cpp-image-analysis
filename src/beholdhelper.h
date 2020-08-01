#include <memory>
#include <string>

#include "json.hpp"

namespace DataCore {

struct MatchResult
{
	std::string symbol;
	int score{0};
	uint8_t starcount{0};
};

inline void to_json(nlohmann::json &j, const MatchResult &m)
{
	j = nlohmann::json{{"symbol", m.symbol}, {"score", m.score}, {"stars", m.starcount}};
}

inline void from_json(const nlohmann::json &j, MatchResult &m)
{
	j.at("symbol").get_to(m.symbol);
	j.at("score").get_to(m.score);
	j.at("starcount").get_to(m.starcount);
}

struct SearchResults
{
	int input_width;
	int input_height;
	MatchResult top;
	MatchResult crew1;
	MatchResult crew2;
	MatchResult crew3;
	std::string error;
	int closebuttons{0};
	size_t fileSize;
};

inline void to_json(nlohmann::json &j, const SearchResults &s)
{
	j = nlohmann::json{{"input_width", s.input_width},
					   {"input_height", s.input_height},
					   {"top", s.top},
					   {"crew1", s.crew1},
					   {"crew2", s.crew2},
					   {"crew3", s.crew3},
					   {"error", s.error},
					   {"fileSize", s.fileSize},
					   {"closebuttons", s.closebuttons}};
}

inline void from_json(const nlohmann::json &j, SearchResults &s)
{
	j.at("input_width").get_to(s.input_width);
	j.at("input_height").get_to(s.input_height);
	j.at("top").get_to(s.top);
	j.at("crew1").get_to(s.crew1);
	j.at("crew2").get_to(s.crew2);
	j.at("crew3").get_to(s.crew3);
	j.at("error").get_to(s.error);
	j.at("fileSize").get_to(s.fileSize);
	j.at("closebuttons").get_to(s.closebuttons);
}

struct IBeholdHelper
{
	virtual bool ReInitialize(bool forceReTraining, const std::string &jsonpath, const std::string &asseturl) = 0;
	virtual SearchResults AnalyzeBehold(const char *url) = 0;
	virtual SearchResults AnalyzeBehold(cv::Mat query, size_t fileSize) = 0;
};

std::shared_ptr<IBeholdHelper> MakeBeholdHelper(const std::string &basePath);

} // namespace DataCore
