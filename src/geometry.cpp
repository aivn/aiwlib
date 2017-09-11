/**
 * Copyright (C) 2016 Sergey Zhdanov, Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/
#include "../include/aiwlib/gauss"
#include "../include/aiwlib/geometry"
using namespace aiw;
//------------------------------------------------------------------------------
Vecf<3> trans_vec (const Vecf<3> &r, const Vecf<3> &center, const Vecf<3> &ox, const Vecf<3> &oy, const Vecf<3> &oz){
    if(fabs(ox * (oy%oz)) <1e-16) WRAISE("Transformation is incorrect", center, ox, oy, oz);
    double x = r[0]-center[0], y = r[1]-center[1], z = r[2]-center[2];
    double x1 = ox[0], x2 = oy[0], x3 = oz[0];
    double y1 = ox[1], y2 = oy[1], y3 = oz[1];
    double z1 = ox[2], z2 = oy[2], z3 = oz[2];
    return vecf( ((x3*y2 - x2*y3)*z - (x3*y - x*y3)*z2 + (x2*y - x*y2)*z3),
                -((x3*y1 - x1*y3)*z - (x3*y - x*y3)*z1 + (x1*y - x*y1)*z3),
                 ((x2*y1 - x1*y2)*z - (x2*y - x*y2)*z1 + (x1*y - x*y1)*z2) ) / ((x3*y2 - x2*y3)*z1 - (x3*y1 - x1*y3)*z2 + (x2*y1 - x1*y2)*z3) + center;
}
//------------------------------------------------------------------------------
Vecf<3> trans_vec_back(const Vecf<3> &r, const Vecf<3> &center, const Vecf<3> &ox, const Vecf<3> &oy, const Vecf<3> &oz){
    if (fabs(ox * (oy%oz)) <1e-16) WRAISE("Transformation is incorrect", center, ox, oy, oz);
    double x = r[0]-center[0], y = r[1]-center[1], z = r[2]-center[2];
    double x1 = ox[0], x2 = oy[0], x3 = oz[0];
    double y1 = ox[1], y2 = oy[1], y3 = oz[1];
    double z1 = ox[2], z2 = oy[2], z3 = oz[2];

    return vecf(x*x1 + x2*y + x3*z,
                x*y1 + y*y2 + y3*z,
                x*z1 + y*z2 + z*z3) + center;
}
//------------------------------------------------------------------------------
//   rotate
//------------------------------------------------------------------------------
/* from Zhdanov
struct RotatedFigure: public BaseFigure{
    std::shared_ptr<BaseFigure> child;
    Vecf<3> center;
    Vecf<3> n_phi;
    // virtual Vecf<3> get_min() const {return rotop(child->get_min()-center)(n_phi).val+center;};
    // virtual Vecf<3> get_max() const {return rotop(child->get_max()-center)(n_phi).val+center;};
    virtual Vecf<3> get_min() const {
        Vecf<3> fcenter = (child->get_max() + child->get_min()) * 0.5;
        Vecf<3> fhdiag  = (child->get_max() - child->get_min()) * 0.5;
        Vecf<3> tmp  = rotop(child->get_min()-center)(n_phi).val+center;
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=-1;iz<=1;iz+=2){
            tmp <<= rotop(fcenter + (fhdiag ^ Vctr(ix,iy,iz)) - center)(n_phi).val+center;
        }
        return tmp;
    };
    virtual Vecf<3> get_max() const {
        Vecf<3> fcenter = (child->get_max() + child->get_min()) * 0.5;
        Vecf<3> fhdiag  = (child->get_max() - child->get_min()) * 0.5;
        Vecf<3> tmp  = rotop(child->get_max()-center)(n_phi).val+center;
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=-1;iz<=1;iz+=2){
            tmp >>= rotop(fcenter + (fhdiag ^ Vctr(ix,iy,iz)) - center)(n_phi).val+center;
        }
        return tmp;
    };    
    virtual bool check(const Vecf<3> &r) const {return child->check(rotop(r-center)(-n_phi).val + center);};
};
*/
//------------------------------------------------------------------------------
struct RotatedFigure: public BaseFigure{ // from aiv
    std::shared_ptr<BaseFigure> child;
    Vecf<3> center;
    Vecf<3> n_phi;
    // virtual Vecf<3> get_min() const {return rotop(child->get_min()-center)(n_phi).val+center;};
    // virtual Vecf<3> get_max() const {return rotop(child->get_max()-center)(n_phi).val+center;};
    virtual Vecf<3> get_min() const {
        Vecf<3> fcenter = (child->get_max() + child->get_min()) * 0.5;
        Vecf<3> fhdiag  = (child->get_max() - child->get_min()) * 0.5;
        Vecf<3> tmp  = rotate(child->get_min()-center, n_phi)+center;
        for(int ix=-1; ix<=1; ix+=2) for(int iy=-1; iy<=1; iy+=2) for(int iz=-1; iz<=1; iz+=2){
					tmp <<= rotate(fcenter + (fhdiag&vecf(ix,iy,iz)) - center, n_phi)+center;
				}
        return tmp;
    };
    virtual Vecf<3> get_max() const {
        Vecf<3> fcenter = (child->get_max() + child->get_min()) * 0.5;
        Vecf<3> fhdiag  = (child->get_max() - child->get_min()) * 0.5;
        Vecf<3> tmp  = rotate(child->get_max()-center, n_phi)+center;
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=-1;iz<=1;iz+=2){
					tmp >>= rotate(fcenter + (fhdiag&vecf(ix,iy,iz)) - center, n_phi)+center;
        }
        return tmp;
    };    
    virtual bool check(const Vecf<3> &r) const {return child->check(rotate(r-center, -n_phi) + center);};
};
//------------------------------------------------------------------------------
Figure Figure::rotate(const Vecf<3> &center, const aiw::Vecf<3> &ort_x, const aiw::Vecf<3> &ort_y){
    RotatedFigure *f = new RotatedFigure; 
    f->child = figure; f->center = center; // f->n_phi = n_phi;
    Figure res; res.figure.reset(f); return res;
}
Figure Figure::rotate(const Vecf<3> &center, const Vecf<3> &n_phi){
    RotatedFigure *f = new RotatedFigure; 
    f->child = figure; f->center = center; f->n_phi = n_phi;
    Figure res; res.figure.reset(f); return res;
}
//------------------------------------------------------------------------------
//   transform
//------------------------------------------------------------------------------
struct TransformedFigure: public BaseFigure{
    std::shared_ptr<BaseFigure> child;
    Vecf<3> center;
    Vecf<3> ox;
    Vecf<3> oy;
    Vecf<3> oz;

