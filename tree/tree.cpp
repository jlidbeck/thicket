#include "tree.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <random>
#include <vector>


#pragma region qnode tree


//  process one node
bool qtree::process()
{
    auto currentNode = nodeQueue.top();
    nodeQueue.pop();

    if (!isViable(currentNode))
        return false;

    addNode(currentNode);

    // create a child node for each available transform.
    // all child nodes are added to the queue, even if not viable.
    for (auto const &t : transforms)
    {
        qnode child;
        beget(currentNode, t, child);
        nodeQueue.push(child);
    }

    return true;
}

//  Generate a child node from parent
void qtree::beget(qnode const & parent, qtransform const & t, qnode & child)
{
    child.generation = parent.generation + 1;
    child.beginTime = parent.beginTime + t.gestation + util::r() * offspringTemporalRandomness;

    child.globalTransform = parent.globalTransform * t.transformMatrix;

    auto m = t.colorTransform * parent.color;
    child.color = m;
}

//  Node draw function for tree of nodes with all the same polygon
void qtree::drawNode(qcanvas &canvas, qnode const &node)
{
    if (!isViable(node))
        return;

    Matx33 m = canvas.globalTransform * node.globalTransform;

    vector<cv::Point2f> v;
    cv::transform(polygon, v, m.get_minor<2, 3>(0, 0));
    vector<vector<cv::Point> > pts(1);
    for (auto const& p : v)
        pts[0].push_back(p * 16);

    cv::Scalar color = (node.det() < 0)
        ? 255.0 * (cv::Scalar(1.0, 1.0, 1.0, 1.0) - node.color)
        : 255.0 * node.color;
    cv::Scalar lineColor = cv::Scalar(255, 255, 255, 255);
    cv::fillPoly(canvas.image, pts, color, cv::LineTypes::LINE_AA, 4);
    //cv::polylines(canvas.image, pts, true, lineColor, 1, cv::LineTypes::LINE_AA, 4);
}


#pragma endregion

