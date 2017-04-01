#ifndef MODEL_HPP
#define MODEL_HPP

#include <aiwlib/magnets/data>
using namespace aiw;
//------------------------------------------------------------------------------
class Model{
	MagnData data;
public:
	Model();
	void init();
	void calc();
	void dump_head();
	void dump_frame();
};
//------------------------------------------------------------------------------
#endif //MODEL_HPP
