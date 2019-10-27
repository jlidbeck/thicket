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
    cv::Mat1b m_field;
    mutable cv::Mat1b m_fieldLayer;
    Matx33 m_fieldTransform;

    int polygonSides = 5;
    int maxRadius = 10;
    int fieldResolution = 10;

public:

    HexGridTree()
    {
    }

    void create(int randomizeSettings) override
    {

        offspringTemporalRandomness = 1000;

        if (randomizeSettings)
        {
            maxRadius = 5 + rand() % 40;
            polygonSides = (randomizeSettings%6)+3;
            
            for (auto &t : transforms)
            {
                t.colorTransform = util::colorSink(util::randomColor(), util::r());
            }

            offspringTemporalRandomness = 1 + rand() % 200;
        }


        m_field.create(maxRadius*2*fieldResolution, maxRadius*2*fieldResolution);
        m_field = 0;
        m_field.copyTo(m_fieldLayer);
        m_fieldTransform = util::transform3x3::getScaleTranslate(fieldResolution, maxRadius*fieldResolution, maxRadius*fieldResolution);

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

        cv::Point2f center(node.m_accum(0, 2), node.m_accum(1, 2));

        if (center.x < -maxRadius || center.x>maxRadius || center.y < -maxRadius || center.y>maxRadius) 
            return false;

        drawField(node);
        cv::Mat andmat;
        cv::bitwise_and(m_field, m_fieldLayer, andmat);
        return(!cv::countNonZero(andmat));
    }

    // add node to hash, data structures, etc.
    virtual void addNode(qnode &currentNode) override
    {
        // take up space
        cv::bitwise_or(m_field, m_fieldLayer, m_field);
        //cout << "Space filled: (" << center.x << ", " << center.y << ") :" << m_covered.size() << endl;
    }

    void drawField(qnode const &node) const
    {
        Matx33 m = m_fieldTransform * node.m_accum;

        vector<cv::Point2f> v;
        cv::transform(polygon, v, m.get_minor<2, 3>(0, 0));
        vector<vector<cv::Point> > pts(1);
        for (auto const& p : v)
            pts[0].push_back(p);

        m_fieldLayer = 0;
        cv::fillPoly(m_fieldLayer, pts, cv::Scalar(255), cv::LineTypes::LINE_8);
        cv::polylines(m_fieldLayer, pts, true, cv::Scalar(0), 1);
    }

};

