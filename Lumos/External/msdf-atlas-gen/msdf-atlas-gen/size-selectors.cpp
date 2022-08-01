
#include "size-selectors.h"

#include <cmath>

namespace msdf_atlas {

template <int MULTIPLE>
SquareSizeSelector<MULTIPLE>::SquareSizeSelector(int minArea) : lowerBound(0), upperBound(-1) {
    if (minArea > 0)
        lowerBound = int(sqrt(minArea-1))/MULTIPLE+1;
    updateCurrent();
}

template <int MULTIPLE>
void SquareSizeSelector<MULTIPLE>::updateCurrent() {
    if (upperBound < 0)
        current = 5*lowerBound/4+16/MULTIPLE;
    else
        current = lowerBound+(upperBound-lowerBound)/2;
}

template <int MULTIPLE>
bool SquareSizeSelector<MULTIPLE>::operator()(int &width, int &height) const {
    width = MULTIPLE*current, height = MULTIPLE*current;
    return lowerBound < upperBound || upperBound < 0;
}

template <int MULTIPLE>
SquareSizeSelector<MULTIPLE> & SquareSizeSelector<MULTIPLE>::operator++() {
    lowerBound = current+1;
    updateCurrent();
    return *this;
}

template <int MULTIPLE>
SquareSizeSelector<MULTIPLE> & SquareSizeSelector<MULTIPLE>::operator--() {
    upperBound = current;
    updateCurrent();
    return *this;
}

template class SquareSizeSelector<1>;
template class SquareSizeSelector<2>;
template class SquareSizeSelector<4>;

SquarePowerOfTwoSizeSelector::SquarePowerOfTwoSizeSelector(int minArea) : side(1) {
    while (side*side < minArea)
        side <<= 1;
}

bool SquarePowerOfTwoSizeSelector::operator()(int &width, int &height) const {
    width = side, height = side;
    return side > 0;
}

SquarePowerOfTwoSizeSelector & SquarePowerOfTwoSizeSelector::operator++() {
    side <<= 1;
    return *this;
}

SquarePowerOfTwoSizeSelector & SquarePowerOfTwoSizeSelector::operator--() {
    side = 0;
    return *this;
}

PowerOfTwoSizeSelector::PowerOfTwoSizeSelector(int minArea) : w(1), h(1) {
    while (w*h < minArea)
        ++*this;
}

bool PowerOfTwoSizeSelector::operator()(int &width, int &height) const {
    width = w, height = h;
    return w > 0 && h > 0;
}

PowerOfTwoSizeSelector & PowerOfTwoSizeSelector::operator++() {
    if (w == h)
        w <<= 1;
    else
        h = w;
    return *this;
}

PowerOfTwoSizeSelector & PowerOfTwoSizeSelector::operator--() {
    w = 0, h = 0;
    return *this;
}

}
