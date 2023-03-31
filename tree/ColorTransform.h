#pragma once


#include "util.h"
#include <nlohmann/json.hpp>
#include <opencv2/core/core.hpp>
#include <vector>


using json = nlohmann::basic_json<>;

inline void to_json(json& j, class ColorTransform const& t);
inline void from_json(json const& j, class ColorTransform& t);


class ColorTransform
{
public:
    cv::Matx<float, 4, 4> hls = hls.eye();

    // static initializers

    template<typename _Tp>
    static ColorTransform rgbSink(cv::Scalar_<_Tp> const& color, _Tp a)
    {
        // todo
        return ColorTransform{};
    }

    template<typename _Tp>
    static ColorTransform rgbSink(cv::Matx<float, 4, 1> const& color, _Tp a)
    {
        // todo
        return ColorTransform{};
    }

    //  Constant color function. Regardless of input, produces [h, l, s]
    template<typename _Tp>
    static ColorTransform colorConstant(_Tp h, _Tp l, _Tp s)
    {
        return ColorTransform{ cv::Matx<_Tp, 4, 4>(
            0, 0, 0, h,
            0, 0, 0, l,
            0, 0, 0, s,
            0, 0, 0, 1) };
    }

    //  Color sink function. If 0 < a <= 1, function applied to any color interpolates it toward [h, l, s]
    template<typename _Tp>
    static ColorTransform hlsSink(_Tp h, _Tp l, _Tp s, _Tp a)
    {
        return ColorTransform{ cv::Matx<_Tp, 4, 4>(
            1 - a, 0, 0, a * h,
            0, 1 - a, 0, a * l,
            0, 0, 1 - a, a * s,
            0, 0, 0, 1) };
    }

    template<typename _Tp>
    static ColorTransform hueShift(_Tp hueShift)
    {
        return ColorTransform{ cv::Matx<_Tp, 4, 4>(
            1, 0, 0, hueShift,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1) };
    }

    // 3D scale and translate.
    // args: { hs, ht, ls, lt, ss, st }
    // Returned matrix transforms (h,l,s) to (h*hs+ht, l*ls+lt, s*ss+st)
    template<typename _Tp>
    static ColorTransform hlsTransform(std::vector<_Tp> const& args)
    {
        return ColorTransform{ cv::Matx<_Tp, 4, 4>(
            args[0], 0, 0, args[1],
            0, args[2], 0, args[3],
            0, 0, args[4], args[5],
            0, 0, 0, 1) };
    }

    // apply (todo: operator *, operator ())

    cv::Scalar apply(cv::Scalar const& color) const
    {
        auto hlsColor = util::cvtColor(color, cv::ColorConversionCodes::COLOR_BGR2HLS);
        return util::cvtColor(hls * hlsColor, cv::ColorConversionCodes::COLOR_HLS2BGR);
    }

    // info

    std::string description() const
    {
        json j;
        ::to_json(j, *this);
        return j.dump(-1);
    }

    //  Hue shift: subset of hlsTransform with only one d.f.
    bool asHueShift(float& hueShift) const
    {
        // row 0, col 3 is the hue translation
        hueShift = hls(0, 3);
        auto test = ColorTransform::hueShift(hueShift);
        return util::approximatelyEqual(hls, test.hls);
    }

    //  HLS sink: subset of hlsTransform which when applied repeatedly converge to an HLS value
    bool asHlsSink(float& h, float& l, float& s, float& a) const
    {
        a = 1.0f - hls(0, 0);
        if (!(a > 0.0f))
            return false;
        h = hls(0, 3) / a;
        l = hls(1, 3) / a;
        s = hls(2, 3) / a;
        auto test = ColorTransform::hlsSink(h, l, s, a);
        return util::approximatelyEqual(hls, test.hls);
    }

