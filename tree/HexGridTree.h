#pragma once


#include "GridTree.h"
#include <opencv2/imgproc.hpp>


namespace std
{
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

class HexGridTree : public GridTree
{
    std::unordered_set<Point2f> m_covered;

    int polygonSides = 5;
    int maxRadius = 10;


public:

    HexGridTree()
    {
    }

    void create(int randomizeSettings) override
    {

        offspringTemporalRandomness = 1000;

        if (randomizeSettings)
        {
            maxRadius = 5 + rand() % 20;
            polygonSides = (randomizeSettings%6)+3;
            
            for (auto &t : transforms)
            {
                t.colorTransform = util::colorSink(util::randomColor(), util::r());
            }

            offspringTemporalRandomness = 1 + rand() % 200;
        }


        m_covered.clear();

        // create regular polygon
        polygon.clear();
        float angle = -6.283 / (polygonSides * 2);
        for (int i = 0; i < polygonSides; i++)
        {
            polygon.push_back(Point2f(sin(angle), cos(angle)));
            angle += 6.283 / polygonSides;
        }

        // add edge transforms to map edge 0 to all other edges
        transforms.clear();
        for (int i = 1; i < polygonSides; ++i)
        {
            Matx41 hsv((float)i/(float)polygonSides, 1.0, 0.5), bgr(1.0, 0.5, 0.0);
            //cv::cvtColor(hsv, bgr, cv::ColorConversionCodes::COLOR_HSV2BGR);
            bgr = util::randomColor();
            transforms.push_back(
                qtransform(
                    util::transform3x3::getEdgeMap(polygon[polygonSides - 1], polygon[0], polygon[i], polygon[i - 1]),
                    util::colorSink(bgr, 0.7f))
            );
        }

        rootNode.m_color = Matx41(1, 1, 1, 1);


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

        auto center = getNodeCentroid(node);

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
        auto center = getNodeCentroid(currentNode);
        m_covered.insert(center);
        //cout << "Space filled: (" << center.x << ", " << center.y << ") :" << m_covered.size() << endl;
    }

    cv::Point2f getNodeCentroid(qnode const &node) const
    {
        // since tile is centered locally at origin, use the transform matrix
        cv::Point2f pt(node.m_accum(0, 2), node.m_accum(1, 2));
        // assume each node is centered near a unique integer grid point
        float ds = 2.0f;
        cv::Point center = cv::Point(pt * ds);
        return cv::Point2f(center) / ds;
    }

};

