/**
 * Copyright (C) 2020 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

// #include "../../include/aiwlib/packer"
#include "../../include/aiwlib/view/mesh"
#include "../../include/aiwlib/view/umesh3D"
#include "../../include/aiwlib/view/zcube"
#include "../../include/aiwlib/view/amr"
#include "../../include/aiwlib/view/sphere"
using namespace aiw;

std::ostream& operator < (std::ostream& S, int32_t  x){ S.write((char*)&x, 4); return S; } 
std::istream& operator > (std::istream& S, int32_t &x){
	S.read((char*)&x, 4);
	// std::cerr<<"uplt int>> "<<x<<'\n';
	return S;
} 
std::ostream& operator < (std::ostream& S, double  x){ S.write((char*)&x, 8); return S; } 
std::istream& operator > (std::istream& S, double &x){  S.read((char*)&x, 8); return S; } 
std::ostream& operator < (std::ostream& S, const std::string& x){ if(S<int32_t(x.size())){ S.write(x.c_str(), x.size()); } return S; } 
std::istream& operator > (std::istream& S, std::string& x){ int32_t sz = 0; if(S>sz){ x.resize(sz); if(sz) S.read(&(x[0]), sz);	} return S; } 


int main(){
	std::vector<std::vector<BaseView*> > data;
	std::vector<UnorderedMesh3DHead*> umeshes;
	while(1){
		char A = std::cin.get(); 
		if(A=='o'){  // открытие файла
			std::string fname; std::cin>fname; std::vector<BaseView*> frames;
			// std::cerr<<"uplte-remote fname="<<fname<<'\n';
			FILE* pf = fopen(fname.c_str(), "r");
			if(!pf){ (std::cout<0).flush(); continue; }
			File fin(pf);
			if(fname.substr(fname.size()-4)==".sgy"){
				MeshView *msh = new MeshView; msh->load_from_segy(fin);
				data.emplace_back(); data.back().push_back(msh);
				continue;
			}
			UnorderedMesh3DHead *umsh = new UnorderedMesh3DHead;
			std::vector<BaseView*> res;
			if(umsh->load(fin)){				
				while(1){
					UnorderedMesh3DView *umsd = new UnorderedMesh3DView;
					if(umsd->load(fin, *umsh)) res.push_back(umsd);
					else{ delete umsd; break; }
				}
				if(res.size()){ umeshes.push_back(umsh); data.push_back(res); }
				else { delete umsh; (std::cout<0).flush();  }
				continue;
			} else delete umsh;
			
			while(1){
				MeshView *msh = new MeshView; if(msh->load(fin)){ res.push_back(msh); continue; } else delete msh;
				SphereView *sph = new SphereView; if(sph->load(fin)){ res.push_back(sph); continue; } else delete sph;
				AdaptiveMeshView *amr = new AdaptiveMeshView; if(amr->load(fin)){ res.push_back(amr); continue; } else delete amr;
				ZCubeView *zcb = new ZCubeView; if(zcb->load(fin)){ res.push_back(zcb); continue; } else delete zcb;
				break;
			}
			std::cout<int32_t(res.size());
			for(auto msh: res)	std::cout<msh->dim()<msh->head;
			if(res.size()) data.push_back(res);
			std::cout.flush();
		} else if(A=='c'){  // получение конфигфайла
			int dID = 0, fID = 0, fc = 0; std::string buf; ConfView conf; std::cin>dID>fID>buf>fc; conf.unpack(buf);
			data[dID][fID]->get_conf(conf, fc); buf = conf.pack(); (std::cout<buf).flush();
		} else if(A=='g'){  // get(conf, r)
			int dID = 0, fID = 0; Vec<2> r; std::string buf; ConfView conf; std::cin>dID>fID>buf>r[0]>r[1]; conf.unpack(buf);
			std::cout<double(data[dID][fID]->get(conf, r)); std::cout.flush();
				// (std::cout<double(data[dID][fID]->get(conf, r))).flush();
		} else if(A=='f'){  // получение пределов
			int dID = 0, fID = 0; std::string buf; ConfView conf; std::cin>dID>fID>buf; conf.unpack(buf);
			Vec<2> ff = data[dID][fID]->f_min_max(conf); (std::cout<ff[0]<ff[1]).flush();
		} else if(A=='p'){  // preview
			int dID = 0, fID = 0; std::string buf; ConfView conf; CalcColor color; std::cin>dID>fID>buf; conf.unpack(buf); std::cin>buf; color.unpack(buf);
			Ind<2> im_sz; std::cin>im_sz[0]>im_sz[1]; ImageBUF image(im_sz);  data[dID][fID]->preview(conf, image, color);
			std::cout.write(image.buf.data(), image.buf.size()); std::cout.flush();
			// std::cerr<<"uplt-remote preview "<<dID<<' '<<fID<<' '<<im_sz<<" OK\n";
		} else if(A=='P'){  // plot
			int dID = 0, fID = 0; std::string buf; ConfView conf; CalcColor color; std::cin>dID>fID>buf; conf.unpack(buf); std::cin>buf; color.unpack(buf);
			Ind<2> im_sz; std::cin>im_sz[0]>im_sz[1]; ImageBUF image(im_sz);  data[dID][fID]->plot(conf, image, color);
			std::cout.write(image.buf.data(), image.buf.size()); std::cout.flush();
			// std::cerr<<"uplt-remote plot "<<dID<<' '<<fID<<' '<<im_sz<<" OK\n";
		} else break; // { std::cerr<<"uplt-remote error: incorrect command "<<A<<std::endl; break; }
	}
}