    // virtual Vecf<3> get_min() const {return trans_vec(child->get_min(), center, ox, oy, oz);};
    // virtual Vecf<3> get_max() const {return trans_vec(child->get_max(), center, ox, oy, oz);};
    virtual Vecf<3> get_min() const {
        Vecf<3> fcenter = (child->get_max() + child->get_min()) * 0.5;
        Vecf<3> fhdiag  = (child->get_max() - child->get_min()) * 0.5;
        Vecf<3> tmp  = trans_vec(child->get_min(), center, ox, oy, oz);
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=-1;iz<=1;iz+=2){
            tmp <<= trans_vec(fcenter + (fhdiag&vecf(ix,iy,iz)), center, ox, oy, oz);
        }
        return tmp;
    };
        
    virtual Vecf<3> get_max() const {
        Vecf<3> fcenter = (child->get_max() + child->get_min()) * 0.5;
        Vecf<3> fhdiag  = (child->get_max() - child->get_min()) * 0.5;
        Vecf<3> tmp  = trans_vec(child->get_max(), center, ox, oy, oz);
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=-1;iz<=1;iz+=2){
            tmp >>= trans_vec(fcenter + (fhdiag&vecf(ix,iy,iz)), center, ox, oy, oz);
        }
        return tmp;
    };

    virtual bool check(const Vecf<3> &r) const {return child->check(trans_vec_back(r, center, ox, oy, oz));};
};
//------------------------------------------------------------------------------
struct MovedFigure: public BaseFigure{
    std::shared_ptr<BaseFigure> child;
    Vecf<3> offset;

