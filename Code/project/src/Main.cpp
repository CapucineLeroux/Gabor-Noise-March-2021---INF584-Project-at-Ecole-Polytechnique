#include <fstream>
#include <iostream>
#include <cmath>
#include <ctime>
#include <vector>
#include "vcl/vcl.hpp"

#include "Vec3.h"
//#include "Pseudo_random_number_generator.h"
#include "Noise.h"
#include "Surface_noise.h"
#include "Window_helper.h"

using namespace std;
using namespace vcl;

vector<Vec3f> black_and_white_noise_image (Noise noise, unsigned resolution);
vector<Vec3f> black_and_white_spectrum_image (Noise noise, unsigned resolution);
void save_as_ppm (vector<Vec3f> image, unsigned resolution, string file_name);
int interactive_2D_noise();
int surface_noise_3D(bool map, float m_K, float m_a, float m_F0);
void update_surface_noise(bool map, float m_K, float m_a, float m_F0);
void update_2D_noise();
Vec3f find_color(float t); //gives the linear interpolation of the color scale for t in [0,1]


//same values as the ones in the window helper, don't forget to keep it the same

float K = 1.f;
float a = 0.05; //choose something between 0 and 0.1
float F0_min = 0.125; //choose something between 0 and 1
float F0_max = 0.125; //choose something between 0 and 1
float w0_min = 0;
float w0_max = 2*pi;

float number_of_impulses_per_kernel = 64.f;
unsigned random_offset = time(0);
bool is_periodic = false;

Noise noise(K, a, F0_min, F0_max, w0_min, w0_max, number_of_impulses_per_kernel, random_offset, is_periodic);

//vector<Vec3f> color_scale = {Vec3f(0.9,0.8,0.67),Vec3f(0.6,0.53,0.38)};
vector<Vec3f> color_scale = {Vec3f(1,0,0),Vec3f(0,0,1)};

//for anisotropic noise, F0min=F0max and w0min=w0max (pi/4 for instance)
//for isotropic noise, F0min=F0max and [w0min,w0max]=[0,2pi]
int main(int argc, char** argv){

    //save images of the noise and its power spectrum

    vector<Vec3f> noise_image = black_and_white_noise_image(noise,256);
    save_as_ppm(noise_image, 256, "noise");
    cout<<"noise saved"<<endl;

    vector<Vec3f> spectrum_image = black_and_white_spectrum_image(noise,256);
    save_as_ppm(spectrum_image, 256, "spectrum");
    cout<<"spectrum saved"<<endl;


    //3D interactive visualisation

    interactive_2D_noise();

    //Surface noise

    //put false if want to test the surface noise of the paper
    //put true if want to map the texture
    //surface_noise_3D(false,1.f,0.05f,0.125f);

    return 0;

}



vector<Vec3f> black_and_white_noise_image (Noise noise, unsigned resolution) {

    vector<Vec3f> image;
    image.resize(resolution*resolution);

    float scale = 6.f*sqrt(noise.variance());

    for (unsigned i=0 ; i<resolution ; i++) {
        for (unsigned j=0 ; j<resolution ; j++) {

            //change of coordinates to have the (x,y) axis system centered
            float x = float(i) + 0.5f - float(resolution)/2.f;
            float y = float(resolution - 1 - j) + 0.5f - float(resolution)/2.f;
            float normed_noise_intensity = 0.5 + noise.intensity(x,y)/scale; //the value is centered between 0 and 1

            //save black and white pixel color
            if (normed_noise_intensity <= 0.f) {
                image[i*resolution + j] = Vec3f(0,0,0);
            }

            else if (normed_noise_intensity >= 1.f){
                image[i*resolution + j] = Vec3f(255,255,255);
            }

            else {
                image[i*resolution + j] = normed_noise_intensity*Vec3f(255,255,255);
            }

        }
    }

    return image;

}



vector<Vec3f> black_and_white_spectrum_image (Noise noise, unsigned resolution) {

    vector<Vec3f> image;
    image.resize(resolution*resolution);

    float scale = 6.f*sqrt(noise.variance());

    for (unsigned i=0 ; i<resolution ; i++) {
        for (unsigned j=0 ; j<resolution ; j++) {

            //change of coordinates to have the (x,y) axis system centered
            float fx = float(i) + 0.5f - float(resolution)/2.f;
            float fy = float(resolution - 1 - j) + 0.5f - float(resolution)/2.f;
            fx *= 1.1f*2.f/float(resolution);
            fy *= 1.1f*2.f/float(resolution);
            float normed_spectrum_intensity = noise.power_spectrum(fx,fy);

            //save black and white pixel color
            if (normed_spectrum_intensity <= 0.f) {
                image[i*resolution + j] = Vec3f(0,0,0);
            }

            else if (normed_spectrum_intensity >= 1.f){
                image[i*resolution + j] = Vec3f(255,255,255);
            }

            else {
                image[i*resolution + j] = normed_spectrum_intensity*Vec3f(255,255,255);
            }

        }
    }

    return image;

}



