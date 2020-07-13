#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "wsserver.h"
#include "beholdhelper.h"
#include "voyimage.h"

int main(int argc, char** argv)
{
	std::string basePath{ "../../../" };
	if (argc > 1) {
		basePath = argv[1];
	}

	std::shared_ptr<IBeholdHelper> beholdHelper = MakeBeholdHelper(basePath);
	std::shared_ptr<IVoyImageScanner> voyImageScanner = MakeVoyImageScanner(basePath);

	// Load all matrices from disk
	beholdHelper->ReInitialize(false);

	// Initialize the Tesseract OCR engine
	voyImageScanner->ReInitialize(false);

	std::cout << "Ready!" << std::endl;

	// Blocking
	start_server([&](std::string&& message) -> std::string {
		std::cout << "Message received: " << message << std::endl;

		// TODO: there's probably a better / smarter way to implement a protocol handler
		boost::property_tree::ptree pt;
		if (message.find("REINIT") == 0) {
			// Reinitialize by reloading assets.json from the configured path
			beholdHelper->ReInitialize(false);
			pt.put("success", true);
		}
		else if (message.find("FORCEREINIT") == 0) {
			// Force reinitialize by re-downloading and re-parsing all assets
			beholdHelper->ReInitialize(true);
			pt.put("success", true);
		}
		else if (message.find("BEHOLD") == 0) {
			// Run the behold analyzer
			std::string beholdUrl = message.substr(6);

			SearchResults results = beholdHelper->AnalyzeBehold(beholdUrl.c_str());

			pt.put("beholdUrl", beholdUrl);
			pt.put_child("results", results.toJson());
			pt.put("success", true);
		}
		else if (message.find("VOYIMAGE") == 0) {
			// Run the behold analyzer
			std::string voyImageUrl = message.substr(8);

			VoySearchResults results = voyImageScanner->AnalyzeVoyImage(voyImageUrl.c_str());

			pt.put("voyImageUrl", voyImageUrl);
			pt.put_child("results", results.toJson());
			pt.put("success", true);
		}
		else {
			// unknown message
			pt.put("success", false);
		}

		std::stringstream ss;
		boost::property_tree::json_parser::write_json(ss, pt);

		return ss.str();
	});

	return 0;
}
