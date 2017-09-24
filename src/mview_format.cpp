#include "../include/aiwlib/mview_format.hpp"
using namespace aiw;
//------------------------------------------------------------------------------
// запись вектора в поток
template <typename T, typename TN=uint32_t>
void dump_vector(const std::vector<T> &V, aiw::File &stream){
    TN N = V.size(); stream.write(&N, sizeof(N));
    if(N) stream.write(&(V[0]), sizeof(T)*V.size());
}
//------------------------------------------------------------------------------
// запись вектора в поток
template <typename T, typename TN=uint32_t>
void load_vector(std::vector<T> &V, aiw::File &stream){
    TN N; stream.read(&N, sizeof(N));
    if(N){ V.resize(N); stream.read(&(V[0]), sizeof(T)*V.size()); }
}
//------------------------------------------------------------------------------
// запись физических параметров подрешетки в поток
void MViewLattice::dump(aiw::File &stream) const {
    stream.write(&Ms, sizeof(Ms));
    dump_vector(K1, stream); dump_vector(K3, stream);
}
//------------------------------------------------------------------------------
// чтение физических параметров подрешетки из потока
void MViewLattice::load(aiw::File &stream){
    stream.read(&Ms, sizeof(Ms));
    load_vector(K1, stream); load_vector(K3, stream);
}
//------------------------------------------------------------------------------
// добавляет магнитный момент в заголовок
// возвращает номер магнитного момента
int MViewFormat::add( aiw::Vec<3, float> coord,  // координаты момента
                      int lattice,                // номер подрешетки
                      float volume                // объем ячейки
                      ){
    if(lattice>=0){
        if(l_indexes.size()!=coords.size())
            WRAISE(l_indexes.size(), coords.size());
        l_indexes.push_back(lattice);
    }
    if(volume>0.f){
        if(volumes.size()!=coords.size())
            WRAISE(volumes.size(), coords.size());
        volumes.push_back(volume);
    }
    coords.push_back(coord);
    return coords.size()-1;
}
//------------------------------------------------------------------------------
// записывает заголовок в поток
void MViewFormat::dump_head(aiw::File &stream, bool pack, bool dumpHext){
    if(volumes.size() && volumes.size()!=coords.size())
        WRAISE("incorrect", volumes.size(), coords.size(), "expected");
    if(l_indexes.size() && l_indexes.size()!=coords.size())
        WRAISE("incorrect", l_indexes.size(), coords.size(), "expected");

    pack_format = pack; Hext_format = dumpHext;
    if(pack_format){ aiw::sph_init_table(5); pack_v.resize(coords.size()); }

    // описание формата
    uint32_t F = 0x6D77;
    F |= (int)pack<<31; F |= (int)Hext_format<<30;
    if(l_indexes.size() && l_params.size()) F |= 1<<29;
    if(exch_links.size()) F |= 1<<28;
    if(offsets.size()) F |= 1<<27;
    if(volumes.size()) F |= 1<<26;
    if(Hstoch.size())  F |= 1<<25;
    stream.write(&F, 4);

    dump_vector(coords, stream); // координаты моментов
    if(volumes.size()) stream.write(&(volumes[0]), 4*volumes.size()); // объемы ячеек
    if(offsets.size()) dump_vector(offsets, stream); // сдвиги центров ячеек
    if(exch_links.size()) dump_vector<MViewLink, uint64_t>(exch_links, stream); // обменные связи
    if(l_indexes.size() && l_params.size()){ // подрешетки
        uint32_t N = l_params.size(); stream.write(&N, 4);
        for(uint32_t i=0; i<N; i++) l_params[i].dump(stream);
        dump_vector(l_indexes, stream);
    }
    if(Hstoch.size()) dump_vector(Hstoch, stream); // дефекты типа случайное поле
}
//------------------------------------------------------------------------------
// читает заголовок из потока
void MViewFormat::load_head(aiw::File & stream){
    // описание формата
    uint32_t F; stream.read(&F, 4);
    if(F && (F&0xffff)!=0x6D77) WRAISE("incorrect format", F);
    pack_format = F&(1<<31); Hext_format = F&(1<<30);

    load_vector(coords, stream); // координаты моментов
    if(F&(1<<26)){ volumes.resize(coords.size()); stream.read(&(volumes[0]), 4*volumes.size()); } // объемы ячеек
    if(F&(1<<27)) load_vector(offsets, stream); // сдвиги центров ячеек
    if(F&(1<<28)) load_vector<MViewLink, uint64_t>(exch_links, stream); // обменные связи
    if(F&(1<<29)){ // подрешетки
        uint32_t N; stream.read(&N, 4); l_params.resize(N);
        for(uint32_t i=0; i<N; i++) l_params[i].load(stream);
        load_vector(l_indexes, stream);
    }
    if(F&(1<<25)) load_vector(Hstoch, stream); // дефкты типа случайное поле
    if(pack_format){ aiw::sph_init_table(PACKED_RANK); pack_v.resize(coords.size()); }
}
//------------------------------------------------------------------------------
// записывает кадр в поток
void MViewFormat::dump_frame( aiw::Vec<3, float> *data,  // массив магнитных моментов
                              aiw::File &stream        // поток aiwlib
                              ){
    stream.write(&time, sizeof(time));
    if(Hext_format) stream.write(&Hext, sizeof(Hext));
    if(pack_format){
        for(uint32_t i=0; i<coords.size(); i++)    pack_v[i] = aiw::sph_cellInd((aiw::Vec<3>)(data[i]), PACKED_RANK);
        stream.write(&(pack_v[0]), 2*coords.size());
    } else stream.write(data, coords.size()*12);
}
//------------------------------------------------------------------------------
// читает кадр из потока, возвращает true (если чтение прошло успешно)
// либо false (если чтение не удалось, например по окончании потока)
// устанавливает поля time и Hext
bool MViewFormat::load_frame( aiw::Vec<3, float> *data, // массив магнитных моментов
                              aiw::File & stream       // поток aiwlib
							  ){
    if ( ! stream.read(&time, sizeof(time))/* != sizeof(time)*/) return false;
    if(Hext_format) if( ! stream.read(&Hext, sizeof(Hext))/* != sizeof(Hext)*/ ) return false ;
    if(pack_format){
        if( ! stream.read(&(pack_v[0]), 2*coords.size())/* != 2*coords.size()*/ ) return false;
        for(uint32_t i=0; i<coords.size(); i++)    data[i] = aiw::sph_cell(pack_v[i], PACKED_RANK);
    } else if ( ! stream.read(data, coords.size()*12)/* != 12*coords.size() */) return false;
    return true;
}
//------------------------------------------------------------------------------
