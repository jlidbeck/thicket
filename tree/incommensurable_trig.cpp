#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <array>
#include <numeric>


using std::cout;
using std::endl;


#pragma region iv<>: Incommensurable vector representation of trig values


//  Exact trig computations for a finite subset of angles
//  Computed via a vector of incommensurables
//  div: Number of divisions of 90 degrees
//  e.g. iv<16, 30> computes exact values for trig identities for multiples of 3 degrees.
//  iv<16, 30>::sin(15) returns a vector representing the exact value of sin(45 degrees).
template<int _N, int _AngleDiv>
class iv
{
protected:
    static const std::array<double, _N> s_icommValues;
    static const std::array<iv<_N, _AngleDiv>, 1+_AngleDiv> s_sintable;

public:
    std::array<int, _N> values;

    iv() { values.fill(0); }

    iv(std::array<int, _N> const &v) { values = v; }

    iv(std::initializer_list<int> const& list) {
        if (list.size() == _N)
        {
            std::copy(list.begin(), list.end(), values.begin());
        }
    }

    iv operator-() const {
        std::array<int, _N> ret;
        for (int i = 0; i < _N; ++i)
            ret[i] = -values[i];
        return ret;
    }

    void operator+=(iv<_N, _AngleDiv> const& v)
    {
        for (int i = 0; i < _N; ++i)
            values[i] += v.values[i];
    }

    bool operator==(iv<_N, _AngleDiv> const& v)
    {
        for (int i = 0; i < _N; ++i)
            if (values[i] != v.values[i])
                return false;
        return true;
    }

    operator double() const {
        return std::inner_product(s_icommValues.begin(), s_icommValues.end(), values.begin(), 0.0);
    }


    static iv<_N, _AngleDiv> sin(int a);
    static iv<_N, _AngleDiv> cos(int a);

};


#pragma region Templated trig fns: values from sine table

template<int _N, int _AngleDiv>
iv<_N, _AngleDiv> iv<_N, _AngleDiv>::cos(int a)
{
    a = ((a %= (4*_AngleDiv)) < 0) ? a + (4*_AngleDiv) : a;

    if (a <= (1*_AngleDiv)) return  s_sintable[(1*_AngleDiv) - a];
    if (a <= (2*_AngleDiv)) return -s_sintable[a - (1*_AngleDiv)];
    if (a <= (3*_AngleDiv)) return -s_sintable[(3*_AngleDiv) - a];
    return                          s_sintable[a - (3*_AngleDiv)];
}

template<int _N, int _AngleDiv>
iv<_N, _AngleDiv> iv<_N, _AngleDiv>::sin(int a)
{
    a = ((a %= (4*_AngleDiv)) < 0) ? a + (4*_AngleDiv) : a;

    if (a <= (1*_AngleDiv)) return  s_sintable[ a];
    if (a <= (2*_AngleDiv)) return  s_sintable[ (2*_AngleDiv) - a];
    if (a <= (3*_AngleDiv)) return -s_sintable[ a - (2*_AngleDiv)];
    return                         -s_sintable[ (4*_AngleDiv) - a];
}

#pragma endregion


#pragma region Template instantiations


#pragma region iv<16, 30> (factors of 3 degrees)


const std::array<double, 16> iv<16, 30>::s_icommValues = {
    0.1250000,      // 1/8
    0.0883883,      // √2/16
    0.2165064,      // √3/8
    0.1530931,
    0.2795085,
    0.1976424,
    0.4841229,
    0.3423266,
    0.2078135,      // √(5-√5)/8
    0.3362493,      // √(5+√5)/8
    0.2938926,
    0.4755283,
    0.3599435,
    0.5824008,
    0.5090370,
    0.8236391 };

const std::array<iv<16, 30>, 31> iv<16, 30>::s_sintable = { {
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },       // sin(0)
    {  0,-1, 0,-1, 0, 1, 0, 1, 0, 1, 0, 0, 0,-1, 0, 0  },
    { -1, 0, 0, 0,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0  },
    {  0, 2, 0, 0, 0, 2, 0, 0,-2, 0, 0, 0, 0, 0, 0, 0  },
    {  0, 0, 1, 0, 0, 0,-1, 0, 0, 0, 0, 1, 0, 0, 0, 0  },
    {  0,-4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },       // sin(15)
    { -2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
    {  0, 1, 0,-1, 0, 1, 0,-1, 1, 0, 0, 0, 1, 0, 0, 0  },
    {  0, 0, 1, 0, 0, 0, 1, 0, 0, 0,-1, 0, 0, 0, 0, 0  },
    {  0, 2, 0, 0, 0,-2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0  },
    {  4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },       // sin(30)
    {  0,-1, 0,-1, 0, 1, 0, 1, 0,-1, 0, 0, 0, 1, 0, 0  },
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0  },
    {  0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0,-1, 0, 0, 0  },
    {  1, 0, 0, 0,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1  },
    {  0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },       // sin(45)
    {  0, 0,-1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0  },
    {  0,-1, 0, 1, 0,-1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0  },
    {  2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
    {  0,-1, 0, 1, 0, 1, 0,-1, 0, 1, 0, 0, 0, 1, 0, 0  },
    {  0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },       // sin(60)
    {  0,-2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0  },
    {  1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0  },
    {  0, 1, 0, 1, 0, 1, 0, 1,-1, 0, 0, 0, 1, 0, 0, 0  },
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0  },
    {  0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },       // sin(75)
    { -1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1  },
    {  0, 2, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0  },
    {  0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0  },
    {  0, 1, 0,-1, 0,-1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0  },
    {  8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  }        // sin(90)
} };

