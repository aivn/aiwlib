#ifndef MPLT_HPP
#define MPLT_HPP
#include "mesh"
using namespace aiw;
class Mplt:public Plottable, public Mesh<float, 3>{
    protected:
        Texture3D data;
        GLint unif_step, unif_size, unif_raystep, unif_cminmaxmul,
              unif_density, unif_brightness, unif_opacity, unif_origin, unif_cubemin, unif_cubemax;
        float raystep, density,brightness,opacity;
        glm::vec3 cminmaxmul;
        glm::vec3 cubemin, cubemax;
    public:
        Mplt(): Plottable(), data(0, 0, 0, 0, GL_TEXTURE1),
        unif_step(-1), unif_size(-1), unif_raystep(-1),
        unif_density(-1), unif_brightness(-1),unif_opacity(-1),unif_origin(-1),
        unif_cubemin(-1), unif_cubemax(-1),
        raystep(0),density(0.5),brightness(1.0),opacity(0.95) {
            VAO.add_buffer(); //actualy this one wil contain only the vertices of a cube;)
        }
        bool load(aiw::File & S){
            bool res = Mesh<float, 3>::load(S);
            if (!raystep){
                raystep = std::min(std::min(step[0],step[1]),step[2]);
            //reload_texture();
                get_auto_box(*reinterpret_cast<aiw::Vec<3, float>*>(&cubemin), *reinterpret_cast<aiw::Vec<3, float>*>(&cubemax));
            }
            return res;
        }
        void reload_cube(){
            int NTR=12;
            float & xmin = cubemin[0], &xmax = cubemax[0],
                &ymin = cubemin[1], &ymax = cubemax[1],
                &zmin = cubemin[2], &zmax = cubemax[2];
            const glm::vec3 cube[8] = {  glm::vec3(xmin,ymin,zmin), glm::vec3(xmax,ymin,zmin), glm::vec3(xmin,ymax,zmin), glm::vec3(xmin,ymin,zmax),
                glm::vec3(xmax,ymax,zmin), glm::vec3(xmin,ymax,zmax), glm::vec3(xmax,ymin,zmax), glm::vec3(xmax,ymax,zmax)};
            unsigned int indices[36] = { 
                0, 2, 4,
                0, 4, 1,
                0, 3, 6,
                0, 6, 1,
                0, 5, 2,
                0, 3, 5,
                1, 6, 7,
                1, 7, 4,
                2, 5, 7,
                2, 7, 4,
                3, 5, 7,
                3, 7, 6
            };
            //for (int k=0; k<NTR; k++){
            //    printf("%g %g %g \n",   cube[indices[3*k]][0],cube[indices[3*k]][1],cube[indices[3*k]][2]);
            //    printf("%g %g %g \n",   cube[indices[3*k+1]][0],cube[indices[3*k+1]][1],cube[indices[3*k+1]][2]);
            //    printf("%g %g %g \n",   cube[indices[3*k+2]][0],cube[indices[3*k+2]][1],cube[indices[3*k+2]][2]);
            //    printf("%g %g %g \n\n\n", cube[indices[3*k  ]][0],cube[indices[3*k  ]][1],cube[indices[3*k  ]][2]);
            //}
            VAO.load_data(POS, sizeof(glm::vec3) * 8, cube);
            VAO.load_indices(NTR*3*sizeof(unsigned int), indices);
            VAO.release();
        }
        void load_on_device(){
            reload_cube();
            reload_texture();
        }
        void set_density(float dens){
            density = dens;
        }
        float get_density(){
            return density;
        }
        void set_brightness(float bright){
            brightness = bright;
        }
        float get_brightness(){
            return brightness;
        }
        void set_opacity(float op){
            opacity = op;
        }
        float get_opacity(){
            return opacity;
        }
        void set_raystep(float rs){
            raystep = rs;
        }
        float get_raystep(){
            return raystep;
        }
        void reload_texture(){
            auto size = Mesh<float, 3>::bbox();
            data.load(&(*this)[Ind<3>(0)], size[0], size[1], size[2]);
        }
        void get_auto_box(aiw::Vec<3, float> & bb_min, aiw::Vec<3, float> & bb_max){
            bb_min = Mesh<float, 3>::bmin;
            bb_max = Mesh<float, 3>::bmax-step;
        }
        void adjust_cube(const Viewer3D & V){
            get_auto_box(*reinterpret_cast<aiw::Vec<3, float>*>(&cubemin), *reinterpret_cast<aiw::Vec<3, float>*>(&cubemax));
            auto vmin = V.get_vmin();
            auto vmax = V.get_vmax();
            for(int i=0; i<3; i++){
                cubemin[i] = std::max(cubemin[i], vmin[i]);
                cubemax[i] = std::min(cubemax[i], vmax[i]);
            }
            reload_cube();
        }
        void attach_shader(ShaderProg * spr){
            //glBindVertexArray(VAO);
            this->VAO.bind();
            //spr->AttachUniform(unif_bmin,"bmin");
            //spr->AttachUniform(unif_bmax,"bmax");
            spr->AttachUniform(unif_cminmaxmul,"minmaxmul");
            spr->AttachUniform(unif_step,"rstep");
            spr->AttachUniform(unif_size,"size");
            spr->AttachUniform(unif_raystep,"raystep");
            spr->AttachUniform(unif_density,"density");
            spr->AttachUniform(unif_brightness,"brightness");
            spr->AttachUniform(unif_opacity,"opacity");
            spr->AttachUniform(unif_origin,"origin");
            spr->AttachUniform(unif_cubemin,"cubemin");
            spr->AttachUniform(unif_cubemax,"cubemax");
            spr->AttachAttr(this->VAO.get_attr(POS),"coord");
            auto size = bbox();
            //if (unif_bmin != -1) glUniform3f(unif_bmin, bmin[0], bmin[1], bmin[2]);
            //if (unif_bmax != -1) glUniform3f(unif_bmax, bmax[0], bmax[1], bmax[2]);
            if (unif_cminmaxmul != -1) glUniform3f(unif_cminmaxmul, cminmaxmul[0], cminmaxmul[1], cminmaxmul[2]);
            if (unif_step != -1) glUniform3f(unif_step, rstep[0], rstep[1], rstep[2]);
            if (unif_size != -1) glUniform3i(unif_size, size[0], size[1], size[2]);
            if (unif_origin != -1) glUniform3f(unif_origin, Mesh<float, 3>::bmin[0], Mesh<float, 3>::bmin[1], Mesh<float, 3>::bmin[2]);
            if (unif_cubemin != -1) glUniform3f(unif_cubemin, cubemin.x, cubemin.y, cubemin.z);
            if (unif_cubemax != -1) glUniform3f(unif_cubemax, cubemax.x, cubemax.y, cubemax.z);
            if (unif_raystep != -1) glUniform1f(unif_raystep, raystep);
            if (unif_density != -1) glUniform1f(unif_density, density);
            if (unif_brightness != -1) glUniform1f(unif_brightness, brightness);
            if (unif_opacity != -1) glUniform1f(unif_opacity, opacity);
            this->VAO.enable_attr(POS, 3, GL_FLOAT);
            this->VAO.release();
        }
        void plot(ShaderProg * spr){
            attach_shader(spr);
            auto NTR=12;
            data.use_texture(spr,"data");
            VAO.bind();
            //glDrawElements(GL_TRIANGLES, Ntr, GL_UNSIGNED_INT, (void *)0);
            glDrawElementsInstanced(GL_TRIANGLES, NTR*3, GL_UNSIGNED_INT, (void *)0, 1);
            //glBindVertexArray(0);
            VAO.release();
        }
        //------------------------------------------------------------------------------
        void rangemove(float shift,bool l){
            float &min = cminmaxmul[0];
            float &max = cminmaxmul[1];
            float len = max - min;
            if (l) min -= shift * len;
            else max += shift * len;
            cminmaxmul[2] = (min < max)? 1.f/(max  - min): 1.f;
        }
        //------------------------------------------------------------------------------
        void extendrange(float factor){
            float &min = cminmaxmul[0];
            float &max = cminmaxmul[1];
            float &mul = cminmaxmul[2];
            float len_2 = (max - min)*0.5f,
                  center = (min + max)* 0.5f;
            min = center - len_2*factor;
            max = center + len_2*factor;
            mul = (min < max)? 1.f/(max  - min): 1.f;
        }
        //------------------------------------------------------------------------------
        void set_range(float lower, float upper){
            cminmaxmul[0] = lower;
            cminmaxmul[1] = upper;
            cminmaxmul[2] = (lower < upper)? 1.f/(upper  - lower): 1.f;
        }
        //------------------------------------------------------------------------------
        float min(){
            return cminmaxmul[0];
        }
        //------------------------------------------------------------------------------
        float max(){
            return cminmaxmul[1];
        }
        //------------------------------------------------------------------------------
        void autoset_minmax(){
            auto minmax = Mesh<float, 3>::min_max();
            set_range(minmax[0], minmax[1]);
        }

};
#endif // MPLT_HPP
