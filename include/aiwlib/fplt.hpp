#ifndef MPLT_HPP
#define MPLT_HPP
#include "mesh"
using namespace aiw;
class Mplt:public Plottable, public Mesh<float, 3>{
    protected:
        Texture3D data;
        GLint unif_bmin, unif_bmax, unif_step, unif_size,
              unif_raystep, unif_cminmaxmul;
        float raystep;
        glm::vec3 cminmaxmul;
    public:
        Mplt(): Plottable(), data(0, 0, 0, 0, GL_TEXTURE1), unif_bmin(-1), unif_bmax(-1), unif_step(-1), unif_size(-1), unif_raystep(-1),raystep(0) {
            VAO.add_buffer(); //actualy this one wil contain only the vertices of a cube;)
        }
        bool load(aiw::File & S){
            bool res = Mesh<float, 3>::load(S);
            if (!raystep) raystep = std::min(std::min(step[0],step[1]),step[2]);
            reload_texture();
            return res;
        }
        void load_on_device(){
            int NTR=6;
            double xmin = Mesh<float, 3>::bmin[0], xmax = Mesh<float, 3>::bmax[0],
                ymin = Mesh<float, 3>::bmin[2], ymax = Mesh<float, 3>::bmax[2],
                zmin = Mesh<float, 3>::bmin[3], zmax = Mesh<float, 3>::bmax[3];
            const glm::vec3 cube[8] = {  glm::vec3(xmin,ymin,zmin), glm::vec3(xmax,ymin,zmin), glm::vec3(xmin,ymax,zmin), glm::vec3(xmin,ymin,zmax),
                glm::vec3(xmax,ymax,zmin), glm::vec3(xmin,ymax,zmax), glm::vec3(xmax,ymin,zmax), glm::vec3(xmax,ymax,zmax)};
            unsigned int indices[24] = { 
                0, 2, 4, 1,
                0, 3, 6, 1,
                0, 3, 5, 2,
                1, 6, 7, 4,
                2, 5, 7, 4,
                3, 5, 7, 6
            };
            VAO.load_data(POS, sizeof(glm::vec3) * NTR*4, cube);
            VAO.load_indices(6*4*sizeof(unsigned int), indices);
            VAO.release();
            reload_texture();
        }
        void reload_texture(){
            auto size = Mesh<float, 3>::bbox();
            data.load(&(*this)[Ind<3>(0)], size[0], size[1], size[2]);
        }
        void get_auto_box(aiw::Vec<3, float> & bb_min, aiw::Vec<3, float> & bb_max){
            bb_min = Mesh<float, 3>::bmin;
            bb_max = Mesh<float, 3>::bmax;
        }
        void attach_shader(ShaderProg * spr){
            //glBindVertexArray(VAO);
            this->VAO.bind();
            spr->AttachUniform(unif_bmin,"bmin");
            spr->AttachUniform(unif_bmax,"bmax");
            spr->AttachUniform(unif_step,"step");
            spr->AttachUniform(unif_size,"size");
            spr->AttachUniform(unif_raystep,"raystep");
            spr->AttachAttr(this->VAO.get_attr(POS),"coord");
            auto size = bbox();
            if (unif_bmin != -1) glUniform3f(unif_bmin,bmin[0], bmin[1], bmin[2]);
            if (unif_bmax != -1) glUniform3f(unif_bmax,bmax[0], bmax[1], bmax[2]);
            if (unif_step != -1) glUniform3f(unif_step,step[0], step[1], step[2]);
            if (unif_size != -1) glUniform3i(unif_size,size[0], size[1], size[2]);
            if (unif_raystep != -1) glUniform1f(unif_raystep,raystep);
            this->VAO.enable_attr(POS, 3, GL_FLOAT);
            this->VAO.release();
        }
        void plot(ShaderProg * spr){
            auto NTR=6;
            data.use_texture(spr,"data");
            VAO.bind();
            //glDrawElements(GL_TRIANGLES, Ntr, GL_UNSIGNED_INT, (void *)0);
            glDrawElementsInstanced(GL_QUADS, NTR, GL_UNSIGNED_INT, (void *)0, 1);
            //glBindVertexArray(0);
            VAO.release();
        }
};
#endif // MPLT_HPP
