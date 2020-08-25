#ifndef FRONTVIEW_HPP
#define FRONTVIEW_HPP
#include "fv_interface.hpp"
#include "../../AbstractViewer/plottable.hpp"
#include <set>
#define POS 0
#define CLR 1
#define NRM 2
using namespace aiw;
aiw::Vec<3,float> glmtoaiw(glm::vec3 v){
    aiw::Vec<3,float> res(0.f);
    res[0] = static_cast<float>(v.x); res[1] = static_cast<float>(v.y); res[2] = static_cast<float>(v.z);
    return res;
}
//------------------------------------------------------------------------------
class Surface: public FV_Interface2, 
        public SurfTemplate
{
protected:
	//std::vector<glm::vec3> triangles;
	std::vector<glm::vec3> normals;
    //std::vector<unsigned int> select;
	//std::vector<float> appends;
//    GLuint VBO[3];
    aiw::Ind<3> orts;
    glm::vec3 minmaxmul;
    std::set<int> tubes;
    ~Surface(){};
//------------------------------------------------------------------------------
public:
    Surface():FV_Interface2(),SurfTemplate(){
	    orts = ind(0,1,2);
	    VAO.add_buffer();
	    VAO.add_buffer();
	    VAO.add_buffer();
    };
    int get_cells_size() const {
        return FV_Interface2::get_cells_size();
    }
    const int get_cell_appends_len() const{
        return FV_Interface2::get_cell_len();
    }
    const int get_appends_len() const{
        return FV_Interface2::get_cell_len() + FV_Interface2::get_ray_len();
    }
    const float get_time() const{
        return FV_Interface2::time;
    }
	// OpenGL
	//int get_Ntr() const {return FV_Interface<NA>::get_cells_size();};
	const glm::vec3 * get_triangles() {
        int Ntr = get_cells_size();
      //  if ( triangles.size() != FV_Interface<NA>::cells.size() * 3 ){
            triangles.resize(Ntr*3);
            //std::cout<<"triangles size after resize"<<appends.size()<<std::endl;
            for(int i=0; i< Ntr; i++){
                auto tr = FV_Interface2::get_cell_tr(i);
                for(int l=0; l<3; l++){
                    auto cv = FV_Interface2::get_ray3(tr[l], orts);
                    for(int k=0; k<3; k++) triangles[i*3+l][k%3] = cv[k];
                }
            }
        //}
		return  reinterpret_cast<glm::vec3 *>(&(triangles[0]));
	}
    //------------------------------------------------------------------------------
    const glm::vec3 * get_normals() {
        // calls after get_triangles
        // it's a strict rule, no exceptions
        //get_triangles();
     //   if ( normals.size() != FV_Interface<NA>::cells.size() * 3 ){
            int Ntr = get_cells_size();
            normals.resize(Ntr*3);
            for (auto i=0; i< Ntr; i++){
                glm::vec3 n  = glm::triangleNormal(triangles[3*i],triangles[3*i+1],triangles[3*i+2]);
                normals[3*i] = n;
                normals[3*i+1] = n;
                normals[3*i+2] = n;
            }
       // }
		return reinterpret_cast<glm::vec3 *>(&(normals[0]));
    }
    //------------------------------------------------------------------------------
	const float *get_appends(int item) {
        int Ntr = get_cells_size();
		appends.resize(Ntr*3);
        if (item >= 0 && item < FV_Interface2::get_cell_len()) {
            for(int i=0; i < Ntr; i++)
                for(int j=0; j < 3; j++) appends[3*i+j] = FV_Interface2::get_cell_app(i, item);
        } else {
            item -= FV_Interface2::get_cell_len();
            if(item >=0 && item < FV_Interface2::get_ray_len()){
                for(int i=0; i < Ntr; i++) {
                    auto tr = FV_Interface2::get_cell_tr(i);
                    for(int j=0; j < 3; j++) appends[3*i+j] = FV_Interface2::get_ray1(tr[j], item);
                }
            } else {
            for(int i=0; i < Ntr; i++)
                for(int j=0; j < 3; j++) appends[3*i+j] = FV_Interface2::get_cell_tbID(i);
            }
        }
		return reinterpret_cast<float*>( &(appends[0]));
	}
    //------------------------------------------------------------------------------
    void attach_shader(ShaderProg * spr){
        //glBindVertexArray(VAO);
        this->VAO.bind();
        spr->AttachUniform(unif_minmax,"minmaxmul");
        spr->AttachAttr(this->VAO.get_attr(POS),"coord");
        spr->AttachAttr(this->VAO.get_attr(NRM),"normal");
        spr->AttachAttr(this->VAO.get_attr(CLR),"color");
        if (unif_minmax != -1) glUniform3f(unif_minmax,minmaxmul[0], minmaxmul[1], minmaxmul[2]);
        this->VAO.enable_attr(CLR, 1, GL_FLOAT);
        this->VAO.enable_attr(NRM, 3, GL_FLOAT);
        this->VAO.enable_attr(POS, 3, GL_FLOAT);
        //if (cattr != -1) {
        //    glEnableVertexAttribArray(cattr);
        //    glBindBuffer(GL_ARRAY_BUFFER, VBO[CLR]);
        //    glVertexAttribPointer(cattr, 1, GL_FLOAT, GL_FALSE, 0, 0);
        //}
        //if (nattr != -1) {
        //    glEnableVertexAttribArray(nattr);
        //    glBindBuffer(GL_ARRAY_BUFFER, VBO[NRM]);
        //    glVertexAttribPointer(nattr, 3, GL_FLOAT, GL_FALSE, 0, 0);
        //}
        //if (vattr != -1) {
        //    glEnableVertexAttribArray(vattr);
        //    glBindBuffer(GL_ARRAY_BUFFER, VBO[POS]);
        //    glVertexAttribPointer(vattr, 3, GL_FLOAT, GL_FALSE, 0, 0);
        //}
        //glBindVertexArray(0);
        this->VAO.release();
    }
    //void attach_index_shader(GLint vattr, GLint nattr, GLint cattr, GLint unif_minmax) const{
    //    glBindVertexArray(VAO);
    //    glUniform3f(unif_minmax,minmaxmul[0], minmaxmul[1], minmaxmul[2]);
    //    glEnableVertexAttribArray(cattr);
    //    glBindBuffer(GL_ARRAY_BUFFER, VBO[CLR]);
    //    //glBindBuffer(GL_ARRAY_BUFFER,
    //    glVertexAttribPointer(cattr, 1, GL_UINT, GL_FALSE, 0, 0);
    //    glEnableVertexAttribArray(nattr);
    //    glBindBuffer(GL_ARRAY_BUFFER, VBO[NRM]);
    //    glVertexAttribPointer(nattr, 3, GL_FLOAT, GL_FALSE, 0, 0);
    //    glEnableVertexAttribArray(vattr);
    //    glBindBuffer(GL_ARRAY_BUFFER, VBO[POS]);
    //    glVertexAttribPointer(vattr, 3, GL_FLOAT, GL_FALSE, 0, 0);
    //    glBindVertexArray(0);
    //}
    //------------------------------------------------------------------------------
    void refill_select(){
        if (SurfTemplate::auto_select or tubes.empty() ) SurfTemplate::refill_select();
        else {
            SurfTemplate::select.clear();
            SurfTemplate::select.reserve(tubes.size());
            //int cid =0;
            for(auto cid=0; cid < get_cells_size(); cid++ ){
                if (tubes.find(FV_Interface2::get_cell_tbID(cid) ) != tubes.end() ) {
                    SurfTemplate::select.push_back(cid*3);
                    SurfTemplate::select.push_back(cid*3+1);
                    SurfTemplate::select.push_back(cid*3+2);
                }
            }
        }
    }
    //------------------------------------------------------------------------------
    bool get_auto_select(){
        return SurfTemplate::auto_select;
    }
    void set_auto_select(bool _asel){
        SurfTemplate::auto_select=_asel;
    }
    int get_cell_tbID(int cid){
	    return FV_Interface2::get_cell_tbID(cid);
	}
    //------------------------------------------------------------------------------
    void set_tubes(int * ids, int len){
        tubes.clear();
        add_tubes(ids, len);
    }
    void add_tubes(int *ids, int len){
        tubes.insert<int*>(ids, ids+len);
        reload_tubes();
    }
    void remove_tubes(int *ids, int len){
        for(int i=0; i<len; i++) tubes.erase(ids[i]);
        reload_tubes();
    }
    //------------------------------------------------------------------------------
    void load_on_device(int item){
        int NTR = get_cells_size();
        //glGenVertexArrays(1, &VAO);
        //glBindVertexArray(VAO);
        //glGenBuffers(4,&VBO[0]);
        const glm::vec3 * tri = get_triangles();
        const glm::vec3 * norm = get_normals();
        //VAO.bind();
        VAO.load_data(POS, sizeof(glm::vec3) * NTR*3, tri);
        VAO.load_data(NRM, sizeof(glm::vec3) * NTR*3, norm);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO[POS]);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * NTR*3, tri, GL_STATIC_DRAW);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO[NRM]);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * NTR*3, norm, GL_STATIC_DRAW);
        reload_appends(item);
        reload_tubes();
        //glBindVertexArray(0);
    }
    //------------------------------------------------------------------------------
    void reload_tubes(){
        //glBindVertexArray(VAO);
        refill_select();
        const unsigned int * indices = &select[0];
        VAO.load_indices(select.size()*sizeof(unsigned int), indices );
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[IND]);
        //glBufferData(GL_ELEMENT_ARRAY_BUFFER, select.size()*sizeof(unsigned int), indices, GL_STATIC_DRAW );
        //glBindVertexArray(0);
    }
    //------------------------------------------------------------------------------
    void reload_appends(int item){
        int NTR = get_cells_size();
        //glBindBuffer(GL_ARRAY_BUFFER, VBO[CLR]);
        //if(ia >=0 && ia < NA){
        const float * ap = get_appends(item);
        VAO.load_data(CLR, sizeof(float) * NTR*3, ap);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(float) * NTR*3, ap, GL_STATIC_DRAW);
        /*} else {
            float * ap = new float [NTR*3];
            for(int i=0; i< NTR*3; i++)ap[i] = 0.f;
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * NTR*3, ap, GL_STATIC_DRAW);
            delete [] ap;
        }*/
    }
    //------------------------------------------------------------------------------
    float min(){
        return minmaxmul[0];
    }
    //------------------------------------------------------------------------------
    float max(){
        return minmaxmul[1];
    }
    //------------------------------------------------------------------------------
    void set_orts(const aiw::Ind<3> _orts){
        orts = _orts - Ind<3>(get_cell_len());
    }
    const aiw::Ind<3> get_orts() const{
        return orts+ Ind<3>(get_cell_len());
    }
    //------------------------------------------------------------------------------
    void autoset_minmax(){
        //int Ntr = get_cells_size();
        //const float * app = reinterpret_cast<const float *>(&appends[0]);
        //std::cout<<Ntr<<std::endl;
        //std::cout<<appends.size()<<std::endl;
        //select?
        if (!select.empty()) {
            minmaxmul=glm::vec3(appends[select[0]], appends[select[0]],0.);
        }else {
            minmaxmul=glm::vec3(0.f,1.f,1.f);
            return;
        }
        float &min = minmaxmul[0];
        float &max = minmaxmul[1];
        float &mul = minmaxmul[2];
        //std::cout<<min<< " " << max<<std::endl;
        for (auto &&i : select){
            //std::cout<<a<<std::endl;
            min = (min<= appends[i])?min:appends[i];
            max = (max>= appends[i])?max:appends[i];
        }
        mul = (max!=min)?1.f / (max - min) : 1.f ;
        if (max==min) minmaxmul=glm::vec3(min-.5f,min+.5f,1.f);
        //std::cout<<min<< " " << max<<std::endl;
    }
    //------------------------------------------------------------------------------
    void get_auto_box(aiw::Vec<3,float> & bb_min, aiw::Vec<3,float> & bb_max ){
        //int Ntr = get_cells_size();
        //const float * app = reinterpret_cast<const float *>(&appends[0]);
        //std::cout<<Ntr<<std::endl;
        //std::cout<<appends.size()<<std::endl;
        //select?
        if (! select.empty()){
            bb_min = glmtoaiw(triangles[select[0]]);
            bb_max = glmtoaiw(triangles[select[0]]);
        }else {
            bb_min = Vec<3,float>(-1.f);
            bb_max = Vec<3,float>(1.f);
            return;
        }
        for (auto && i: select){
            //std::cout<<a<<std::endl;
            bb_min <<= glmtoaiw(triangles[i]);
            bb_max >>= glmtoaiw(triangles[i]);
        }
    }
    void params_copy(class Surface & other){
        this->minmaxmul = other.minmaxmul;
        // select ?
        this->orts = other.get_orts();
        this->tubes = other.tubes;
    }
    //------------------------------------------------------------------------------
    void rangemove(float shift,bool l){
        float &min = minmaxmul[0];
        float &max = minmaxmul[1];
        float len = max - min;
        if (l) min -= shift * len;
        else max += shift * len;
        minmaxmul[2] = (min < max)? 1.f/(max  - min): 1.f;
    }
    //------------------------------------------------------------------------------
    void extendrange(float factor){
        float &min = minmaxmul[0];
        float &max = minmaxmul[1];
        float &mul = minmaxmul[2];
        float len_2 = (max - min)*0.5f,
              center = (min + max)* 0.5f;
        min = center - len_2*factor;
        max = center + len_2*factor;
        mul = (min < max)? 1.f/(max  - min): 1.f;
    }
    //------------------------------------------------------------------------------
    void set_range(float lower, float upper){
        minmaxmul[0] = lower;
        minmaxmul[1] = upper;
        minmaxmul[2] = (lower < upper)? 1.f/(upper  - lower): 1.f;
    }
    //------------------------------------------------------------------------------
    void slice(float angle, float x , float y, aiw::File && outp, int ind=0 ) const{
        // уравнение плоскости
        // cos(ang)*x -sin(ang)*y =0
        // v1*a + v2*(1-a)
        // [x] *cos(ang) =[y]* sin(ang)
        // a*((v1[0]-v2[0])*cos(ang) -(v1[1]-v2[1])*sin(ang)) = -v2[0]*cos(ang) + v2[0]*sin(ang)
        glm::vec3 pv(x,y,0.f);
        float cosa = cos(angle), sina = sin(angle);
        for(unsigned int i = 0; i< triangles.size(); i+=3){
            bool l = false;
            for(int j =0; j<3; j++){
                glm::vec3 v1= triangles[i+((j+1)%3)];
                glm::vec3 v2= triangles[i+((j+2)%3)];
                v1-=pv; v2-=pv;
                //printf("%.2f %.2f\n",v1[0], v1[1]);
                if ((v1[0]*cosa- v1[1]*sina) * (v2[0]*cosa- v2[1]*sina)<=0.f ){
                    //const auto & c = FV_Interface2::get_cells[i/3];
                    auto cind = FV_Interface2::get_cell_app(i/3, 0);
                    auto cJ = FV_Interface2::get_cell_app(i/3, 1);
                    if (cind != (float) ind) continue;
                    float a = (-v2[0] *cosa + v2[1]*sina) /
                        ((v1[0]-v2[0])*cosa - (v1[1]-v2[1]) *sina);
                    auto v = a*v1+(1.f-a)*v2;
                    if (fabs(v[0]*cosa - v[1]*sina)>1e-4) WOUT(v[0]*cosa - v[1]*sina);
                    l = true;
                    outp.printf("%.4f %.4f %g %.0f\n", -v[0]*sina + v[1]*cosa ,v[2], cJ, cind);
                }
            }
            if (l) outp.printf("\n");
        }
    }
};
//------------------------------------------------------------------------------
#endif //FRONTVIEW_HPP
