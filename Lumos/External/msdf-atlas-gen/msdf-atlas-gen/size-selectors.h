
#pragma once

namespace msdf_atlas {

// The size selector classes are used to select the minimum dimensions of the atlas fitting a given constraint.

/// Selects square dimensions which are also a multiple of MULTIPLE
template <int MULTIPLE = 1>
class SquareSizeSelector {

public:
    explicit SquareSizeSelector(int minArea = 0);
    bool operator()(int &width, int &height) const;
    SquareSizeSelector<MULTIPLE> & operator++();
    SquareSizeSelector<MULTIPLE> & operator--();

private:
    int lowerBound, upperBound;
    int current;

    void updateCurrent();

};

/// Selects square power-of-two dimensions
class SquarePowerOfTwoSizeSelector {

public:
    explicit SquarePowerOfTwoSizeSelector(int minArea = 0);
    bool operator()(int &width, int &height) const;
    SquarePowerOfTwoSizeSelector & operator++();
    SquarePowerOfTwoSizeSelector & operator--();

private:
    int side;

};

/// Selects square or rectangular (2:1) power-of-two dimensions
class PowerOfTwoSizeSelector {

public:
    explicit PowerOfTwoSizeSelector(int minArea = 0);
    bool operator()(int &width, int &height) const;
    PowerOfTwoSizeSelector & operator++();
    PowerOfTwoSizeSelector & operator--();

private:
    int w, h;

};

}