    //  Hls transform: subset of 4x4 matrix with only 6 values set non-identy:
    //  hue, lum, sat, scale (the diagonal) and offset (last column).
    bool asHlsTransform(std::vector<float>& v) const
    {
        v = {
            hls(0, 0), hls(0, 3),
            hls(1, 1), hls(1, 3),
            hls(2, 2), hls(2, 3)
        };
        auto test = ColorTransform::hlsTransform(v);
        return util::approximatelyEqual(hls, test.hls);
    }

    //  Interpolate this transform with another
    //  0<=factor<=1
    //  factor==0: this transform is unchanged
    //  factor==1: this transform is set equal to {ct}
    void linterp(ColorTransform const& ct, double f)
    {
        double b = 1.0 - f;
        float bh, bl, bs, ba;
        float fh, fl, fs, fa;
        if (asHueShift(bh))
        {
            if (ct.asHueShift(fh))
            {
                *this = ColorTransform::hueShift(f * fh + b * bh);
                return;
            }
        }
        if (asHlsSink(bh, bl, bs, ba))
        {
            if (ct.asHlsSink(fh, fl, fs, fa))
            {
                *this = ColorTransform::hlsSink(f * fh + b * bh, f * fl + b * bl, f * fs + b * bs, f * fa + b * ba);
                return;
            }
        }
        std::vector<float> bt;
        if (asHlsTransform(bt))
        {
            std::vector<float> at;
            if (ct.asHlsTransform(at))
            {
                // not quite perfect, because scaling factors are combined linearly
                //*this = ColorTransform::hlsTransform(a * ah + b * bh, a * al + b * bl, a * as + b * bs);
                hls = b * hls + f * ct.hls;
                return;
            }
        }
        // catchall
        // not quite perfect, because scaling factors are combined linearly
        hls = b * hls + f * ct.hls;
    }
};


#pragma region Serialization

inline void to_json(json& j, ColorTransform const& t)
{
    // new form:
    if (t.hls == t.hls.eye())
    {
        // "color": "I"
        j = "I";
        return;
    }

    float h, l, s, a;
    if (t.asHlsSink(h, l, s, a))
    {
        j = json{ {"hlsSink", json{h, l, s, a} } };
        return;
    }
    std::vector<float> args;
    if (t.asHlsTransform(args))
    {
    //    j = json{ {"hlsTransform", json{h, l, s} } };
    //    return;
    //}
    //if (t.asHlsTransform(v))
    //{
        j = json{ {"hlsTransform", args } };
        return;
    }

    // legacy: just 4x4 array representing HLS transform
    to_json(j, t.hls);

}

inline void from_json(json const& j, ColorTransform& t)
{
    if (j.is_array())
    {
        // legacy: a 4x4 array representing the HLS transform
        // "color": [[ ... ]]
        from_json(j, t.hls);
    }
    else if (j.is_string())
    {
        // assume "I"
        t = ColorTransform();
    }
    else
    {
        if (j.contains("hlsSink"))
        {
            // new form: "color": { hlsSink: [h,l,s,a] }
            float h = j["hlsSink"][0];
            float l = j["hlsSink"][1];
            float s = j["hlsSink"][2];
            float a = j["hlsSink"][3];
            t = ColorTransform::hlsSink(h, l, s, a);
        }
        else if (j.contains("hlsTransform"))
        {
            auto hls = j["hlsTransform"].get<std::vector<float>>();

            if (hls.size() == 3)
            {
                // hlsTransform legacy: hueshift, lumscale, satscale
                std::vector<float> args{ 1.0f, hls[0], hls[1], 0.0f, hls[2], 0.0f };
                t = ColorTransform::hlsTransform(args);
            }
            else if (hls.size() == 6)
            {
                t = ColorTransform::hlsTransform(hls);
            }
            else
            {
                throw(std::exception("hlsTransform: unexpected parameters"));
            }
        }
        else
        {
            throw(std::exception("Unknown ColorTransform"));
        }
    }
}

#pragma endregion
