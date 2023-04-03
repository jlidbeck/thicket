#pragma once

#include "SelfLimitingPolygonTree.h"


//  Limited-growth qtrees
//  These models limit growth by prohibiting self-intersection

#pragma region HatTree

//  Tesselations based on the "Hat" aperiodic monotile
//  
class HatTree : public SelfLimitingPolygonTree
{
    vector<int> polygonSideId;

public:
    virtual void setRandomSeed(int randomize) override
    {
        SelfLimitingPolygonTree::setRandomSeed(randomize);

        fieldResolution = 20;
        float maxRadius = 50;
        domain = cv::Rect_<float>(-maxRadius, -maxRadius, maxRadius * 2, maxRadius * 2);
        gestationRandomness = 0;

        rootNodeColor = cv::Scalar(1.0, 1.0, 0.0, 1);

        // override polygon with hat monotile polygon
        float const r32 = sqrt(3.0f) / 2;
        float const r34 = sqrt(3.0f) / 4;
        polygon.clear();
        cv::Point2f pt(0, 0);
        polygon.push_back(pt);
        polygon.push_back(pt += cv::Point2f( 0.5f,   0  ));
        polygon.push_back(pt += cv::Point2f( 0.5f,   0  ));
        polygon.push_back(pt += cv::Point2f( 0.25f,  r34));
        polygon.push_back(pt += cv::Point2f(-0.75f,  r34));
        polygon.push_back(pt += cv::Point2f( 0,      r32));
        polygon.push_back(pt += cv::Point2f(-0.5f,   0  ));
        polygon.push_back(pt += cv::Point2f(-0.25f,  r34));
        polygon.push_back(pt += cv::Point2f(-0.75f, -r34));
        polygon.push_back(pt += cv::Point2f( 0,     -r32));
        polygon.push_back(pt += cv::Point2f(-0.5f,   0  ));
        polygon.push_back(pt += cv::Point2f(-0.25f, -r34));
        polygon.push_back(pt += cv::Point2f( 0.75f, -r34));
        polygon.push_back(pt += cv::Point2f( 0.75f,  r34));

        for (int i = 0; i < polygon.size(); ++i)
        {
            auto p0 = polygon[i];
            auto p1 = polygon[(i + 1) % polygon.size()];
            auto lenp = (cv::norm(p1 - p0));
            printf("side length: [%d]: %lf \n", i, lenp);
        }

        // 3 side lengths: 1, 1/2, sqrt(3)/2
        // however we split the 1-length side to 2 1/2-length sides

        polygonSideId = { 2,2,2,3,3,2,2,3,3,2,2,3,3,2 };
        assert(polygonSideId.size() == polygon.size());

        drawPolygon = polygon;
        // modify polygon that's drawn
        //drawPolygon.erase(drawPolygon.begin() + 1, drawPolygon.begin() + 5);

        // override edge transforms
        transforms.clear();
        //auto ct1 = ColorTransform::hlsSink(util::hsv2bgr( 10.0, 1.0, 0.75), 0.3);
        //auto ct2 = ColorTransform::hlsSink(util::hsv2bgr(200.0, 1.0, 0.75), 0.3);

        // dark blue --> ? (all mirrored)
        transforms.push_back(createEdgeTransform( 0, 6, true));
        transforms.push_back(createEdgeTransform( 1, 5, true));
        transforms.push_back(createEdgeTransform( 2, 6, true));
        transforms.push_back(createEdgeTransform( 3,11, true));
        transforms.push_back(createEdgeTransform( 4,12, true));
        transforms.push_back(createEdgeTransform( 5,13, true));
        transforms.push_back(createEdgeTransform( 6, 0, true));
        transforms.push_back(createEdgeTransform( 7, 3, true));
        transforms.push_back(createEdgeTransform( 8, 4, true));
        transforms.push_back(createEdgeTransform( 9, 5, true));
        transforms.push_back(createEdgeTransform(10, 6, true));
        transforms.push_back(createEdgeTransform(11, 3, true));
        transforms.push_back(createEdgeTransform(12, 4, true));
        transforms.push_back(createEdgeTransform(13, 5, true));
        // light blue --> white (same parity)
        transforms.push_back(createEdgeTransform( 0,  1));
        transforms.push_back(createEdgeTransform( 1,  0));
        transforms.push_back(createEdgeTransform( 1,  2));
        transforms.push_back(createEdgeTransform( 2,  1));
        transforms.push_back(createEdgeTransform( 2,  9));
        transforms.push_back(createEdgeTransform( 2, 13));
        transforms.push_back(createEdgeTransform( 3,  8));
        transforms.push_back(createEdgeTransform( 3, 12));
        transforms.push_back(createEdgeTransform( 4,  7));
        transforms.push_back(createEdgeTransform( 4, 11));
        transforms.push_back(createEdgeTransform( 5,  0));
        transforms.push_back(createEdgeTransform( 5, 10));
        transforms.push_back(createEdgeTransform( 6,  9));
        transforms.push_back(createEdgeTransform( 6, 13));
        transforms.push_back(createEdgeTransform( 7,  8));
        transforms.push_back(createEdgeTransform( 7, 12));
        transforms.push_back(createEdgeTransform( 8,  7));
        transforms.push_back(createEdgeTransform( 9,  2));
        transforms.push_back(createEdgeTransform(10, 13));
        transforms.push_back(createEdgeTransform(11, 12));
        transforms.push_back(createEdgeTransform(12, 11));
        transforms.push_back(createEdgeTransform(13, 1));
        transforms.push_back(createEdgeTransform(13, 2));

        /*for (int i = 0; i < polygonSideId.size(); ++i)
        {
            for (int j = 0; j < polygonSideId.size(); ++j)
            {
                if (polygonSideId[i] == polygonSideId[j])
                {
                    transforms.push_back(createEdgeTransform(i, j));
                    transforms.push_back(createEdgeTransform(i, j, true));
                }

                //if (r(20) == 0)
                //    transforms.push_back(createEdgeTransform(i, j));

                //if (r(20) == 0)
                //    transforms.push_back(createEdgeTransform(i, j, true));
            }
        }*/

        randomizeTransforms(3);
    }

