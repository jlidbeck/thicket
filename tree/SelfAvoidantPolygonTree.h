#pragma once


#include "tree.h"
#include "util.h"
#include <vector>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <array>


using std::cout;
using std::endl;


//  Limited-growth qtrees
//  These models limit growth by prohibiting self-intersection


class SelfAvoidantPolygonTree : public qtree
{
protected:
    // settings
    int polygonSides = 5;

    int fieldResolution = 10;   // pixels per unit

    // minimum scale (relative to rootNode) considered for new nodes
    float minimumScale = 0.05f; 

    // intersection field
    cv::Mat1b m_field;
    mutable cv::Mat1b m_fieldLayer;
    Matx33 m_fieldTransform;


public:

    SelfAvoidantPolygonTree() { }

    virtual json getSettings() const override
    {
        json j = qtree::getSettings();
        j["name"] = "SelfAvoidantPolygonTree";
        j["fieldResolution"] = fieldResolution;
        j["polygonSides"] = polygonSides;
        return j;
    }

    virtual bool settingsFromJson(json const &j) override
    {
        if (!qtree::settingsFromJson(j))
            return false;

        fieldResolution = j["fieldResolution"];
        polygonSides = j["polygonSides"];

        return true;
    }

    virtual void setRandomSeed(int randomize)
    {
        qtree::setRandomSeed(randomize);

        maxRadius = 10.0;
        polygonSides = 5;
        offspringTemporalRandomness = 1000;

        if (randomize)
        {
            maxRadius = 5.0 + r(40.0);
            polygonSides = (randomize % 6) + 3;

            offspringTemporalRandomness = r(200.0);
        }
    }

    virtual void create() override
    {
        prng.seed(randomSeed);

        // initialize intersection field
        int size = (int)(0.5 + maxRadius * 2 * fieldResolution);
        m_field.create(size, size);
        m_field = 0;
        m_field.copyTo(m_fieldLayer);
        m_fieldTransform = util::transform3x3::getScaleTranslate((double)fieldResolution, maxRadius*fieldResolution, maxRadius*fieldResolution);

        util::polygon::createRegularPolygon(polygon, polygonSides);

        // add edge transforms to map edge 0 to all other edges
        transforms.clear();
        for (int i = 1; i < polygonSides; ++i)
        {
            Matx41 hsv((float)i/(float)polygonSides, 1.0, 0.5), bgr(1.0, 0.5, 0.0);
            //cv::cvtColor(hsv, bgr, cv::ColorConversionCodes::COLOR_HSV2BGR);
            bgr = randomColor();
            transforms.push_back(
                qtransform(
                    util::transform3x3::getEdgeMap(polygon[polygonSides - 1], polygon[0], polygon[i], polygon[i - 1]),
                    util::colorSink(bgr, 0.7f))
            );
        }


        // clear and initialize the queue with the seed

        qnode rootNode;
        createRootNode(rootNode);

        util::clear(nodeQueue);
        nodeQueue.push(rootNode);
    }

    virtual void createRootNode(qnode & rootNode)
    {
        rootNode.beginTime = 0;
        rootNode.generation = 0;
        rootNode.color = cv::Scalar(0, 0, 1, 1);

        // center root node at origin
        auto centroid = util::polygon::centroid(polygon);
        rootNode.globalTransform = util::transform3x3::getScaleTranslate(1.0f, -centroid.x, -centroid.y);
    }


    virtual bool isViable(qnode const &node) const override
    {
        if (!node) 
            return false;

        if (fabs(node.det()) < minimumScale*minimumScale)
            return false;

        // since the node polygon is centered at its local origin, its global
		// centroid is simply the translation from its transform matrix
        cv::Matx<float, 3, 1> center(node.globalTransform(0, 2), node.globalTransform(1, 2), 1.0f);

        if(center(0)*center(0) + center(1)*center(1) > maxRadius*maxRadius)
            return false;

        // quick check field pixel at transformed node center
        auto fieldPt = m_fieldTransform * center;
        if (m_field.at<uchar>(fieldPt(1), fieldPt(0)))
            return false;

        if (!drawField(node))
            return false;   // out of image bounds

        thread_local cv::Mat andmat;
        cv::bitwise_and(m_field, m_fieldLayer, andmat);
        return(!cv::countNonZero(andmat));
    }

