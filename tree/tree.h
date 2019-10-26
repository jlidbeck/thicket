#pragma once

#include "util.h"
#include <opencv2/core/core.hpp>
#include <vector>
#include <queue>


class qcanvas;

using std::vector;
using std::priority_queue;

using cv::Point2f;
typedef cv::Matx<float, 3, 3> Matx33;
typedef cv::Matx<float, 4, 1> Matx41;
typedef cv::Matx<float, 4, 4> Matx44;


class qtransform
{
public:
    Matx33 transformMatrix;
    Matx44 colorTransform;

public:

    qtransform(Matx33 const &cvMat = Matx33::eye(), Matx44 const &colorTransform_ = Matx44::eye())
    {
        transformMatrix = cvMat;
        colorTransform = colorTransform_;
    }

    template<typename _Tp>
    qtransform(_Tp m00, _Tp m01, _Tp mtx, _Tp m10, _Tp m11, _Tp mty, Matx44 const &colorTransform_)
    {
        transformMatrix = Matx33(m00, m01, mtx, m10, m11, mty, 0, 0, 1);
        colorTransform = colorTransform_;
    }

    qtransform(double angle, double scale, cv::Point2f translate)
    {
        transformMatrix = util::transform3x3::getRotationMatrix2D(Point2f(), angle, scale, translate.x, translate.y);
        colorTransform = colorTransform.eye();
        colorTransform(2, 2) = 0.94f;
        colorTransform(1, 1) = 0.96f;
    }
};


class qnode
{
public:
    double beginTime;
    int generation;
    Matx33 m_accum;
    Matx41 m_color = Matx41(1, 0, 0, 1);

    qnode(double beginTime_ = 0);

    inline float det() const { return (m_accum(0, 0) * m_accum(0, 0) + m_accum(1, 0) * m_accum(1, 0)); }

    inline bool operator!() const { return (det() < 1e-6); }

    void getPolyPoints(std::vector<cv::Point2f> const &points, std::vector<cv::Point2f> &transformedPoints) const
    {
        cv::transform(points, transformedPoints, m_accum.get_minor<2, 3>(0, 0));
    }

    void draw(qcanvas &canvas, std::vector<cv::Point2f> const &points) const;

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
    qnode rootNode;
    std::vector<qtransform> transforms;
    std::priority_queue<qnode, std::vector<qnode>, qnode::EarliestFirst> nodeQueue;

    std::vector<cv::Point2f> polygon;
    int offspringTemporalRandomness = 100;

private:
    cv::Rect_<float> m_boundingRect;

public:
    qtree() {}

    virtual void create(int settings);

    // process the next node in the queue
    virtual bool process();

    // override to indicate that a child node should not be added
    virtual bool isViable(qnode const & node) const { return true; }

    // invoked when a viable node is pulled from the queue. override to update drawing, data structures, etc.
    virtual void addNode(qnode & node) { }

    // generate a child node from a parent
    virtual void beget(qnode const & parent, qtransform const & t, qnode & child);

    virtual cv::Rect_<float> getBoundingRect() const;
    virtual void drawNode(qcanvas &canvas, qnode const &node);
};
