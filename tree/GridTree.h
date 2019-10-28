#pragma once


#include "tree.h"
#include "util.h"
#include <random>
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

class GridTree : public qtree
{
    std::unordered_set<cv::Point> m_covered;

    int maxRadius = 100;


public:

    GridTree() { }
	

    void create(int randomizeSettings) override
    {
        offspringTemporalRandomness = 1000;

        m_covered.clear();

        polygon = { {
                {0,0},{1,0},{1,1},{0,1}
            } };

        transforms = { {
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[2], polygon[1]), util::colorSink(Matx41(9,4,0,1)*0.111f, 0.8f)),
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[3], polygon[2]), util::colorSink(Matx41(9,9,9,1)*0.111f, 0.5f)),
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[0], polygon[3]), util::colorSink(Matx41(0,4,9,1)*0.111f, 0.8f)/*),
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[1], polygon[0]), util::colorSink(Matx41(9,9,9,1)*0.111f, 0.2f)*/)   // [3] never relevant
            } };

        rootNode.m_accum = util::transform3x3::getTranslate(-0.5f, -0.5f);
        rootNode.m_color = Matx41(1, 1, 1, 1);


        //if (settings)
        //{
        //    for (auto &t : transforms)
        //    {
        //        t.colorTransform = util::colorSink(util::randomColor(), util::r());
        //    }

        //    offspringTemporalRandomness = 1 + rand() % 500;
        //}

        cout << "Settings changed: " << transforms.size() << " transforms, offspringTemporalRandomness: " << offspringTemporalRandomness << endl;

        // clear and initialize the queue with the seed

        util::clear(nodeQueue);
        nodeQueue.push(rootNode);
    }


    virtual cv::Rect_<float> getBoundingRect() const override
    {
            //vector<cv::Point2f> v;
            //rootNode.getPolyPoints(polygon, v);
            //return util::getBoundingRect(v);

        float r = maxRadius;
        return cv::Rect_<float>(-r, -r, 2 * r, 2 * r);
    }

    virtual bool isViable(qnode const &node) const override
    {
        if (!node) 
            return false;

        cv::Point center = getNodeKey(node);

        if (center.x < -maxRadius || center.x>maxRadius || center.y < -maxRadius || center.y>maxRadius) 
            return false;

        if (m_covered.find(center) == m_covered.end())
            return true;

        //cout << "**collision (" << center.x << ", " << center.y << ")\n";
        return false;
    }

    // add node to hash, data structures, etc.
    virtual void addNode(qnode &currentNode) override
    {
        // take up space
        auto center = getNodeKey(currentNode);
        m_covered.insert(center);
        //cout << "Space filled: (" << center.x << ", " << center.y << ") :" << m_covered.size() << endl;
    }

    private:
    cv::Point getNodeKey(qnode const &node) const
    {
        // assume each node is centered near a unique integer grid point
        vector<cv::Point2f> pts;
        node.getPolyPoints(polygon, pts);
        cv::Point center = ((pts[0] + pts[2]) / 2.0f );
        return center;
    }

};

