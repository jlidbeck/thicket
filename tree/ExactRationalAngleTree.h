#include "tree.h"
#include "incommensurable_trig.h"
#include "nlohmann/json.hpp"


class ExactNode : public qnode
{
public:
    iv<16, 30> vector;
};


class ExactRationalAngleTree : public qtree
{
private:

    ExactNode m_rootNode;

public:

public:
    virtual void setRandomSeed(int n) override
    {
        qtree::setRandomSeed(n);
    }

    virtual void create() override
    {
        m_rootNode.vector = iv<16, 30>();
        m_rootNode.color = cv::Scalar(0.5, 0.5, 0.5, 1);
        m_rootNode.globalTransform = Matx33::eye();

        polygon = { {
                { .1f,-1}, { .08f,0}, { .1f, 1},
                {-.1f, 1}, {-.08f,0}, {-.1f,-1}
            } };

        transforms = { {
                qtransform(),
                qtransform()
            } };


        for (auto &t : transforms)
        {
            t.colorTransform = ColorTransform::rgbSink(randomColor(), r());
            t.gestation = 1 + rand() % 500;
        }

        json jt;
        to_json(jt);

        cout << "ExactRationalAngleTree settings: " << to_string(jt) << endl;

        // clear and initialize the queue with the seed

        util::clear(nodeQueue);
        nodeQueue.push(m_rootNode);
    }

    virtual void beget(qnode const & parent, qtransform const & t, qnode & child) override
    {
        int angle = rand() % 120;

    }

    virtual void to_json(json& j) const override
    {
        qtree::to_json(j);

        j["_class"] = "ExactRationalAngleTree";
    }

    virtual void from_json(json const& j) override
    {
        try {
            qtree::from_json(j);
        }
        catch (std::exception&)
        {
            // let's ignore exceptions for now
            randomSeed = 42;
        }
    }

};
