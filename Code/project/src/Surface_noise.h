#pragma once

#include <fstream>
#include <iostream>
#include <cmath>
#include "vcl/vcl.hpp"
#include "Pseudo_random_number_generator.h"

using namespace std;
using namespace vcl;

//only isotropic noise

class Surface_noise {

    public:

        Surface_noise (float K, float a, float F0, float number_of_impulses_per_kernel, unsigned random_offset, bool is_periodic, unsigned period=256.f)
        :  m_K(K), m_a(a), m_F0(F0), m_random_offset(random_offset), m_is_periodic(is_periodic), m_period(period)
        {
            m_kernel_radius = 1.f/m_a;
            m_impulse_density = number_of_impulses_per_kernel/(pi*pow(m_kernel_radius,2));
        }


        float intensity (float x, float y, float z, vec3 n) {

            x = x/m_kernel_radius ;
            y = y/m_kernel_radius ;
            z = z/m_kernel_radius ;


            float frac_x = x-floor(x);
            float frac_y = y-floor(y);
            float frac_z = y-floor(y);

            float noise_intensity = 0.f;

            for (int i=-1 ; i<=1 ; i++) {
                for (int j=-1 ; j<=1 ; j++) {
                    for (int k=-1 ; k<=1 ; k++) {
                        noise_intensity += cell_noise(floor(x) + i, floor(y) + j, floor(z) + k, frac_x - i, frac_y - j, frac_z - k, n);
                    }
                }
            }

            return noise_intensity;

        }


        float cell_noise (int i, int j, int k, float x, float y, float z, vec3 n) {

            unsigned seed;

            if (m_is_periodic) { seed = ((unsigned)j % m_period)*m_period + ((unsigned)i % m_period) + m_random_offset; } //periodic noise
            else { seed = morton(i, j) + m_random_offset; } // nonperiodic noise

            if (seed == 0) {seed = 1;}

            Pseudo_random_number_generator prng(seed);

            float number_of_impulses_per_cell = m_impulse_density*pow(m_kernel_radius,3);
            unsigned number_of_impulses = prng.poisson(number_of_impulses_per_cell);

            float noise = 0.f;

            for (unsigned i=0 ; i<number_of_impulses ; i++) {

              float xi = prng.uniform_0_1();
              float yi = prng.uniform_0_1();
              float zi = prng.uniform_0_1();
              vec3 pi = {xi,yi,zi};
              vec3 p = {x,y,z};
              float wi = 1.f - norm(pi-projection_3D(pi,p,n));

              float w0i = prng.uniform(0, 2.f*3.14f);

              vec2 pibis = projection_2D(pi,p,n);
              vec2 pbis = projection_2D(p,p,n);

              if (norm(pbis-pibis) < 1.f) {
                noise += wi*gabor(m_K, m_a, m_F0, w0i, m_kernel_radius*(pbis-pibis)[0], m_kernel_radius*(pbis-pibis)[1]);
              }

            }

            return noise;

        }

        //Projects point M on the plane define by point p and vector n
        vec3 projection_3D (vec3 M, vec3 p, vec3 n){
            float alpha = dot(p-M,n)/dot(n,n);
            return M + alpha*n;
        }

        vec2 projection_2D (vec3 M, vec3 p, vec3 n){

            vec3 p_orth = projection_3D(M,p,n);
            vec3 u1;
            if (fabs(n[0]) > pow(10,-2)) {
                u1 = {(dot(p,n)-n[1])/n[0],1,0};
                if (norm(u1-p)<pow(10,-2)){
                    u1 = {(dot(p,n)-n[2])/n[0],0,1};
                }
            }
            else if (fabs(n[1]) > pow(10,-2)) {
                u1 = {1,(dot(p,n)-n[0])/n[1],0};
                if (norm(u1-p)<pow(10,-2)){
                    u1 = {0,(dot(p,n)-n[2])/n[1],1};
                }
            }
            else {
                u1 = {1,0,(dot(p,n)-n[0])/n[2]};
                if (norm(u1-p)<pow(10,-2)){
                    u1 = {0,1,(dot(p,n)-n[1])/n[2]};
                }
            }

            u1 = (u1)/norm(u1) ;
            vec3 u2 = cross(n,u1);

            return dot(p_orth,u1)*vec2(1,0) + dot(p_orth,u2)*vec2(0,1);

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
            return ((m_impulse_density*pow(m_K,2))/(12.f*pow(m_a,2))) * (1.f + exp(-2.f*pi*pow(m_F0,2)/pow(m_a,2)));
        }


        float gabor_fourier_transform (float K, float a, float F0, float w0, float fx, float fy) {
            return ( exp( -(pow(fx-F0*cos(w0),2) + pow(fy-F0*sin(w0),2))*pi/pow(a,2) ) + exp( -(pow(fx+F0*cos(w0),2) + pow(fy+F0*sin(w0),2))*pi/pow(a,2) ) )*K/(2.f*pow(a,2));
        }


    private:

        float m_K;
        float m_a;
        float m_F0;
        float m_kernel_radius;
        float m_impulse_density;
        unsigned m_random_offset;
        bool m_is_periodic;
        unsigned m_period;

};