    virtual void to_json(json& j) const override
    {
        SelfLimitingPolygonTree::to_json(j);

        j["_class"] = "HatTree";
    }

    virtual void from_json(json const& j) override
    {
        SelfLimitingPolygonTree::from_json(j);

        if (drawPolygon.empty())
        {
            drawPolygon = polygon;
        }
    }

    virtual void create() override
    {
        SelfLimitingPolygonTree::create();

    }

    // overriding to save intersection field mask as well
    virtual void saveImage(fs::path imagePath, qcanvas const& canvas) override
    {
        // save the intersection field mask
        imagePath = imagePath.replace_extension("mask.png");
        cv::imwrite(imagePath.string(), m_field);
    }

    void drawNode(qcanvas& canvas, qnode const& node) override
    {
        cv::Scalar color =
            //(node.det() < 0) ? 255.0 * (cv::Scalar(1.0, 1.0, 1.0, 1.0) - node.color) : 
            255.0 * node.color;

        // todo
        canvas.fillPoly(drawPolygon, node.globalTransform, 255.0 * node.color, lineThickness, lineColor);

        // modify polygon that's drawn
        //pts[0].resize(4);

        //float d = cv::norm(pts[0][0] - pts[0][1]);
        //auto ctr0 = (pts[0][0] + pts[0][1] + pts[0][2] + pts[0][8]) / 4;
        //cv::circle(canvas.image, ctr0, d*0.3f, color, -1, cv::LineTypes::FILLED, 4);
        //auto ctr1 = (pts[0][2] + pts[0][3] + pts[0][7] + pts[0][8]) / 4;
        //cv::circle(canvas.image, ctr1, d*0.2f, color, -1, cv::LineTypes::FILLED, 4);
        //auto ctr2 = (pts[0][3] + pts[0][4] + pts[0][6] + pts[0][7]) / 4;
        //cv::circle(canvas.image, ctr2, d*0.15f, color, -1, cv::LineTypes::FILLED, 4);
        //auto ctr3 = (pts[0][4] + pts[0][6]) / 2;
        //cv::circle(canvas.image, ctr3, d*0.1f, color, -1, cv::LineTypes::FILLED, 4);
    }


};

REGISTER_QTREE_TYPE(HatTree);

#pragma endregion
