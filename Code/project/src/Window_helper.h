#include "vcl/vcl.hpp"
#include <iostream>

#include "Noise.h"

using namespace std;
using namespace vcl;

struct user_interaction_parameters {
        vec2 mouse_prev;
        timer_fps fps_record;
        mesh_drawable global_frame;
        bool cursor_on_gui;
};
user_interaction_parameters user;


struct scene_environment
{
        camera_around_center camera;
        mat4 projection;
        vec3 light;
};
scene_environment scene;

mesh shape;            // Mesh structure of the deformed shape
mesh_drawable visual;  // Visual representation of the deformed shape
bool need_update = false;

//values recorded on the window interface
float w_K = 1.f;
float w_a = 0.05; //choose something between 0 and 0.1
float w_F0_min = 0.125; //choose something between 0 and 1
float w_F0_max = 0.225; //choose something between 0 and 1
float w_F0 = 0.225;
float w_w0_min = 0.f;
float w_w0_max = pi/4.f;
float w_w0 = pi/4.f;
float w_height_amplitude = 1.f/20.f;
bool w_is_periodic = false;
bool w_isotropic = false;
bool w_anisotropic = false;
//bool w_anisotropic_filtering = false;
bool w_customed = (!w_isotropic) && (!w_anisotropic);
bool w_height_noise = true;
bool w_color_scale = false;
bool w_cylinder = true;

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);

void initialize_2D_data();
void initialize_3D_data();
void display_interface();
void update_surface();


void initialize_2D_data()
{
        GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
        GLuint const shader_uniform_color = opengl_create_shader_program(opengl_shader_preset("single_color_vertex"), opengl_shader_preset("single_color_fragment"));
        GLuint const texture_white = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});
        mesh_drawable::default_shader = shader_mesh;
        mesh_drawable::default_texture = texture_white;
        curve_drawable::default_shader = shader_uniform_color;
        segments_drawable::default_shader = shader_uniform_color;

        user.global_frame = mesh_drawable(mesh_primitive_frame());
        scene.camera.distance_to_center = 2.5f;

        int const N = 500;
        shape = mesh_primitive_grid({-1,-1,0}, {1,-1,0}, {1,1,0}, {-1,1,0}, N, N);
        visual = mesh_drawable(shape);

}


void initialize_3D_data()
{
        GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
        GLuint const shader_uniform_color = opengl_create_shader_program(opengl_shader_preset("single_color_vertex"), opengl_shader_preset("single_color_fragment"));
        GLuint const texture_white = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});
        mesh_drawable::default_shader = shader_mesh;
        mesh_drawable::default_texture = texture_white;
        curve_drawable::default_shader = shader_uniform_color;
        segments_drawable::default_shader = shader_uniform_color;

        user.global_frame = mesh_drawable(mesh_primitive_frame());
        scene.camera.distance_to_center = 2.5f;

        shape = mesh_load_file_obj("../assets/man.obj");
        float const scaling = 0.005f;
        for(auto& p: shape.position) p *= scaling;
        visual = mesh_drawable(shape);
}


void display_interface()
{

        ImGui::Text("Gabor Noise "); ImGui::Spacing(); ImGui::Spacing();

        ImGui::Checkbox("Height noise", &w_height_noise);
        ImGui::Spacing(); ImGui::Spacing();

        if (w_height_noise) {
            ImGui::Text("Height amplitude : ");
            ImGui::SliderFloat("",&w_height_amplitude, 0.f, 1.0f);
            ImGui::Spacing(); ImGui::Spacing();
        }

        ImGui::Checkbox("Color noise", &w_color_scale);
        ImGui::Spacing(); ImGui::Spacing();

        ImGui::Text("Gaussian Magnitude : ");
        ImGui::SliderFloat(" K", &w_K, 0.01f, 2.0f);

        ImGui::Text("Gaussian Width : ");
        ImGui::SliderFloat(" a", &w_a, 0.f, 0.1f);

        ImGui::Checkbox("Periodic noise", &w_is_periodic);
        ImGui::Spacing();ImGui::Spacing();

        ImGui::Spacing();ImGui::Spacing();
        ImGui::Checkbox("Isotropic", &w_isotropic);
        ImGui::Checkbox("Anisotropic", &w_anisotropic);
        ImGui::Spacing();ImGui::Spacing();

        w_customed = (!w_isotropic) && (!w_anisotropic);

        /**if (w_anisotropic){
            ImGui::Checkbox("Anisotropic filtering", &w_anisotropic_filtering);
            ImGui::Spacing();ImGui::Spacing();
        }
        else {
            w_anisotropic_filtering = false;
        }*/

        if (w_isotropic && !w_anisotropic) {

            ImGui::Text("Frequency magnitude : ");
            ImGui::SliderFloat(" F0", &w_F0, 0.f, 1.f);
            w_F0_min = w_F0;
            w_F0_max = w_F0;

            w_w0_min = 0.f;
            w_w0_max = 2.f*pi;

        }

        else if (!w_isotropic && w_anisotropic) {

            ImGui::Text("Frequency magnitude : ");
            ImGui::SliderFloat(" F0", &w_F0, 0.f, 1.f);
            w_F0_min = w_F0;
            w_F0_max = w_F0;

            ImGui::Text("Frequency orientation : ");
            ImGui::SliderFloat(" w0", &w_w0, 0.f, 2.f*pi);
            w_w0_min = w_w0;
            w_w0_max = w_w0;

        }

        else if (w_customed) {

            ImGui::Spacing(); ImGui::Spacing();

            ImGui::Text("Frequency magnitude : ");
            ImGui::Text("F0");
            ImGui::SliderFloat(" F0_min", &w_F0_min, 0.f, 1.f);
            ImGui::SliderFloat(" F0_max", &w_F0_max, 0.f, 1.f);

            ImGui::Spacing();

            ImGui::Text("Frequency orientation : ");
            ImGui::Text("w0");
            ImGui::SliderFloat(" w0_min", &w_w0_min, 0.f, 2.f*pi);
            ImGui::SliderFloat(" w0_max", &w_w0_max, 0.f, 2.f*pi);

            ImGui::Spacing();

        }

        if (!(w_isotropic && w_anisotropic) && w_F0_min <= w_F0_max &&  w_w0_min <= w_w0_max) {
            ImGui::Spacing();ImGui::Spacing();
            need_update = ImGui::Button("Apply changes");
        }

}


void window_size_callback(GLFWwindow* , int width, int height)
{
        glViewport(0, 0, width, height);
        float const aspect = width / static_cast<float>(height);
        scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.1f, 100.0f);
}


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
        vec2 const  p1 = glfw_get_mouse_cursor(window, xpos, ypos);
        vec2 const& p0 = user.mouse_prev;

        glfw_state state = glfw_current_state(window);

        auto& camera = scene.camera;
        if(!user.cursor_on_gui){
                if(state.mouse_click_left && !state.key_ctrl)
                        scene.camera.manipulator_rotate_trackball(p0, p1);
                if(state.mouse_click_left && state.key_ctrl)
                        camera.manipulator_translate_in_plane(p1-p0);
                if(state.mouse_click_right)
                        camera.manipulator_scale_distance_to_center( (p1-p0).y );
        }

        user.mouse_prev = p1;
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
        opengl_uniform(shader, "projection", current_scene.projection);
        opengl_uniform(shader, "view", scene.camera.matrix_view());
        opengl_uniform(shader, "light", scene.light, false);
}

