#include "utils.h"

namespace DataCore {

cv::Mat SubMat(cv::Mat input, int rowStart, int rowEnd, int colStart, int colEnd)
{
	return input(cv::Rect(colStart, rowStart, colEnd - colStart, rowEnd - rowStart));
}

} // namespace DataCore