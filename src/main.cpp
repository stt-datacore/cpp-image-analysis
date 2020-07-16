#include <iostream>

#include <opencv2/opencv.hpp>

#include "beholdhelper.h"
#include "httpserver.h"
#include "networkhelper.h"
#include "voyimage.h"
#include "wsserver.h"

#include "json.hpp"

using namespace DataCore;

int main(int argc, char **argv)
{
	std::string basePath{"../../../"};
	if (argc > 1) {
		basePath = argv[1];
	}

	NetworkHelper networkHelper;
	std::shared_ptr<IBeholdHelper> beholdHelper = MakeBeholdHelper(basePath);
	std::shared_ptr<IVoyImageScanner> voyImageScanner = MakeVoyImageScanner(basePath);

	// Load all matrices from disk
	beholdHelper->ReInitialize(false);

	// Initialize the Tesseract OCR engine
	voyImageScanner->ReInitialize(false);

	std::cout << "Ready!" << std::endl;

	// Blocking
	start_http_server([&](std::string &&message) -> std::string {
		std::cout << "Message received: " << message << std::endl;

		// TODO: there's probably a better / smarter way to implement a protocol
		// handler
		nlohmann::json j;
		if (message.find("REINIT") == 0) {
			// Reinitialize by reloading assets.json from the configured path
			beholdHelper->ReInitialize(false);
			j["success"] = true;
		} else if (message.find("FORCEREINIT") == 0) {
			// Force reinitialize by re-downloading and re-parsing all assets
			beholdHelper->ReInitialize(true);
			j["success"] = true;
		} else if (message.find("BEHOLD") == 0) {
			// Run the behold analyzer
			std::string beholdUrl = message.substr(6);

			SearchResults results = beholdHelper->AnalyzeBehold(beholdUrl.c_str());
			j["beholdUrl"] = beholdUrl;
			j["results"] = results;
			j["success"] = true;

		} else if (message.find("VOYIMAGE") == 0) {
			// Run the behold analyzer
			std::string voyImageUrl = message.substr(8);

			VoySearchResults results = voyImageScanner->AnalyzeVoyImage(voyImageUrl.c_str());

			j["voyImageUrl"] = voyImageUrl;
			j["results"] = results;
			j["success"] = true;
		} else if (message.find("BOTH") == 0) {
			// Run both analyzers
			std::string url = message.substr(4);

			size_t fileSize;
			cv::Mat query;
			networkHelper.downloadUrl(url, [&](std::vector<uint8_t> &&v) -> bool {
				query = cv::imdecode(v, cv::IMREAD_UNCHANGED);
				fileSize = v.size();
				return true;
			});

			VoySearchResults voyResult = voyImageScanner->AnalyzeVoyImage(query, fileSize);
			SearchResults beholdResult = beholdHelper->AnalyzeBehold(query, fileSize);

			j["url"] = url;
			j["beholdResult"] = beholdResult;
			j["voyResult"] = voyResult;
			j["success"] = true;
		} else {
			// unknown message
			j["success"] = false;
		}

		return j.dump();
	});

	return 0;
}
