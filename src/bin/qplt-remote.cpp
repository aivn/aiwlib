#include <glob.h>
#include <map>
#include "../../include/aiwlib/binaryio"
#include "../../include/aiwlib/qplt/base"
using namespace aiw;

std::vector<std::vector<QpltContainer*> > containers;
std::map<int, QpltPlotter*> plotters;
int last_plotter_ID = 0;

int main(){ // передавать при запуске размер памяти?
	qplt_global_init(); 
	
	StdOstream stdOut; StdIstream  stdIn;  // File flog("qplt.log", "w");
	while(1){
		char A = std::cin.get(); 
		if(A=='o'){  // открытие файла
			std::string fname;  stdIn.load(fname); //<<< вайлдкарты bash должны быть где то тут
			glob_t glbuf; glob(fname.c_str(), 0, nullptr, &glbuf); stdOut.dump(int(glbuf.gl_pathc)); 
			for(size_t I=0; I<glbuf.gl_pathc; I++){
				// flog("open %\n", fname);
				auto res = factory(glbuf.gl_pathv[I]);
				// flog("% frames\n", int(res.size()));
				if(res.size()) containers.push_back(res);
				stdOut.dump(int(containers.size())-1, int(res.size()));  // <== число фреймов
				// flog("% total files\n", int(containers.size()));
				for(auto f: res){ // <== передаем фреймы
					stdOut.dump(std::string(f->fname()), f->dim, f->szT, f->head, f->info, f->bbox, f->bmin, f->bmax, f->logscale, f->step);
					for(int i=0; i<f->dim; i++) stdOut.dump(f->anames[i]);
				}
			}
			// flog("OK\n");
			globfree(&glbuf);
			std::cout.flush();
		} else if(A=='p'){ // создаем плоттер
			int fileID, frameID, mode, f_opt, arr_lw[2], arr_spacing, nan_color, ctype, Din, mask, offset[3], diff, vconv, minus, axisID[3], faai, D3deep;
			float f_lim[2], sposf[6], bmin_[6], bmax_[6], th_phi[2], cell_aspect[3];
			std::string paletter;
			stdIn.load(fileID, frameID, mode, f_opt, f_lim,  paletter, arr_lw, arr_spacing,  nan_color,
					   ctype, Din, mask, offset, diff, vconv, minus, axisID, sposf, bmin_, bmax_, faai,
					   th_phi, cell_aspect, D3deep);
			QpltPlotter* plt = containers[fileID][frameID]->plotter(mode, f_opt, f_lim, paletter.c_str(), arr_lw, arr_spacing, nan_color, 
																	ctype, Din, mask, offset, diff, vconv, minus,
																	axisID, sposf, bmin_, bmax_, faai, th_phi, cell_aspect, D3deep);
			plotters[last_plotter_ID] = plt;
			stdOut.dump(last_plotter_ID, plt->get_dim(), plt->bbox, plt->bmin, plt->bmax, plt->get_f_min(), plt->get_f_max(), plt->flats_sz());
			for(int i=0; i<plt->flats_sz(); i++){ auto f = plt->get_flat(i); stdOut.dump(f.axis, f.bounds, f.bmin, f.bmax); }
			last_plotter_ID++; std::cout.flush();
		} else if(A=='q'){ int pID; stdIn.load(pID); delete plotters[pID]; plotters.erase(pID);  // удаляем плоттер
		} else if(A=='s'){ // устанавливаем размер изображения
			int pID, xy1[2], xy2[2]; stdIn.load(pID, xy1, xy2);
			QpltPlotter* plt = plotters[pID]; plt->set_image_size(xy1, xy2);
			stdOut.dump(plt->center, plt->ibmin, plt->ibmax);
			for(int i=0; i<plt->flats_sz(); i++){ auto f = plt->get_flat(i); stdOut.dump(f.a, f.b, f.c, f.d, f.nX, f.nY); }
			std::cout.flush();
		} else if(A=='P'){ int pID; stdIn.load(pID); std::string im = plotters[pID]->plot(); stdOut.write(im.c_str(), im.size()); std::cout.flush(); } // отрисовка
		else if(A=='g'){ int pID, xy[2]; stdIn.load(pID, xy); stdOut.dump(plotters[pID]->get(xy)); std::cout.flush(); } // значение в точке
		else if(A=='m'){ float m; stdIn.load(m); QpltContainer::mem_limit = m; WERR(QpltContainer::mem_limit); }  // устанавливаем лимит памяти
		else break; //if(A=='E') break;
	}
	return 0;
}
