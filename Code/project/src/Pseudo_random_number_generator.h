#pragma once

#include <fstream>
#include <iostream>
#include <cmath>

using namespace std;

class Pseudo_random_number_generator {

    public:

        Pseudo_random_number_generator (unsigned seed = 0.f) {
            m_x = seed;
        }

        unsigned& x () {
            return m_x;
        }


        unsigned next () {
            m_x *= 3039177861u;
            return m_x;
        }


        float uniform_0_1 () {
            return float(next()) / float(UINT_MAX);
        }


        float uniform (float min, float max) {
            return min + (uniform_0_1() * (max - min));
        }


        unsigned poisson (float mean) {

            float g = exp(-mean);
            unsigned alpha = 0.f;
            float t = uniform_0_1();

            while (t > g) {
                alpha += 1.f;
                t *= uniform_0_1();
            }

            return alpha;
        }


    private:

        unsigned m_x;

};
