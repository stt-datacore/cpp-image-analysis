#include <memory>

struct IVoyImageScanner
{
	virtual bool ReInitialize(bool forceReTraining) = 0;
	virtual bool AnalyzeVoyImage(const char* url) = 0;
};

std::shared_ptr<IVoyImageScanner> MakeVoyImageScanner(const char* basePath);
