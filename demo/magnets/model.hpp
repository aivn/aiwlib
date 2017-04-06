#ifndef MODEL_HPP
#define MODEL_HPP

#include <aiwlib/magnets/data>
#include <aiwlib/sphere>
using namespace aiw;
//------------------------------------------------------------------------------
class Model{
	MagnData data;
public:
	// основные параметры задачи
	double J, alpha, gamma; // обмен, диссипация и прецессия
	aiw::Ind<3> tile_sz;    // размеры tile в ячейках
	aiw::Ind<3> max_tsz;    // максимальный размер области, в тайлах
	int periodic_bc;        // битовая маска периодических граничных условий
	aiw::Vecf<3> nK;        // направление анизотропии (единичный вектор)
	float K;                // величина анизотропии
	float h;                // шаг по времени
	aiw::Vecf<3> Hext;      // внешнее поле	
	
	// диагностика
	double W;                        // полная энергия системы на один атом
	aiw::Vecf<3> Ma, Mb;             // средние намагниченности подрешеток, они же используются как н.у. при инициализации
	aiw::Sphere<float> dfa, dfb, df; // функции распределения намагниченности (по подрешеткам и полная)
	int df_rank=3;                   // ранг ф.р.
	double t;                        // время
	
	// соновные методы
	Model();            
	void init(); // инициализация модели
	void calc_RK4(int Nit); // считать на Nit шагов методом RK4

	// сброс и чтение данных в новом формате
	void dump_head(aiw::IOstream &str);  
	void dump_frame(aiw::IOstream &str); 
	void load_head(aiw::IOstream &str);  
	void load_frame(aiw::IOstream &str); 

	// сброс и чтение данных в старом формате
	void dump_head_spins(aiw::IOstream &str);
	void dump_frame_spins(aiw::IOstream &str);
	void load_head_spins(aiw::IOstream &str); // проверяет соответствует ли записанная геометрия текущей геометрии модели, если нет выкидывает исключение 
	void load_frame_spins(aiw::IOstream &str);
};
//------------------------------------------------------------------------------
#endif //MODEL_HPP
