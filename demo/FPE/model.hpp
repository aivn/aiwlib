#pragma once
#include <vector>
#include <aiwlib/mesh>
using namespace aiw;

class Model{
    std::vector<Vec<2> > tracs; // массив траекторий
public:
    double a, b, A, Omega, gamma, T, t, h; // параметры расчета

	Vec<6> av;  // моменты x, v, xx, vv, xv, W
	// вклад в моменты от одной траектории
	inline Vec<6> d_av(const Vec<2> &p) const { 
		return p|(p&p)|(p[0]*p[1])|
			((.5*a + .25*b*p[0]*p[0])*p[0]*p[0] + .5*p[1]*p[1]);
	}
    Mesh<float, 2> f; // функция распределения
	
    void init(Vec<2> xv0, int N); // инициализация
    void calc(); // расчет одного шага
};