    virtual Vecf<3> get_min() const {return child->get_min()+offset;};
    virtual Vecf<3> get_max() const {return child->get_max()+offset;};
    virtual bool check(const Vecf<3> &r) const {return child->check(r-offset);};
};
//------------------------------------------------------------------------------
Figure Figure::move(const Vecf<3> &offset){
    MovedFigure *f = new MovedFigure; 
    f->child = figure; f->offset = offset;
    Figure res; res.figure.reset(f); return res;
}
//------------------------------------------------------------------------------
Figure Figure::transform(const Vecf<3> &center, const Vecf<3> &ox, const Vecf<3> &oy, const Vecf<3> &oz){
    TransformedFigure *f = new TransformedFigure; 
    f->child = figure; f->center = center; f->ox = ox; f->oy = oy; f->oz = oz;
    Figure res; res.figure.reset(f); return res;
}
//------------------------------------------------------------------------------
struct UnionFigure: public BaseFigure{
    std::shared_ptr<BaseFigure> lchild;
    std::shared_ptr<BaseFigure> rchild;
    virtual Vecf<3> get_min() const { return lchild->get_min() << rchild->get_min(); };
    virtual Vecf<3> get_max() const { return lchild->get_max() >> rchild->get_max(); };
    virtual bool check(const Vecf<3> &r) const { return lchild->check(r) || rchild->check(r); };
};
//------------------------------------------------------------------------------
struct IntersectFigure: public BaseFigure{
    std::shared_ptr<BaseFigure> lchild;
    std::shared_ptr<BaseFigure> rchild;
    virtual Vecf<3> get_min() const { return lchild->get_min() >> rchild->get_min(); };
    virtual Vecf<3> get_max() const { return lchild->get_max() << rchild->get_max(); };
    virtual bool check(const aiw::Vecf<3> &r) const { return lchild->check(r) && rchild->check(r); };
};
//------------------------------------------------------------------------------
struct DifferenceFigure: public BaseFigure{
    std::shared_ptr<BaseFigure> lchild;
    std::shared_ptr<BaseFigure> rchild;
    virtual Vecf<3> get_min() const { return lchild->get_min(); }; // вот тут можно и по-точнее
    virtual Vecf<3> get_max() const { return lchild->get_max(); }; // вот тут можно и по-точнее
    virtual bool check(const aiw::Vecf<3> &r) const { return lchild->check(r) && !rchild->check(r); };
};
//------------------------------------------------------------------------------
Figure Figure::operator + (const Figure &other) const{
    UnionFigure *f = new UnionFigure;
    f->lchild = figure; f->rchild = other.figure;
    Figure res; res.figure.reset(f); return res;
};
//------------------------------------------------------------------------------
Figure Figure::operator * (const Figure &other) const{
    IntersectFigure *f = new IntersectFigure;
    f->lchild = figure; f->rchild = other.figure;
    Figure res; res.figure.reset(f); return res;
};
//------------------------------------------------------------------------------
Figure Figure::operator - (const Figure &other) const{
    DifferenceFigure *f = new DifferenceFigure;
    f->lchild = figure; f->rchild = other.figure;
    Figure res; res.figure.reset(f); return res;
};
//------------------------------------------------------------------------------
Vecf<3> rotmov(const aiw::Vecf<3> &r, const aiw::Vecf<3> &center, const aiw::Vecf<3> &n, double phi){
    if(n.abs()<1e-16) WRAISE("Transformation is incorrect", r, center, n, phi);
    Vec<3> r0 = r-center;
    if(n(0,1).abs()>1e-16) r0 = rotate(r0, vecf(n[1], -n[0], 0.)*acos(n[2]/n.abs()));
	else{ if(n[2]<0) r0 = rotate(r0, vecf(1., 0., 0.)*acos(-1.0)); }
    if(phi > 1e-16) r0 = rotate(r0, n*phi);
    return r0;
}
//------------------------------------------------------------------------------
Vecf<3> rotmov_back(const aiw::Vecf<3> &r, const aiw::Vecf<3> &center, const aiw::Vecf<3> &n, double phi){
    if (n.abs()<1e-16) WRAISE("Transformation is incorrect", r, center, n, phi);
    Vec<3> r0 = r;
    if(phi>1e-16){ r0 = rotate(r0, n*-phi); }
    if(n(0,1).abs()>1e-16) r0 = rotate(r0, vecf(n[1], -n[0], 0.)*acos(n[2]/n.abs())); 
	else { if(n[2]<0) r0 = rotate(r0, vecf(1.,0.,0.)*acos(-1.0)); }
    r0 = r+center;
    return r0;
}
//------------------------------------------------------------------------------
struct Cylinder: public BaseFigure{
    aiw::Vecf<3> boc; // центр нижней грани
    double R, H;    // радиус и высота
    aiw::Vecf<3> n; // направляющая вдоль оси z

