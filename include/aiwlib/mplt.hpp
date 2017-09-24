#ifndef MVIEW_INTERFACE_HPP
#define MVIEW_INterFACE_HPP
#include "mview_format.hpp"
#include "../../AbstractViewer/plottable.hpp"
class  MViewInterface: public Plottable, public MViewFormat{
    private:
        //std::vector<glm::vec3> points;
        std::vector<glm::vec3> normals;
        //multiple sellects ?
        std::vector<unsigned int> select;
        GLint unif_radius;
        ~MViewInterface(){};
        // ldisplay lattices parametrs?
    public:
        float radius;
        MViewInterface(): Plottable(), radius(1.){ 
	        VAO.add_buffer();
	        VAO.add_buffer();
        }
        bool load(aiw::File & S){
            if (S.tell()==0){
                MViewFormat::load_head(S);
            }
            normals.resize(MViewFormat::size());
            auto check = MViewFormat::load_frame(reinterpret_cast<aiw::Vec<3, float> *>( & ( normals[0] ) ), S);
            //WOUT(S.tell(), time);
            return check;
        }
        void load_on_device(){
            int Npt=MViewFormat::size();
            //glBindVertexArray(VAO);
            //VAO.bind()
            VAO.load_data(POS,sizeof( aiw::Vec<3, float> ) * Npt,
                    reinterpret_cast<float *>(& (MViewFormat::coords[0]) ));
            //glBindBuffer(GL_ARRAY_BUFFER, VBO[POS]);
            //glBufferData(GL_ARRAY_BUFFER, sizeof( aiw::Vec<3, float> ) * Npt,
            //        reinterpret_cast<float *>(& (MViewFormat::coords[0]) ), GL_STATIC_DRAW);
            reload_normals();
            reload_select();
        }
        void reload_normals(){
            int Npt=MViewFormat::size();
            VAO.load_data(NRM,  sizeof( aiw::Vec<3, float> ) * Npt,
                    reinterpret_cast<float *>(& (normals[0]) ));
            //glBindVertexArray(VAO);
            //glBindBuffer(GL_ARRAY_BUFFER, VBO[NRM]);
            //glBufferData(GL_ARRAY_BUFFER, sizeof( aiw::Vec<3, float> ) * Npt,
            //        reinterpret_cast<float *>(& (normals[0]) ), GL_STATIC_DRAW);
            //glBindVertexArray(0);
        }
        void reload_select(){
            //glBindVertexArray(VAO);
            VAO.load_indices(select.size()*sizeof(unsigned int),
                    reinterpret_cast<unsigned int *>( & (select[0]) ));
            //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[IND]);
            //glBufferData(GL_ELEMENT_ARRAY_BUFFER, select.size()*sizeof(unsigned int),
            //        reinterpret_cast<unsigned int *>( & (select[0]) ), GL_STATIC_DRAW );
            //glBindVertexArray(0);
        }
        void set_select(int * ids, int len){
            select.clear();
            select.resize(len);
            for(unsigned int i =0; i< select.size(); i++) select[i] = ids[i];
            reload_select();
        }
        int lattice_size(int l){
            if (l<0 && l>=MViewFormat::lattices_size()) return 0;
            int result =0;
            for(int i=0;i < MViewFormat::size(); i++){
                int j = get_lattice(i);
                if (j==l) result ++;
            }
            return result;
        }
        void select_lattice(int l){
            select.resize(lattice_size(l));
            int cur = 0;
            for(int i=0;i < MViewFormat::size(); i++){
                int j = get_lattice(i);
                if (j==l){
                    cur ++;
                    select[cur] = static_cast<unsigned int>(i);
                }
            }
            reload_select();
        }
        void select_all(){
            select.resize(MViewFormat::size());
            //select.resize(std::min(MViewFormat::size(),1000000));
            //WOUT(select.size());
            for(unsigned int i=0; i< select.size(); i++){
                select[i] = i;
            }
            reload_select();
        }
        void get_auto_box(aiw::Vec<3, float> & bb_min, aiw::Vec<3, float> & bb_max){
            if (!select.empty()){
                bb_min = MViewFormat::get_coord(select[0]);
                bb_max = MViewFormat::get_coord(select[0]);
                for(auto i:select){
                    auto v = MViewFormat::get_coord(i);
                    bb_min <<= v;
                    bb_max >>= v;
                }
                bb_min += aiw::Vec<3, float>(-radius);
                bb_max += aiw::Vec<3, float>(radius);
                if (bb_min ==bb_max){
                    bb_min += aiw::Vec<3, float>(-1.f);
                    bb_max += aiw::Vec<3, float>(1.f);
                }
            } else{
                bb_min = aiw::Vec<3, float>(-radius);
                bb_max = aiw::Vec<3, float>(radius);
                if (bb_min ==bb_max){
                    bb_min += aiw::Vec<3, float>(-1.f);
                    bb_max += aiw::Vec<3, float>(1.f);
                }
            }
        }
        void attach_shader(ShaderProg * spr) {
            //glBindVertexArray(VAO);
            VAO.bind();
            spr->AttachUniform(unif_radius, "radius");
            spr->AttachAttr(VAO.get_attr(POS), "coord");
            spr->AttachAttr(VAO.get_attr(NRM), "normal");
            VAO.enable_attr(POS,3, GL_FLOAT);
            VAO.enable_attr(NRM, 3, GL_FLOAT);
            //if (vattr != -1){
            //    glEnableVertexAttribArray(vattr);
            //    glBindBuffer(GL_ARRAY_BUFFER, VBO[POS]);
            //    glVertexAttribPointer(vattr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            //}
            //if (nattr != -1){
            //    glEnableVertexAttribArray(nattr);
            //    glBindBuffer(GL_ARRAY_BUFFER, VBO[NRM]);
            //    glVertexAttribPointer(nattr, 3, GL_FLOAT, GL_FALSE, 0, 0);
            //}
            if (unif_radius != -1) glUniform1f(unif_radius, radius);
            //glBindVertexArray(0);
            VAO.release();
        }
        void plot(ShaderProg * spr){
            int Npt = select.size();
            attach_shader(spr);
            //glBindVertexArray(VAO);
            VAO.bind();
            glDrawElementsInstanced(GL_POINTS, Npt, GL_UNSIGNED_INT, (void *)0, 1);
            VAO.release();
            //glBindVertexArray(0);
        }
};
#endif //MVIEW_INTERFACE_HPP
