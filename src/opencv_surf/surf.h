///////////// see LICENSE.txt in the OpenCV root directory //////////////

// This is a local copy of the SURF algorithm from opencv_contrib, to avoid needing to compile the entire opencv from source

#include <opencv2/features2d.hpp>

namespace cv
{
namespace xxfeatures2d
{

class CV_EXPORTS_W SURF : public Feature2D
{
public:
    /**
    @param hessianThreshold Threshold for hessian keypoint detector used in SURF.
    @param nOctaves Number of pyramid octaves the keypoint detector will use.
    @param nOctaveLayers Number of octave layers within each octave.
    @param extended Extended descriptor flag (true - use extended 128-element descriptors; false - use
    64-element descriptors).
    @param upright Up-right or rotated features flag (true - do not compute orientation of features;
    false - compute orientation).
        */
    CV_WRAP static Ptr<SURF> create(double hessianThreshold = 100,
        int nOctaves = 4, int nOctaveLayers = 3,
        bool extended = false, bool upright = false);

    CV_WRAP virtual void setHessianThreshold(double hessianThreshold) = 0;
    CV_WRAP virtual double getHessianThreshold() const = 0;

    CV_WRAP virtual void setNOctaves(int nOctaves) = 0;
    CV_WRAP virtual int getNOctaves() const = 0;

    CV_WRAP virtual void setNOctaveLayers(int nOctaveLayers) = 0;
    CV_WRAP virtual int getNOctaveLayers() const = 0;

    CV_WRAP virtual void setExtended(bool extended) = 0;
    CV_WRAP virtual bool getExtended() const = 0;

    CV_WRAP virtual void setUpright(bool upright) = 0;
    CV_WRAP virtual bool getUpright() const = 0;
};

typedef SURF SurfFeatureDetector;
typedef SURF SurfDescriptorExtractor;

//! Speeded up robust features, port from CUDA module.
////////////////////////////////// SURF //////////////////////////////////////////
/*!
 SURF implementation.

 The class implements SURF algorithm by H. Bay et al.
 */
class SURF_Impl : public SURF
{
public:
    //! the full constructor taking all the necessary parameters
    explicit CV_WRAP SURF_Impl(double hessianThreshold,
                               int nOctaves = 4, int nOctaveLayers = 2,
                               bool extended = true, bool upright = false);

    //! returns the descriptor size in float's (64 or 128)
    CV_WRAP int descriptorSize() const CV_OVERRIDE;

    //! returns the descriptor type
    CV_WRAP int descriptorType() const CV_OVERRIDE;

    //! returns the descriptor type
    CV_WRAP int defaultNorm() const CV_OVERRIDE;

    void set(int, double);
    double get(int) const;

    //! finds the keypoints and computes their descriptors.
    // Optionally it can compute descriptors for the user-provided keypoints
    void detectAndCompute(InputArray img, InputArray mask,
                          CV_OUT std::vector<KeyPoint>& keypoints,
                          OutputArray descriptors,
                          bool useProvidedKeypoints = false) CV_OVERRIDE;

    void setHessianThreshold(double hessianThreshold_) CV_OVERRIDE { hessianThreshold = hessianThreshold_; }
    double getHessianThreshold() const CV_OVERRIDE { return hessianThreshold; }

    void setNOctaves(int nOctaves_) CV_OVERRIDE { nOctaves = nOctaves_; }
    int getNOctaves() const CV_OVERRIDE { return nOctaves; }

    void setNOctaveLayers(int nOctaveLayers_) CV_OVERRIDE { nOctaveLayers = nOctaveLayers_; }
    int getNOctaveLayers() const CV_OVERRIDE { return nOctaveLayers; }

    void setExtended(bool extended_) CV_OVERRIDE { extended = extended_; }
    bool getExtended() const CV_OVERRIDE { return extended; }

    void setUpright(bool upright_) CV_OVERRIDE { upright = upright_; }
    bool getUpright() const CV_OVERRIDE { return upright; }

    double hessianThreshold;
    int nOctaves;
    int nOctaveLayers;
    bool extended;
    bool upright;
};

}
}