    //  draw node on field stage layer to prepare for collision detection
    //  full collision detection is not done here, but this function returns false if out-of-bounds or other
    //  quick detection means that this node is not viable.
    bool drawField(qnode const &node) const
    {
        Matx33 m = m_fieldTransform * node.globalTransform;

        vector<cv::Point2f> v;

        // first, transform model polygon to model global coordinates to test against maxRadius
        cv::transform(polygon, v, node.globalTransform.get_minor<2, 3>(0, 0));
        for (auto const& p : v)
            if (p.dot(p) > maxRadius*maxRadius)
                return false;

        // transform model polygon to field coords
        cv::transform(polygon, v, m.get_minor<2, 3>(0, 0));
        vector<vector<cv::Point> > pts(1);
        for (auto const& p : v)
            pts[0].push_back(p);

        m_fieldLayer = 0;
        cv::fillPoly(m_fieldLayer, pts, cv::Scalar(255), cv::LineTypes::LINE_8);
        // reduce by drawing outline in black, since OpenCV fillPoly draws extra pixels along edge
        cv::polylines(m_fieldLayer, pts, true, cv::Scalar(0), 1);
		
		return true;
    }

    // update field image: composite new node
    virtual void addNode(qnode &currentNode) override
    {
        cv::bitwise_or(m_field, m_fieldLayer, m_field);
    }

};


class ScaledPolygonTree : public SelfAvoidantPolygonTree
{
    double m_ratio;
    bool m_ambidextrous;

public:
    virtual void setRandomSeed(int randomize) override
    {
        SelfAvoidantPolygonTree::setRandomSeed(randomize);

        fieldResolution = 200;
        maxRadius = 3.5;
        
        // ratio: child size / parent size
        std::array<float, 5> ratioPresets = { {
            (sqrt(5.0f) - 1.0f) / 2.0f,        // phi
            0.5f,
            1.0f / 3.0f,
            1.0f / sqrt(2.0f),
            (sqrt(3.0f) - 1.0f) / 2.0f
        } };

        m_ratio = ratioPresets[randomize%ratioPresets.size()];
        m_ambidextrous = (randomize % 2);
    }

    virtual json getSettings() const override
    {
        json j = SelfAvoidantPolygonTree::getSettings();

        j["name"] = "ScaledPolygonTree";
        j["ratio"] = m_ratio;
        j["ambidextrous"] = m_ambidextrous;

        return j;
    }

    virtual bool settingsFromJson(json const &j) override
    {
        if (!SelfAvoidantPolygonTree::settingsFromJson(j))
            return false;

        m_ratio = j["ratio"];
        m_ambidextrous = j["ambidextrous"];

        return true;
    }

    virtual void create() override
    {
        SelfAvoidantPolygonTree::create();

        // override edge transforms
        transforms.clear();
        for (int i = 1; i < polygon.size(); ++i)
        {
            auto edgePt1 = m_ratio * polygon[i - 1] + (1.0f - m_ratio)*polygon[i    ];
            transforms.push_back(
                qtransform(
                    util::transform3x3::getEdgeMap(polygon[polygon.size() - 1], polygon[0], polygon[i], edgePt1),
                    util::colorSink(randomColor(), 0.5))
            );

            if (m_ambidextrous)
            {
                auto edgePt2 = m_ratio * polygon[i    ] + (1.0f - m_ratio)*polygon[i - 1];
                transforms.push_back(
                    qtransform(
                        util::transform3x3::getEdgeMap(polygon[polygon.size() - 1], polygon[0], edgePt2, polygon[i - 1]),
                        util::colorSink(randomColor(), 0.5))
                );
            }
        }
    }

};


class TrapezoidTree : public SelfAvoidantPolygonTree
{
public:
    virtual void setRandomSeed(int randomize) override
    {
        SelfAvoidantPolygonTree::setRandomSeed(randomize);

        fieldResolution = 200;
        maxRadius = 10;
        offspringTemporalRandomness = 10;
    }

