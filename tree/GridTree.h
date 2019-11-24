#pragma once


#include "tree.h"
#include "util.h"
#include <vector>
#include <iostream>
#include <unordered_set>


using std::cout;
using std::endl;


namespace std
{
    template<>
    struct hash<cv::Point>
    {
        std::size_t operator()(cv::Point const &in) const
        {
            return (in.x << (4*sizeof(int))) ^ (in.y);
        }
    };
    template<>
    struct hash<Point2f>
    {
        std::size_t operator()(Point2f const &in) const
        {
            int x = (int)(0.5f + 100.0f*in.x);
            int y = (int)(0.5f + 100.0f*in.y);
            return (x << (4*sizeof(int))) ^ y;
        }
    };
}


//  Intersection field is a grid, stored as a set of integer pairs
//  to detect intersection
class GridTree : public qtree
{
    // settings

    // set of grid points covered by nodes
    std::unordered_set<cv::Point> m_covered;


public:

    GridTree() { }
	

    void create() override
    {
        gestationRandomness = 1000;

        m_covered.clear();

        polygon = { {
                {0,0},{1,0},{1,1},{0,1}
            } };

        transforms = { {
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[2], polygon[1]), ColorTransform::rgbSink(Matx41(9,4,0,1)*0.111f, 0.8f)),
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[3], polygon[2]), ColorTransform::rgbSink(Matx41(9,9,9,1)*0.111f, 0.5f)),
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[0], polygon[3]), ColorTransform::rgbSink(Matx41(0,4,9,1)*0.111f, 0.8f)/*),
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[1], polygon[0]), ColorTransform::rgbSink(Matx41(9,9,9,1)*0.111f, 0.2f)*/)   // [3] never relevant
            } };

        qnode rootNode;
        rootNode.globalTransform = util::transform3x3::getTranslate(-0.5f, -0.5f);
        rootNode.color = cv::Scalar(1, 1, 1, 1);

        // clear and initialize the queue with the seed

        util::clear(nodeQueue);
        nodeQueue.push(rootNode);
    }


    virtual bool isViable(qnode const &node) const override
    {
        if (!node) 
            return false;

        cv::Point center = getNodeKey(node);

        if(!isPointInBounds(center))
            return false;

        if (m_covered.find(center) == m_covered.end())
            return true;

        return false;
    }

    // add node to hash, data structures, etc.
    virtual void addNode(qnode &currentNode) override
    {
        // take up space
        auto center = getNodeKey(currentNode);
        m_covered.insert(center);
    }

private:

    // get integer grid point nearest node centroid
    cv::Point getNodeKey(qnode const &node) const
    {
        vector<cv::Point2f> pts;
        getPolyPoints(node, pts);
        cv::Point center = ((pts[0] + pts[2]) / 2.0f );
        return center;
    }

};


#define Half12  0.94387431268

