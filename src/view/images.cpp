/**
 * Copyright (C) 2015, 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <omp.h>
#include "../../include/aiwlib/view/images"
using namespace aiw;

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
void aiw::ImagePIL::load(PyObject *buf){
	const char *src = PyString_AsString(buf);
	//#pragma omp parallel for
	for(int y=0; y<size[1]; y++)
		for(int x=0; x<size[0]; x++){
			char* ptr = im->image[y] + x*im->pixelsize; const char *color = src+(x+y*size[0])*3;
			ptr[0] = color[0]; ptr[1] = color[1]; ptr[2] = color[2];
		}
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