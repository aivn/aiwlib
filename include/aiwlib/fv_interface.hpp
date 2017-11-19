#ifndef FV_INTERFACE2
#define FV_INTERFACE2

#include "vec"
#include "iostream"
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
//using namespace aiw;
class FV_Interface2 {
    struct cell_t{
        aiw::Ind<3> tr; int tbID;
        cell_t(): tbID(-1) {}
    };
	std::vector<float> rays;
	std::vector<float> cells_app;
    std::vector<cell_t> cells;
    //size_t str_size;
    int cell_len, ray_len;
    size_t cells_size, rays_size;
public:
    std::string appends_names;
    float time;

    FV_Interface2(): cell_len(0), ray_len(0), cells_size(0),
     rays_size(0){};
    //~FV_Interface2(){};

    // read interface
    bool load(aiw::File& S){
        size_t str_size;
        if(!S.read(&time, sizeof(time))) return false; 
        if(!S.read(&str_size, sizeof(str_size))) return false;
        if(!S.read(&cell_len, sizeof(cell_len))) return false;
        if(!S.read(&ray_len, sizeof(ray_len))) return false;
        if(!S.read(&cells_size, sizeof(cells_size))) return false;
        if(!S.read(&rays_size, sizeof(rays_size))) return false;
        appends_names.resize(str_size);
        rays.resize(rays_size*ray_len); cells.resize(cells_size);
        cells_app.resize(cells_size*cell_len);
        if(!S.read(&(appends_names[0]), sizeof(char)*str_size)) return false;
        if(!S.read(&(cells[0]), sizeof(cell_t)*cells.size()) ) return false;
        if(!S.read(&(cells_app[0]), sizeof(float)*cells_app.size()) ) return false;
        if(!S.read(&(rays[0]), sizeof(float)*rays.size()) ) return false;
        return true;
    }

    int get_cell_len() const { return cell_len; }
    int get_ray_len() const { return ray_len; }
    size_t get_rays_size() const { return rays_size; }
    size_t get_cells_size() const { return cells_size; }

    int get_cell_tbID(int cID) const{
        return cells[cID].tbID;
    };
    const aiw::Ind<3>& get_cell_tr(int cID) const{
        return cells[cID].tr;
    };
    float get_cell_app(int cID, int item) const{
        return cells_app[cID*cell_len + item];
    };
    float get_ray1(int rID, int item) const{
        return rays[rID*ray_len + item];
    };
    aiw::Vec<3, float> get_ray3(int rID, const aiw::Ind<3>& items) const{
        aiw::Vec<3, float> result;
        for(int i=0; i<3; i++) result[i] = get_ray1(rID, items[i]);
        return result;
    };
    const float* get_cells_app() const{
        return reinterpret_cast<const float *>(&(cells_app[0]));
    };
    const float* get_rays() const{
        return reinterpret_cast<const float *>(&(rays[0]));
    };

    // write interface
	void init(int Nrays, int Ncells, int cell_len_, int ray_len_, const char* str = ""){
        cell_len = cell_len_; ray_len = ray_len_;
        cells_size = Ncells; rays_size = Nrays;
        appends_names=std::string(str);
        rays.resize(rays_size*ray_len); cells.resize(cells_size);
        cells_app.resize(cell_len*cells_size);
    }
    void dump(aiw::File &S) const {
        size_t str_size = appends_names.size();
        S.write(&time, sizeof(time));
        S.write(&str_size, sizeof(str_size));
        S.write(&cell_len, sizeof(cell_len));
        S.write(&ray_len, sizeof(ray_len));
        S.write(&cells_size, sizeof(cells_size));
        S.write(&rays_size, sizeof(rays_size));
        S.write(&(appends_names[0]), sizeof(char)*appends_names.size());
        S.write(&(cells[0]), sizeof(cell_t)*cells.size());
        S.write(&(cells_app[0]), sizeof(float)*cells_app.size());
        S.write(&(rays[0]), sizeof(float)*rays.size());
    }
    template <int D> void set_ray(int rID, const aiw::Vec<D, float> &ray){
        if(D!=ray_len) WRAISE( D, ray_len);
        for(int i=0; i<D; i++) rays[D*rID+i] = ray[i];
    }
    template <int D> void set_cell_app(int cID, const aiw::Vec<D, float> &app){
        if(D!=cell_len) WRAISE(D, cell_len);
        for(int i=0; i<D; i++) cells_app[D*cID+i] = app[i];
    }
    void set_cell_tr(int cID, const aiw::Ind<3> &tr){ cells[cID].tr = tr; }
    void set_cell_tbID(int cID, int tbID){ cells[cID].tbID = tbID; }
};
#endif //FV_INTERFACE2