#pragma endregion

#pragma region iv<8, 15> (factors of 6 degrees)

const std::array<double, 8> iv<8, 15>::s_icommValues = {
    0.1250000,
    0.2165064,
    0.2795085,
    0.4841229,
    0.2938926,
    0.4755283,
    0.5090370,
    0.8236391 };

const std::array<iv<8, 15>, 16> iv<8, 15>::s_sintable = { {
    {  0, 0, 0, 0, 0, 0, 0, 0  },
    { -1, 0,-1, 0, 0, 0, 1, 0  },
    {  0, 1, 0,-1, 0, 1, 0, 0  },
    { -2, 0, 2, 0, 0, 0, 0, 0  },
    {  0, 1, 0, 1,-1, 0, 0, 0  },
    {  4, 0, 0, 0, 0, 0, 0, 0  },
    {  0, 0, 0, 0, 2, 0, 0, 0  },
    {  1, 0,-1, 0, 0, 0, 0, 1  },
    {  0,-1, 0, 1, 0, 1, 0, 0  },
    {  2, 0, 2, 0, 0, 0, 0, 0  },
    {  0, 4, 0, 0, 0, 0, 0, 0  },
    {  1, 0, 1, 0, 0, 0, 1, 0  },
    {  0, 0, 0, 0, 0, 2, 0, 0  },
    { -1, 0, 1, 0, 0, 0, 0, 1  },
    {  0, 1, 0, 1, 1, 0, 0, 0  },
    {  8, 0, 0, 0, 0, 0, 0, 0  }
} };

#pragma endregion

#pragma region iv<2, 3> (factors of 30 degrees)


const std::array<double, 2> iv<2, 3>::s_icommValues = {
    0.5,
    0.8660254       // √3/2
};

const std::array<iv<2, 3>, 4> iv<2, 3>::s_sintable = { {
    {  0, 0  },
    {  1, 0  },     // sin(30) == 1/2
    {  0, 1  },     // sin(60) == √3/2
    {  2, 0  }
} };

#pragma endregion

#pragma region iv<2, 2> (factors of 45 degrees)

const std::array<double, 2> iv<2, 2>::s_icommValues = {
    1.0,
    0.7071068 };

const std::array<iv<2, 2>, 3> iv<2, 2>::s_sintable = { {
    {  0, 0  },
    {  0, 1  },
    {  1, 0  }
} };

#pragma endregion


#pragma endregion


#pragma endregion


template<int _N, int _AngleDiv>
void test()
{
    cout << "\nIV test: " << _AngleDiv << " divisions (" << (90 / _AngleDiv) << " degrees): exact representation as vector of size " << _N << endl;

    iv<_N, _AngleDiv> zero;
    cout << "zero test: " << (double)zero << " iszero: " 
        << (zero == -zero)
        << (zero == iv<_N, _AngleDiv>::sin(0))
        << (zero == iv<_N, _AngleDiv>::sin(  6 * _AngleDiv))
        << (zero == iv<_N, _AngleDiv>::sin( 12 * _AngleDiv))
        << (zero == iv<_N, _AngleDiv>::sin( 18 * _AngleDiv))
        << (zero == iv<_N, _AngleDiv>::sin( -6 * _AngleDiv)) // sin(-180 deg)
        << (zero == iv<_N, _AngleDiv>::sin(-12 * _AngleDiv))
        << (zero == iv<_N, _AngleDiv>::sin(-18 * _AngleDiv))
        << endl;

    iv<_N, _AngleDiv> val;
    iv<_N, _AngleDiv> totalCos, totalSin;
    for (int a = 0; a <= 4*_AngleDiv; ++a)
    {
        iv<_N, _AngleDiv> c = iv<_N, _AngleDiv>::cos(a);
        iv<_N, _AngleDiv> s = iv<_N, _AngleDiv>::sin(a);
        totalCos += c;
        totalSin += s;
        double vcos = (double)(c);
        double vsin = (double)(s);
        double arad = a * std::_Pi / 2.0 / _AngleDiv;
        double testCos = cos(arad);
        double testSin = sin(arad);
        std::cout << std::setw(3) << (a * 90 / _AngleDiv)
            << " cos:" << std::setw(9) << vcos
            << " sin:" << std::setw(9) << vsin
            << " hyp:" << std::setw(9) << (vcos*vcos + vsin * vsin)
            << " err:" << std::setw(4) << ((int)(1000.0*abs(testCos - vcos)))/1000.0
            << " total==0:"<<(zero==totalCos)<<(zero==totalSin)
            << endl;
    }

}

int runTests()
{
    test<16, 30>();
    test<8, 15>();
    test<2, 3>();
    test<2, 2>();

    return 0;
}
