#pragma once

#include <fstream>
#include <iostream>
#include <cmath>
#include "vcl/vcl.hpp"
#include "Pseudo_random_number_generator.h"

using namespace std;
using namespace vcl;

class Noise {

    public:

        Noise (float K, float a, float F0_min, float F0_max, float w0_min, float w0_max, float number_of_impulses_per_kernel, unsigned random_offset, bool is_periodic, unsigned period=256.f)
        :  m_K(K), m_a(a), m_F0_min(F0_min), m_F0_max(F0_max), m_w0_min(w0_min), m_w0_max(w0_max), m_random_offset(random_offset), m_is_periodic(is_periodic), m_period(period)
        {
            m_kernel_radius = 1.f/m_a;
            m_impulse_density = number_of_impulses_per_kernel/(pi*pow(m_kernel_radius,2));
        }



        float intensity (float x, float y) {

            x = x/m_kernel_radius ;
            y = y/m_kernel_radius ;


            float frac_x = x-floor(x);
            float frac_y = y-floor(y);

            float noise_intensity = 0.f;

            for (int i=-1 ; i<=1 ; i++) {
                for (int j=-1 ; j<=1 ; j++) {
                    noise_intensity += cell_noise(floor(x) + i, floor(y) + j, frac_x - i, frac_y - j);
                }
            }

            return noise_intensity;

        }



        float cell_noise (int i, int j, float x, float y) {

            unsigned seed;

            if (m_is_periodic) { seed = ((unsigned)j % m_period)*m_period + ((unsigned)i % m_period) + m_random_offset; } //periodic noise
            else { seed = morton(i, j) + m_random_offset; } // nonperiodic noise

            if (seed == 0) {seed = 1;}

            Pseudo_random_number_generator prng(seed);

            float number_of_impulses_per_cell = m_impulse_density*pow(m_kernel_radius,2);
            unsigned number_of_impulses = prng.poisson(number_of_impulses_per_cell);

            float noise = 0.f;
            for (unsigned i=0 ; i<number_of_impulses ; i++) {

              float xi = prng.uniform_0_1();
              float yi = prng.uniform_0_1();
              float wi = prng.uniform(-1,1);

              float F0i = prng.uniform(m_F0_min, m_F0_max);
              float w0i = prng.uniform(m_w0_min, m_w0_max);

              if ((pow(x-xi,2) + pow(y-yi,2)) < 1.f) {
                noise += wi*gabor(m_K, m_a, F0i, w0i, (x-xi)*m_kernel_radius, (y-yi)*m_kernel_radius); // anisotropic if F0min=F0max and w0min=w0max, isotropic if F0min=F0max and w0min=0,w0max=2pi
              }

            }

            return noise;

        }



        unsigned morton (unsigned x, unsigned y) {
            unsigned z = 0;
            for (unsigned i = 0; i < (sizeof(unsigned) * CHAR_BIT); ++i) {
                z |= ((x & (1 << i)) << i) | ((y & (1 << i)) << (i + 1));
            }
            return z;
        }



        float gabor (float K, float a, float F0, float w0, float x, float y) {
            float gaussian = K*exp( -pi*pow(a,2)*(pow(x,2) + pow(y,2)) );
            float harmonic = cos( 2.f*pi*F0*(x*cos(w0) + y*sin(w0)) );
            return gaussian*harmonic;
        }


        float variance() {

            int N_steps;
            float dF;
            float integral;

            if (m_F0_max-m_F0_min > pow(10,-2)) {

                N_steps = 100;
                dF = (m_F0_max-m_F0_min)/float(N_steps-1);

                integral = 0.f;
                for (int Fi=0 ; Fi<N_steps ; Fi++){
                    float F0 = float(Fi)*dF;
                    integral += dF*(1.f + exp(-2.f*pi*pow(F0,2)/pow(m_a,2)));
                }

                return integral*m_impulse_density*pow(m_K,2)/(12.f*pow(m_a,2)*(m_F0_max-m_F0_min));

            }

            else {
                return ((m_impulse_density*pow(m_K,2))/(12.f*pow(m_a,2))) * (1.f + exp(-2.f*pi*pow(m_F0_min,2)/pow(m_a,2)));
            }
        }


        float gabor_fourier_transform (float K, float a, float F0, float w0, float fx, float fy) {
            return ( exp( -(pow(fx-F0*cos(w0),2) + pow(fy-F0*sin(w0),2))*pi/pow(a,2) ) + exp( -(pow(fx+F0*cos(w0),2) + pow(fy+F0*sin(w0),2))*pi/pow(a,2) ) )*K/(2.f*pow(a,2));
        }



