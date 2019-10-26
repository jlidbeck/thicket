#pragma once


#include "GridTree.h"


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

    int maxRadius = 50;


public:

    HexGridTree()
    {
    }

    void create(int randomizeSettings) override
    {
        m_covered.clear();
        polygon.clear();
        for (int i = 0; i < 6; i++)
        {
            float angle = i * 6.283 / 6;
            polygon.push_back(Point2f(cos(angle), sin(angle)));
        }

        // edge 1-2 is up
        transforms = { {
            qtransform(util::transform3x3::getEdgeMap(polygon[4], polygon[5], polygon[0], polygon[5]), util::colorSink(Matx41(9,0,4,1)*0.111f, 0.7f)),
            qtransform(util::transform3x3::getEdgeMap(polygon[4], polygon[5], polygon[1], polygon[0]), util::colorSink(Matx41(9,4,0,1)*0.111f, 0.7f)),
            qtransform(util::transform3x3::getEdgeMap(polygon[4], polygon[5], polygon[2], polygon[1]), util::colorSink(Matx41(9,9,9,1)*0.111f, 0.5f)),
            qtransform(util::transform3x3::getEdgeMap(polygon[4], polygon[5], polygon[3], polygon[2]), util::colorSink(Matx41(0,4,9,1)*0.111f, 0.7f)),
            qtransform(util::transform3x3::getEdgeMap(polygon[4], polygon[5], polygon[4], polygon[3]), util::colorSink(Matx41(4,0,9,1)*0.111f, 0.7f))
        } };

        rootNode.m_color = Matx41(1, 1, 1, 1);


        offspringTemporalRandomness = 1000;

        if (randomizeSettings)
        {
            for (auto &t : transforms)
            {
                t.colorTransform = util::colorSink(util::randomColor(), util::r());
            }

            offspringTemporalRandomness = 1 + rand() % 200;
        }

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
        cout << "Space filled: (" << center.x << ", " << center.y << ") :" << m_covered.size() << endl;
    }

    cv::Point2f getNodeCentroid(qnode const &node) const
    {
        // assume each node is centered near a unique integer grid point
        vector<cv::Point2f> pts;
        node.getPolyPoints(polygon, pts);
        cv::Point center = cv::Point(((pts[0] + pts[3]) / 2.0f ) * 100.0f);
        return cv::Point2f(center) / 100.0f;
    }

};

