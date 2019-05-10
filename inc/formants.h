#ifndef FORMANTS_H
#define FORMANTS_H


#include <valarray>
#include <vector>

using valarray = std::valarray<double>;

struct formant {
    double freq;
    double bw;
    double mag;
};

std::vector<formant> estimateFormants(const valarray& u, int nb);


#endif // FORMANTS_H