    virtual void create() override
    {
        SelfAvoidantPolygonTree::create();

        // override polygon
        polygon = { { { -0.5f, -0.5f}, {0.5f, -0.5f}, {0.4f, 0.4f}, {-0.5f, 0.45f} } };

        //auto m = util::transform3x3::getMirroredEdgeMap(cv::Point2f( 0.0f,0.0f ), cv::Point2f( 1.0f,0.0f ), cv::Point2f( 0.0f,0.0f ), cv::Point2f( 1.0f,0.0f ));

        int steps = 24;
        float angle = 6.283f / steps;
        float r0 = 0.5f;
        float r1 = 1.0f;
        float growthFactor = pow(r1 / r0, 2.0f / steps);
        polygon = { { {r0,0}, {r1,0}, {r1*growthFactor*cos(angle),r1*growthFactor*sin(angle)}, {r0*growthFactor*cos(angle),r0*growthFactor*sin(angle)} } };

        // override edge transforms
        transforms.clear();
        //for (int i = 1; i < polygon.size(); ++i)
        //{
        //    transforms.push_back(
        //        qtransform(
        //            util::transform3x3::getMirroredEdgeMap(polygon[0], polygon[1], polygon[i], polygon[(i+1)%polygon.size()]),
        //            util::colorSink(randomColor(), 0.5f))
        //    );
        //}

        transforms.push_back(
            qtransform(
                util::transform3x3::getMirroredEdgeMap(polygon[0], polygon[1], polygon[1], polygon[2]),
                util::colorSink(randomColor(), 0.5))
        );
        transforms.push_back(
            qtransform(
                util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[3], polygon[2]),
                util::colorSink(randomColor(), 0.5))
        );
        transforms.push_back(
            qtransform(
                util::transform3x3::getMirroredEdgeMap(polygon[0], polygon[1], polygon[3], polygon[0]),
                util::colorSink(randomColor(), 0.5))
        );

        //transforms[0].gestation = 1111.1;
        transforms[0].gestation = 6.2;
        transforms[1].gestation = 1.0;
        transforms[2].gestation = 7.3;

        //transforms[0].colorTransform = util::colorSink(1.0f,1.0f,1.0f, 0.5f);
        //transforms[0].colorTransform = util::colorSink(0.0f,0.5f,0.0f, 0.8f);
        //transforms[1].colorTransform = util::colorSink(0.5f,1.0f,1.0f, 0.3f);
        //transforms[2].colorTransform = util::colorSink(0.9f,0.5f,0.0f, 0.8f);

    }

    virtual void createRootNode(qnode & rootNode) override
    {
        rootNode.color = cv::Scalar(0.2, 0.5, 0, 1);
    }

};


class ThornTree : public SelfAvoidantPolygonTree
{
public:
    virtual void setRandomSeed(int randomize) override
    {
        SelfAvoidantPolygonTree::setRandomSeed(randomize);

        fieldResolution = 50;
        maxRadius = 20;
        offspringTemporalRandomness = 100;
    }

    virtual void create() override
    {
        SelfAvoidantPolygonTree::create();

        // override polygon
        util::polygon::createStar(polygon);

        // override edge transforms
        transforms.clear();
        //auto ct1 = util::colorSink(util::hsv2bgr( 10.0, 1.0, 0.75), 0.3);
        //auto ct2 = util::colorSink(util::hsv2bgr(200.0, 1.0, 0.75), 0.3);

        std::vector<Matx44> colors;
        std::uniform_real_distribution<double> dist{ 0.0,1.0 };
        double sat = r();
        double v = r();
        colors.push_back(util::colorSink(util::hsv2bgr(r()*360.0, sat, v), 0.5 * r()));
        colors.push_back(util::colorSink(util::hsv2bgr(r()*360.0, sat, v), 0.5 * r()));
        colors.push_back(util::colorSink(util::hsv2bgr(r()*360.0, sat, v), 0.5 * r()));
        auto icolor = colors.begin();

        for (int i = 0; i < polygon.size(); ++i)
        {
            for (int j = 0; j < polygon.size(); ++j)
            {
                if (++icolor == colors.end())
                    icolor = colors.begin();

                if(r(20) == 0)
                    transforms.push_back(
                        qtransform(
                            util::transform3x3::getEdgeMap(polygon[i], polygon[(i + 1) % polygon.size()], polygon[(j + 1) % polygon.size()], polygon[j]),
                            *icolor)
                    );
                if (r(20) == 0)
                    transforms.push_back(
                        qtransform(
                            util::transform3x3::getMirroredEdgeMap(polygon[i], polygon[(i + 1) % polygon.size()], polygon[j], polygon[(j + 1) % polygon.size()]),
                            *icolor)
                    );
            }
        }
    }

    virtual void createRootNode(qnode & rootNode) override
    {
        rootNode.color = cv::Scalar(1,1,1, 1);
    }

    virtual void saveImage(fs::path imagePath) override
    {
        // save the intersection field mask
        imagePath = imagePath.replace_extension("mask.png");
        cv::imwrite(imagePath.string(), m_field);
    }

    virtual json getSettings() const override
    {
        json j = SelfAvoidantPolygonTree::getSettings();
        j["name"] = "ThornTree";
        return j;
    }


};
