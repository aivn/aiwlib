#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <sstream>
#include "../../include/aiwlib/fv_interface.hpp"
using namespace aiw;

void help(){
	printf("usage: fv-slice [start_frame]:[step_frame:][end_frame] axe1=value1 [axe2=value2 ...] infile\n");
}

struct Axe{
	int pos;
	bool on_ray;
	float val;
};

int main(int argc, const char **argv){
	// разбор аргументов командной строки
	if(argc<4){ help();	return 1; }
	int frame=0, step_frame=1, end_frame=-1;
	{
		int i1=0; while(argv[1][i1]!=':' && argv[1][i1]) i1++;
		if(i1) frame = atoi(argv[1]);
		else if(argv[1][i1]!=':'){ help(); return 1; }
		if(argv[1][i1]){
			int i2 = ++i1; while(argv[1][i2]!=':' && argv[1][i2]) i2++;
			if(i2!=i1 && argv[1][i2]){ step_frame = atoi(argv[1]+i1); end_frame = atoi(argv[1]+i2+1); }
			else if(i2!=i1) end_frame = atoi(argv[1]+i1);
			else if(argv[1][i2]) end_frame = atoi(argv[1]+i2+1);
		} 
	}
	std::map<std::string, float> axes_table;
	for(int i=2; i<argc-1; i++){
		int p=0; while(argv[i][p]!='=' && argv[i][p]) p++;
		if(argv[i][p]){ std::string name(argv[i], p); axes_table[name] = atof(argv[i]+p+1); }		
		else{ help(); return 1; }
	}
	
	// цикл по фреймам и вывод результатов
	aiw::File fin(argv[argc-1],"r");
	FV_Interface2 fv;
	// загрузка нужного фрейма
	for(int i=0; i<=frame; i++) if(!fv.load(fin)) return 0;
	while(1){
		printf("#:%s\n# time=%g\n", fv.appends_names.c_str(), fv.time);
		
		// анализ осей
		Axe axes[256]; int Naxes=0; bool on_ray = false;
		std::stringstream S(fv.appends_names); int pos=0; std::string n; S>>n;
		while(S){
			if(axes_table.find(n)!=axes_table.end()){
				axes[Naxes].pos = pos;
				axes[Naxes].on_ray = pos>=fv.get_cell_len();
				axes[Naxes].val = axes_table[n];
				if(on_ray && axes[Naxes].on_ray) continue;
				if(axes[Naxes].on_ray){ on_ray = true; axes[Naxes].pos -= fv.get_cell_len(); }
				Naxes++;
			}
			S>>n; pos++;
		}
		
		// цикл по граням
		float vmin=0, vmax=0;
		for(int cID=0; cID<(int)(fv.get_cells_size()); cID++){
			const Ind<3> &tr = fv.get_cell_tr(cID);
			bool skipline = false;
			for(int k=0; k<3; k++){ // цикл по ребрам
				int a = tr[(k+1)%3], b = tr[(k+2)%3];
				bool use = true; float w = .5;
				for(int i=0; i<Naxes; i++){
					if(axes[i].on_ray){
						float ax = fv.get_ray1(a, axes[i].pos), bx = fv.get_ray1(b, axes[i].pos);
						if(vmin==0 || vmin>ax) vmin = ax;
						if(vmin==0 || vmin>bx) vmin = bx;
						if(vmax==0 || vmax<ax) vmax = ax;
						if(vmax==0 || vmax<bx) vmax = bx;
						if((ax<=axes[i].val && axes[i].val<=bx)||(bx<=axes[i].val && axes[i].val<=ax))
							w = fabs(bx-ax)>fabs((bx + ax)*0.5*1e-8)?fabs((axes[i].val-ax)/(bx-ax)):0.5;							
						else{ use = false; break; }
					} else if(axes[i].val != fv.get_cell_app(cID, axes[i].pos)){ use = false; break; }
				}
				if(use){ // вывод значения
					for(int i=0; i<fv.get_cell_len(); i++) printf("%g ", fv.get_cell_app(cID, i));
					for(int i=0; i<fv.get_ray_len(); i++) printf("%g ", fv.get_ray1(a, i)*(1-w)+fv.get_ray1(b, i)*w);
					printf("\n");
					skipline = true;
				}
			}
			if(skipline) printf("\n\n");
		}
		// WOUT(axes[0].pos, vmin, vmax);
		
		// загрузка нужного фрейма, следующий номер фрейма
		for(int i=0; i<step_frame; i++) if(!fv.load(fin)) return 0;
		frame += step_frame; if(end_frame>0 && frame>end_frame) return 0;
	}
	return 0;
}