        float power_spectrum (float fx, float fy){

            int N_steps;
            float dF;
            float dw;
            float integral;

            if (m_F0_max-m_F0_min <= pow(10,-2) && m_w0_max-m_w0_min <= pow(10,-2)) {
                float G = gabor_fourier_transform(m_K, m_a, m_F0_min, m_w0_min, fx, fy);
                return pow(fabs(G),2)*m_impulse_density/3.f;
            }

            else if (m_F0_max-m_F0_min <= pow(10,-2)) {

                N_steps = 100;
                dw = (m_w0_max-m_w0_min)/float(N_steps-1);

                integral = 0.f;
                for (int wi=0 ; wi<N_steps ; wi++){
                    integral += dw*pow(fabs(gabor_fourier_transform(m_K, m_a, m_F0_min, m_w0_min + float(wi)*dw, fx, fy)),2);
                }

                return integral*m_impulse_density/(3.f*(m_w0_max-m_w0_min));

            }

            else if (m_w0_max-m_w0_min <= pow(10,-2)) {

                N_steps = 100;
                dF = (m_F0_max-m_F0_min)/float(N_steps-1);

                integral = 0.f;
                for (int Fi=0 ; Fi<N_steps ; Fi++){
                    integral += dF*pow(fabs(gabor_fourier_transform(m_K, m_a, m_F0_min + float(Fi)*dF, m_w0_min, fx, fy)),2);
                }

                return integral*m_impulse_density/(3.f*(m_F0_max-m_F0_min));

            }

            else {

                N_steps = 30;
                dF = (m_F0_max-m_F0_min)/float(N_steps-1);
                dw = (m_w0_max-m_w0_min)/float(N_steps-1);

                integral = 0.f;
                for (int wi=0 ; wi<N_steps ; wi++){
                    for (int Fi=0 ; Fi<N_steps ; Fi++){
                        integral += dw*dF*pow(fabs(gabor_fourier_transform(m_K, m_a, m_F0_min + float(Fi)*dF, m_w0_min + float(wi)*dw, fx, fy)),2);
                    }
                }

                return integral*m_impulse_density/(3.f*(m_F0_max-m_F0_min)*(m_w0_max-m_w0_min));

            }

        }


        //with J = I and an anisotropic noise
        /**vector<float> anisotropically_filter(float F0, float w0){

            mat2 I;
            I(0,0) = 1;
            I(0,1) = 0;
            I(1,0) = 0;
            I(1,1) = 1;

            float sigma = pow(variance(),0.5);

            //mat2 sigmaF = inverse(4.f*pow(pi,2)*pow(sigma,2)*J*transpose(J));
            mat2 sigmaF = (1.f/pow(2*pi*sigma,2))*I;
            mat2 sigmaF_inverse = pow(2*pi*sigma,2)*I;
            //mat2 sigmaG = (pow(m_a,2)/(2.f*pi))*I;
            mat2 sigmaG = (pow(m_a,2)/(2.f*pi))*I;
            mat2 sigmaG_inverse = ((2*pi)/pow(m_a,2))*I;
            //mat2 sigmaFG = inverse( inverse(sigmaF) + inverse(sigmaG) );
            mat2 sigmaFG = (1.f/(pow(2*pi*sigma,2) + 2*pi/pow(m_a,2)))*I;
            mat2 sigmaFG_inverse = (pow(2*pi*sigma,2) + 2*pi/pow(m_a,2))*I;

            vec2 muG(F0*cos(w0),F0*sin(w0));
            //vec2 muFG = sigmaFG*inverse(sigmaG)*muG;
            vec2 muFG = sigmaFG*sigmaG_inverse*muG;

            float new_w0 = atan2(muFG[1],muFG[0]);

            float new_F0;
            if (fabs(cos(new_w0)) > pow(10,-2)) {
                new_F0 = muFG[0]/cos(new_w0);
            }
            else {
                new_F0 = muFG[1]/sin(new_w0);
            }

            float new_a = pow(2*pi*pow(norm(sigmaFG),0.5),0.5);
            mat2 M = (1.f/(1.f/pow(2*pi*sigma,2) + pow(m_a,2)/(2*pi)))*I;
            float new_K = (m_K*pow(new_a,2)/pow(m_a,2))*exp(-0.5f*dot(muG,M*muG));

            return vector<float>{new_K,new_a,new_F0,new_w0};

        }*/



    private:

        float m_K;
        float m_a;
        float m_F0_min;
        float m_F0_max;
        float m_w0_min;
        float m_w0_max;
        float m_kernel_radius;
        float m_impulse_density;
        unsigned m_random_offset;
        bool m_is_periodic;
        unsigned m_period;

};
