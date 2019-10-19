#pragma once

#include <opencv2/core/core.hpp>


namespace transform3x3
{
    template<typename _Tp> 
    cv::Mat_<_Tp> getRotationMatrix2D(cv::Point_<_Tp> center, double angle, double scale, _Tp translateX = 0, _Tp translateY = 0)
    {
        cv::Mat_<_Tp> m = Mat_<_Tp>::eye(3, 3);
        m(cv::Range(0, 2), cv::Range::all()) = cv::getRotationMatrix2D(center, angle, scale);
        m.at<_Tp>(0, 2) += translateX;
        m.at<_Tp>(1, 2) += translateY;
        return m;
    }

    template<typename _Tp>
    cv::Mat_<_Tp> getFlipScaleOffset(double scale, double offX, double offY)
    {
        return (cv::Mat_<_Tp>(3, 3) <<
            scale, 0, offX,
            0, -scale, offY,
            0, 0, 1);
    }

    template<typename _Tp>
    cv::Mat_<_Tp> getRotateFlipScaleOffset(double angle, double scale, double offX, double offY)
    {
        cv::Mat_<_Tp> m = cv::Mat_<_Tp>::eye(3, 3);
        m(cv::Range(0, 2), cv::Range::all()) = cv::getRotationMatrix2D(cv::Point_<_Tp>(1.0, 0.0), angle, scale);

        m.at<_Tp>(1, 1) = -m.at<_Tp>(1, 1);
        m.at<_Tp>(0, 2) = offX;
        m.at<_Tp>(1, 2) = offY;
        return m;
    }
}