void save_as_ppm (vector<Vec3f> image, unsigned resolution, string file_name) {

    ofstream file;
    file.open("../output/"+file_name+".ppm");
    file<<"P6\n"<<resolution<<" "<<resolution<<"\n255\n";

    Vec3f color;
    for (unsigned i=0 ; i<resolution ; i++) {
        for (unsigned j=0 ; j<resolution ; j++) {
            color = image[i*resolution + j];
            file<<static_cast<unsigned char>(color[0])<<static_cast<unsigned char>(color[1])<<static_cast<unsigned char>(color[2]);   //write as ascii
        }
    }

    file.close();
}



int interactive_2D_noise(){

    int const width = 1280, height = 1024;
    GLFWwindow* window = create_window(width, height);
    window_size_callback(window, width, height);

    imgui_init(window);
    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);

    initialize_2D_data();

    user.fps_record.start();
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
            scene.light = scene.camera.position();
            user.fps_record.update();

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glClear(GL_DEPTH_BUFFER_BIT);
            imgui_create_frame();
            if(user.fps_record.event) {
                    std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
                    glfwSetWindowTitle(window, title.c_str());
            }

            ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
            user.cursor_on_gui = ImGui::IsAnyWindowFocused();

            display_interface();

            if (need_update){
                cout<<"3D surface update"<<endl;
                update_2D_noise();}
            draw(visual,scene);

            ImGui::End();
            imgui_render_frame(window);
            glfwSwapBuffers(window);
            glfwPollEvents();
    }

    imgui_cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;

}


int surface_noise_3D(bool map, float m_K, float m_a, float m_F0){

    int const width = 1280, height = 1024;
    GLFWwindow* window = create_window(width, height);
    window_size_callback(window, width, height);

    imgui_init(window);
    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);

    initialize_3D_data();
    update_surface_noise(map,m_K,m_a,m_F0);

    user.fps_record.start();
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
            scene.light = scene.camera.position();
            user.fps_record.update();

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glClear(GL_DEPTH_BUFFER_BIT);
            imgui_create_frame();
            if(user.fps_record.event) {
                    std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
                    glfwSetWindowTitle(window, title.c_str());
            }

            ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
            user.cursor_on_gui = ImGui::IsAnyWindowFocused();

            draw(visual,scene);

            ImGui::End();
            imgui_render_frame(window);
            glfwSwapBuffers(window);
            glfwPollEvents();
    }

    imgui_cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;

}


//if map= true, consider there is a uv map
void update_surface_noise(bool map, float m_K, float m_a, float m_F0){

    shape = mesh_load_file_obj("../assets/man.obj");
    float const scaling = 0.005f;
    for(auto& p: shape.position) p *= scaling;

    if (map) {

        Noise surface_noise = Noise(m_K, m_a, m_F0, m_F0, 0.f, 2.f*pi, number_of_impulses_per_kernel, random_offset, is_periodic);
        float scale = 6.f*sqrt(surface_noise.variance());

        for (size_t i=0 ; i<shape.position.size() ; i++){

            vec2 p_2D = shape.uv[i];
            float noise_intensity = surface_noise.intensity(500*p_2D[0],500*p_2D[1]);

            if (0.5f + noise_intensity/(scale) <= 0.f) {
                float t = 0.f;
                shape.color[i][0] = find_color(t)[0];
                shape.color[i][1] = find_color(t)[1];
                shape.color[i][2] = find_color(t)[2];
            }
            else if (0.5f + noise_intensity/(scale) >= 1.f) {
                float t = 1.f;
                shape.color[i][0] = find_color(t)[0];
                shape.color[i][1] = find_color(t)[1];
                shape.color[i][2] = find_color(t)[2];
            }
            else {
                float t = 0.5f + noise_intensity/(scale);
                shape.color[i][0] = find_color(t)[0];
                shape.color[i][1] = find_color(t)[1];
                shape.color[i][2] = find_color(t)[2];
            }

        }

    }

    else {

        float m_K = 1.f;
        float m_a = 0.05f;
        float m_F0 = 0.125f;

        Surface_noise surface_noise = Surface_noise(m_K, m_a, m_F0, number_of_impulses_per_kernel, random_offset, is_periodic);
        float scale = 6.f*sqrt(surface_noise.variance());

        for (size_t i=0 ; i<shape.position.size() ; i++){

            vec3 p = shape.position[i];
            vec3 n = shape.normal[i];
            float noise_intensity = surface_noise.intensity(500*p[0],500*p[1],500*p[2],n);

            if (0.5f + noise_intensity/(scale) <= 0.f) {
                float t = 0.f;
                shape.color[i][0] = find_color(t)[0];
                shape.color[i][1] = find_color(t)[1];
                shape.color[i][2] = find_color(t)[2];
            }
            else if (0.5f + noise_intensity/(scale) >= 1.f) {
                float t = 1.f;
                shape.color[i][0] = find_color(t)[0];
                shape.color[i][1] = find_color(t)[1];
                shape.color[i][2] = find_color(t)[2];
            }
            else {
                float t = 0.5f + noise_intensity/(scale);
                shape.color[i][0] = find_color(t)[0];
                shape.color[i][1] = find_color(t)[1];
                shape.color[i][2] = find_color(t)[2];
            }

        }


    }

    visual.clear();
    visual = mesh_drawable(shape);
    visual.shading.phong = {0.3f, 0.6f, 0.05f, 64};

}