    virtual Vecf<3> get_min() const {
        Vecf<3> tmp_min = rotmov_back(vecf(-R,-R,0.), boc, n, 0.);
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=0;iz<=1;iz+=1){
            tmp_min<<=rotmov_back(vecf(R*ix,R*iy,H*iz), boc, n, 0.);
        }
        return tmp_min;
    }
    virtual Vecf<3> get_max() const {
        Vecf<3> tmp_max = rotmov_back(vecf(R,R,H), boc, n, 0.);
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=0;iz<=1;iz+=1){
            tmp_max>>=rotmov_back(vecf(R*ix,R*iy,H*iz), boc, n, 0.);
        }
        return tmp_max;
    }
    virtual bool check(const aiw::Vecf<3> &r) const {
        Vecf<3> r0 = rotmov(r, boc, n, 0.);
        return fabs(r0[2]-H*0.5)<=H*0.5 && r0(0,1).abs()<=R;
    }
};
//------------------------------------------------------------------------------
struct Box: public BaseFigure{
    aiw::Vecf<3> boc; // центр нижней грани
    double A, B, H;    // стороны нижней грани и высота
    aiw::Vecf<3> n; // направляющая вдоль оси z
    double phi; // угол поворота относительно оси z

    virtual Vecf<3> get_min() const {
        Vecf<3> tmp_min = rotmov_back(vecf(-A*0.5,-B*0.5,0.), boc, n, phi);
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=0;iz<=1;iz+=1){
            tmp_min<<=rotmov_back(vecf(A*ix*0.5,B*iy*0.5,H*iz), boc, n, phi);
        }
        return tmp_min;
    }
    virtual Vecf<3> get_max() const {
        Vecf<3> tmp_max = rotmov_back(vecf(A*0.5,B*0.5,H), boc, n, phi);
        for (int ix=-1;ix<=1;ix+=2) for (int iy=-1;iy<=1;iy+=2) for (int iz=0;iz<=1;iz+=1){
            tmp_max>>=rotmov_back(vecf(A*ix*0.5,B*iy*0.5,H*iz), boc, n, phi);
        }
        return tmp_max;
    }
    virtual bool check(const aiw::Vecf<3> &r) const {
        Vecf<3> r0 = rotmov(r, boc, n, phi);
        return fabs(r0[2]-H*0.5)<=H*0.5 && fabs(r0[1])<=B*0.5 && fabs(r0[0])<=A*0.5;
    }
};
//------------------------------------------------------------------------------
struct Sphere: public BaseFigure{
    aiw::Vecf<3> center; // центр
    double R;    // радиус

    virtual Vecf<3> get_min() const { return center - Vecf<3>(float(R)); }
    virtual Vecf<3> get_max() const { return center + Vecf<3>(float(R)); }
    virtual bool check(const aiw::Vecf<3> &r) const { return (r-center).abs()<=R; }
};
//------------------------------------------------------------------------------
Figure aiw::cylinder(const aiw::Vecf<3> &bottom_origin_center, const aiw::Vecf<3> &n, double R, double H){
    Cylinder* f = new Cylinder;
    f->boc = bottom_origin_center;
    f->n = n; f->R = R; f->H = H;
    Figure res; res.figure.reset(f); return res;
};
//------------------------------------------------------------------------------
Figure aiw::box(const aiw::Vecf<3> &bottom_origin_center, const aiw::Vecf<3> &n, double phi, double A, double B, double H){
    Box* f = new Box;
    f->boc = bottom_origin_center;
    f->n = n; f->phi = phi; f->A = A; f->B = B; f->H = H;
    Figure res; res.figure.reset(f); return res;
};
//------------------------------------------------------------------------------
Figure aiw::cube(const aiw::Vecf<3> &bottom_origin_center, const aiw::Vecf<3> &n, double phi, double A){
    Box* f = new Box;
    f->boc = bottom_origin_center;
    f->n = n; f->phi = phi; f->A = A; f->B = A; f->H = A;
    Figure res; res.figure.reset(f); return res;
};
//------------------------------------------------------------------------------
Figure aiw::spheroid(const aiw::Vecf<3> &center, double R){
    Sphere* f = new Sphere;
    f->center = center; f->R = R; 
    Figure res; res.figure.reset(f); return res;

};
//------------------------------------------------------------------------------
