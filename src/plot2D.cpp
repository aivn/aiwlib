/**
 * Copyright (C) 2015, 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../include/aiwlib/plot2D"
using namespace aiw;

//------------------------------------------------------------------------------
//   IMAGES
//------------------------------------------------------------------------------
void aiw::CalcColor::init(float const *pal_, float min_, float max_){
	pal = pal_;
	for(len_pal=0; pal[len_pal]>=0.; ++len_pal){}
	len_pal /= 3; len_pal--; // ???
	if(logscale && min_<=0) min_ = 1e-16;
	min = min_; mul = logscale? 1./log(max_/min_) : 1./(max_-min_);
}
//------------------------------------------------------------------------------
Ind<3> *aiw::CalcColor::magn_pal = nullptr;
void aiw::magn_pal_init(int max_rgb){
	if(aiw::CalcColor::magn_pal) return;
	sph_init_table(5);
	aiw::CalcColor::magn_pal = new Ind<3>[sph_cells_num(5)];
	for(size_t i=0; i<sph_cells_num(5); ++i){
		const Vec<3> &n = sph_cell(i, 5); Ind<3> &c = aiw::CalcColor::magn_pal[i]; // 1,0,0, 1,.5,0,   1,1,0, 0,1,0,   0,1,1, 0,0,1,   1,0,1,
		//       yellow
		//       (1,1,0)
		//  green       red
		// (0,1,0)    (1,0,0)
		//        blue
		//       (0,0,1)
		float phi = atan2f(n[1], n[0])*M_2_PI;
		/*
	  float S, V; if(n[2]<0){ S = 1.; V = 1+n[2]; } else { S = 1-n[2]; V = 1.; }
	  float Vmin = (1-S)*V, H = phi-floor(phi);
	  float a = (V-Vmin)*H;
				float Vinc = Vmin+a, Vdec = V-a;
				
				if(phi<-1) c = vec(Vmin, Vdec, Vinc); // (0,1,0) ==> (0,0,1) зеленый ==> синий
				else if(phi<0) c = vec(Vinc, Vmin, Vdec); // (0,0,1) ==> (1,0,0) синий ==> красный
				else if(phi<1)  c = vec(V, Vinc, Vmin); // (1,0,0) ==> (1,1,0) красный ==> желтый
				else c = vec(Vdec, V, Vmin); // (1,1,0) ==> (0,1,0) желтый ==> зеленый
				*/
				
		if(phi<-1){  c[1] = pow(-1-phi,1.2)*max_rgb; c[2] = max_rgb-c[1]; } // (0,1,0) ==> (0,0,1) зеленый ==> синий
		else if(phi<0){ c[2] = pow(-phi,.8)*max_rgb; c[0] = max_rgb-c[2]; } // (0,0,1) ==> (1,0,0) синий ==> красный
		else if(phi<1){ c[0] = max_rgb;  c[1] = phi*max_rgb;         } // (1,0,0) ==> (1,1,0) красный ==> желтый
		else{  c[1] = max_rgb;  c[0] = pow(2-phi, .4)*max_rgb; } // (1,1,0) ==> (0,1,0) желтый ==> зеленый
		
				
		// if(n[2]<0) c *= sqrt(1+n[2]); else c += (ind(max_rgb)-c)*n[2]*n[2]*n[2];
		if(n[2]<0) c *= 1+.85*n[2]; else c += (ind(max_rgb)-c)*n[2]*n[2]*.9;
		// return c; // *max_rgb; //>>ind(0)<<ind(max_rgb);
	}
}
//------------------------------------------------------------------------------
#ifndef AIW_NO_PIL
void aiw::ImagePIL::construct(PyObject* p){
	if(!PyObject_HasAttrString( p, "getim")) 
		throw PyErr_Format(PyExc_AttributeError, "'%s' object at <%zx> has no attribute 'getim'", 
						   PyString_AsString(PyObject_GetAttrString(PyObject_GetAttrString(p, "__class__"), "__name__")), 
						   (size_t)p); 
	im = (ImagingMemoryInstance*)PyCObject_AsVoidPtr(PyObject_CallMethod(p, (char*)"getim", NULL));
	size = ind(im->xsize, im->ysize); pil_ptr = p;
}
aiw::ImagePIL::ImagePIL(PyObject* p){ construct(p); }	

aiw::ImagePIL::ImagePIL(Ind<2> size_){
	construct(PyObject_CallMethodObjArgs(PyImport_ImportModule("PIL.Image"),   // return zero pointer ???
										 PyString_FromString("new"), PyString_FromString("RGB"), 
										 PyTuple_Pack(2, PyInt_FromLong(size_[0]), PyInt_FromLong(size_[1])), NULL)); 										 
}

#endif // AIW_NO_PIL
//------------------------------------------------------------------------------
#ifndef AIW_NO_PNG
void aiw::ImagePNG::init(const char* fname, Ind<2> size_){
	size = size_;
	fp = fopen(fname, "wb"); 	//Open file for writing
	if(!fp) WRAISE("[Errno 2] No such file or directory: ", fname); 

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);   //Initialize write structure
	if(!png_ptr) WRAISE("failed allocate_write_struct: ", fname);
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) WRAISE("failed allocate_info_struct: ", fname);
	if(setjmp(png_jmpbuf(png_ptr))) WRAISE("failed png_creation: ", fname);
	png_init_io(png_ptr, fp);
	//Write header 
	png_set_IHDR(png_ptr, info_ptr, size[0], size[1], 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_text title_text; //set title
	title_text.compression = PNG_TEXT_COMPRESSION_NONE;
	title_text.key = (char*)"Title";
	title_text.text =(char*) fname;
	png_set_text(png_ptr, info_ptr, &title_text, 1);
	png_write_info(png_ptr, info_ptr);
	
	rows = new png_bytep[size[1]];
	for(int y=0; y<size[1]; y++){
		rows[y] = (png_bytep) malloc(3*size[0]*sizeof(png_byte)); //allocate memory for one row
		if(!rows[y]) WRAISE("failed malloc row: ", y, fname);
	}
}
aiw::ImagePNG::~ImagePNG(){
	png_write_image(png_ptr, rows);
	png_write_end(png_ptr, NULL); //write end
	if(png_ptr && info_ptr) png_destroy_write_struct(&(png_ptr), &(info_ptr));
	else{
		if(info_ptr) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
		if(png_ptr)  png_destroy_write_struct(&(png_ptr), (png_infopp)NULL);
	}
	for(int y=0; y<size[1]; y++) delete [] rows[y];
	free(rows);
	if(fp) fclose(fp);
}
#endif // AIW_NO_PNG
//------------------------------------------------------------------------------
