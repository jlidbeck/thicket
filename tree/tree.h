#pragma once

#include "util.h"
#include <opencv2/core/core.hpp>
#include <vector>
#include <queue>
#include <filesystem>

namespace fs = std::filesystem;


class qcanvas;

using std::vector;
using std::priority_queue;

using cv::Point2f;
typedef cv::Matx<float, 3, 3> Matx33;
typedef cv::Matx<float, 4, 1> Matx41;
typedef cv::Matx<float, 4, 4> Matx44;


class qcanvas
{
public:
    Matx33 globalTransform;
    cv::Mat image;

    qcanvas() {
    }

    void create(cv::Mat im)
    {
        image = im;
    }

    // sets global transform map to map provided domain to image, centered, vertically flipped
    void setScaleToFit(cv::Rect_<float> const &rect, float buffer)
    {
        if (image.empty()) throw std::exception("Image is empty");

        globalTransform = util::transform3x3::centerAndFit(rect, cv::Rect_<float>(0, 0, image.cols, image.rows), buffer, true);
    }

};


class qtransform
{
public:
    Matx33 transformMatrix;
    Matx44 colorTransform;
    double gestation;

public:

    qtransform(Matx33 const &transformMatrix_ = Matx33::eye(), Matx44 const &colorTransform_ = Matx44::eye(), double gestation_ = 1.0)
    {
        transformMatrix = transformMatrix_;
        colorTransform = colorTransform_;
        gestation = gestation_;
    }

    template<typename _Tp>
    qtransform(_Tp m00, _Tp m01, _Tp mtx, _Tp m10, _Tp m11, _Tp mty, Matx44 const &colorTransform_)
    {
        transformMatrix = Matx33(m00, m01, mtx, m10, m11, mty, 0, 0, 1);
        colorTransform = colorTransform_;
        gestation = 1.0;
    }

    qtransform(double angle, double scale, cv::Point2f translate)
    {
        transformMatrix = util::transform3x3::getRotationMatrix2D(Point2f(), angle, scale, translate.x, translate.y);
        colorTransform = colorTransform.eye();
        colorTransform(2, 2) = 0.94f;
        colorTransform(1, 1) = 0.96f;
        gestation = 1.0;
    }
};

namespace std
{
    template<typename _Tp>
    string to_string(std::vector<cv::Point_<_Tp> > const &polygon)
    {
        return std::string("[]");
    }

    template<typename _Tp, int m, int n>
    string to_string(cv::Matx<_Tp, m, n> const &mat)
    {
        auto sz = std::string("[ ");
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < n; ++j)
                sz += std::to_string(mat(i, j)) + (j < n - 1 ? ", " : ",\n  ");
        return sz + " ]";
    }

    template<typename _Tp>
    std::string to_string(qtransform const &t)
    {
        return std::string("{ transform: ") + to_string(t.transformMatrix)
            + ",\n    color: " + to_string(t.colorTransform)
            + ",\n    gestation: " + to_string(t.gestation)
            + "\n}";
    }
}


class qnode
{
public:
    double      beginTime;
    int         generation;
    Matx33      globalTransform;
    cv::Scalar  color = cv::Scalar(1, 0, 0, 1);

    qnode(double beginTime_ = 0)
    {
        beginTime = beginTime_;
        generation = 0;
        color = cv::Scalar(1, 1, 1, 1);
        globalTransform = globalTransform.eye();
    }

    inline float det() const { return (globalTransform(0, 0) * globalTransform(1, 1) - globalTransform(0, 1) * globalTransform(1, 0)); }

    inline bool operator!() const { return !( fabs(det()) > 1e-5 ); }

    void getPolyPoints(std::vector<cv::Point2f> const &points, std::vector<cv::Point2f> &transformedPoints) const
    {
        cv::transform(points, transformedPoints, globalTransform.get_minor<2, 3>(0, 0));
    }

    struct EarliestFirst
    {
        bool operator()(qnode const& a, qnode const& b)
        {
            return (a.beginTime > b.beginTime);
        }
    };

    struct BiggestFirst
    {
        bool operator()(qnode const& a, qnode const& b)
        {
            return (b.det() > a.det());
        }
    };
};


class qtree
{
public:
    // settings
    int maxRadius = 100;

    // same polygon for all nodes
    std::vector<cv::Point2f> polygon;
    std::vector<qtransform> transforms;
    double offspringTemporalRandomness = 100.0;
    
    std::priority_queue<qnode, std::deque<qnode>, qnode::EarliestFirst> nodeQueue;

public:
    qtree() {}

    virtual void randomizeSettings(int seed) { }
    virtual std::string getSettingsString() const
    {
        std::string sz = "{ transforms:[\n";
        for (auto &t : transforms)
            sz += std::to_string<float>(t) + ",\n";
        sz += "],\n  offspringTemporalRandomness:" + std::to_string(offspringTemporalRandomness)
            + ",\n  maxRadius:" + std::to_string(maxRadius)
            + ",\n  polygon:" + std::to_string(polygon)
            + "\n}";
        return sz;
    }

    virtual void create() = 0;

    // process the next node in the queue
    virtual bool process();

    // override to indicate that a child node should not be added
    virtual bool isViable(qnode const & node) const { return true; }

    // invoked when a viable node is pulled from the queue. override to update drawing, data structures, etc.
    virtual void addNode(qnode & node) { }

    // generate a child node from a parent
    virtual void beget(qnode const & parent, qtransform const & t, qnode & child);

    virtual cv::Rect_<float> getBoundingRect() const
    {
        float r = (float)maxRadius;
        return cv::Rect_<float>(-r, -r, 2 * r, 2 * r);
    }

    virtual void drawNode(qcanvas &canvas, qnode const &node);

    virtual void saveImage(fs::path imagePath) { };
};


