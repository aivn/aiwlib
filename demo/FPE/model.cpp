#include <aiwlib/gauss> // библиотека со случайным источником
#include "model.hpp"

void Model::init(Vec<2> xv0, int N){
    tracs.resize(N, xv0);
	av = d_av(xv0)|0;
    f.fill(0.f); f[xv0] = 1./f.step.prod();
    rand_init(); // инициализация случайного источника
	t = 0.;
}
void Model::calc(){
    double sgT =  sqrt(2*h*gamma*T), stab = 1./(1.-.5*gamma*h);
	double Fext = A*sin(Omega*(t+.5*h));
	double df = 1./(tracs.size()*f.step.prod());
	
    f.fill(0.f); av = vec(0.); 
	for(Vec<2> &p: tracs){ // цикл по траекториям		
		double old_x = p[0];
		p[0] += p[1]*h*.5; // численная схема
		p[1] += h*(-a*p[0]-b*p[0]*p[0]*p[0]+Fext);
		p[0] += p[1]*h*.5;
		p[1] += stab*(-gamma*p[1]*h+sgT*rand_gauss());

        f[p] += df; av += d_av(p)|bool(old_x*p[0]<0); 	// диагностика		 
	}
	av /= tracs.size(); t += h;
}