void update_2D_noise(){

    K = w_K;
    a = w_a;
    F0_min = w_F0_min;
    F0_max = w_F0_max;
    w0_min = w_w0_min;
    w0_max = w_w0_max;
    is_periodic = w_is_periodic;

    noise = Noise(K, a, F0_min, F0_max, w0_min, w0_max, number_of_impulses_per_kernel, random_offset, is_periodic);

    /**if (w_anisotropic_filtering){
        vector<float> new_params = noise.anisotropically_filter(F0_min,w0_min);
        K = new_params[0];
        a = new_params[1];
        F0_min = new_params[2];
        F0_max = new_params[2];
        w0_min = new_params[3];
        w0_max = new_params[3];
        noise = Noise(K, a, F0_min, F0_max, w0_min, w0_max, number_of_impulses_per_kernel, random_offset, is_periodic);
    }*/

    int N = int(sqrt(shape.position.size()));
    float scale = 6.f*sqrt(noise.variance());

    for (int j=0 ; j<N ; j++) {
        for (int i=0 ; i<N ; i++) {

            vec3 p = shape.position[j*N+i];
            float noise_intensity = noise.intensity(100*p[0],100*p[1]);

            if (w_height_noise) {
                shape.position[j*N+i][2] = w_height_amplitude*noise_intensity/(scale);
            }

            else {
                shape.position[j*N+i][2] = 0.f;
            }

            if (w_color_scale) {
                if (0.5f + noise_intensity/(scale) <= 0.f) {
                    float t = 0.f;
                    shape.color[j*N+i][0] = find_color(t)[0];
                    shape.color[j*N+i][1] = find_color(t)[1];
                    shape.color[j*N+i][2] = find_color(t)[2];
                }
                else if (0.5f + noise_intensity/(scale) >= 1.f) {
                    float t = 1.f;
                    shape.color[j*N+i][0] = find_color(t)[0];
                    shape.color[j*N+i][1] = find_color(t)[1];
                    shape.color[j*N+i][2] = find_color(t)[2];
                }
                else {
                    float t = 0.5f + noise_intensity/(scale);
                    shape.color[j*N+i][0] = find_color(t)[0];
                    shape.color[j*N+i][1] = find_color(t)[1];
                    shape.color[j*N+i][2] = find_color(t)[2];
                }
            }

            else {
                shape.color[j*N+i][0] = 1.f;
                shape.color[j*N+i][1] = 1.f;
                shape.color[j*N+i][2] = 1.f;
            }
        }
    }

    shape = shape.compute_normal();
    visual.clear();
    visual = mesh_drawable(shape);
    visual.shading.phong = {0.3f, 0.6f, 0.05f, 64};

}



Vec3f find_color(float t){

    int n = color_scale.size();
    int i = floor(t*float(n-1));

    if (i==n-1) {
        return color_scale[n-1];
    }

    else {
        float tbis = (t-float(i)/float(n-1))/(float(i+1)/float(n-1)-float(i)/float(n-1));
        return (1.f-tbis)*color_scale[i] + tbis*color_scale[i+1];
    }
}



