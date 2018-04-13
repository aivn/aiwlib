#include <aiwlib/gauss>
#include "model.hpp"
using namespace aiw;

void Model::init(Vec<2> xv0, int N){
    tracs.resize(N, xv0);
	av = d_av(xv0);
    f.fill(0.f); f[xv0] = 1./f.step.prod();
    rand_init();
	t = 0.;
}
void Model::calc(){
    double sgT =  sqrt(2*gamma*T), stab = h/(1.-.5*gamma*h);
	double Fext = A*sin(Omega*(t+.5*h)), df = 1./(tracs.size()*f.step.prod());
    f.fill(0.f); av = vec(0.); 
	for(Vec<2> &p: tracs){
		// численная схема
		p[0] += p[1]*h*.5;
		p[1] += h*(-a*p[0]-b*p[0]*p[0]*p[0]+Fext);
		p[0] += p[1]*h*.5;
		p[1] += stab*(-gamma*p[1]+sgT*rand_gauss());

		// диагностика
        f[p] += df; av += d_av(p);		
	}
	av /= tracs.size(); t += h;
}

