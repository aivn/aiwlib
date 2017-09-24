#ifndef MVIEW_FORMAT_HPP
#define MVIEW_FORMAT_HPP

#define PACKED_RANK 5
#include <vector>
#include "vec"
#include "sphere"
//------------------------------------------------------------------------------
// обменная связь между двумя атомами
struct MViewLink{
    int i, j; // номера моментов между которыми установлена связь
    float J;  // величина обменного взаимодействия
};
//------------------------------------------------------------------------------
// описание оси анизотропии
struct MViewAniso{
    float K;               // величина анизотропии
    aiw::Vec<3, float> n; // направление оси анизотропии
};
//------------------------------------------------------------------------------
// описани примеси типа случайное поле
struct MViewHstoch{
    int i;                 // номер магнитного момента
    aiw::Vec<3, float> H; // случайное поле
};
//------------------------------------------------------------------------------
class MViewFormat;
// описание одной подрешетки или гранулы
class MViewLattice{
    std::vector<MViewAniso> K1, K3; // массивы осей анизотропии

    // запись физических параметров подрешетки в поток
    void dump(aiw::File &stream) const;

    // чтение физических параметров подрешетки из потока
    void load(aiw::File &stream);

    friend class MViewFormat;
public:
    float Ms; // модуль магнитного момента

    // число осей линейной анизотропии
    int aniso1_size() const { return K1.size(); }

    // добавляет линейную ось анизотропии
    // возвращает номер оси
    int add_aniso1(){ K1.emplace_back(); return K1.size()-1; }

    // доступ к линейной оси анизотропии
    MViewAniso& aniso1(int i){ return K1[i]; }
    const MViewAniso& aniso1(int i) const { return K1[i]; }

    // число осей кубической анизотропии
    int aniso3_size() const { return K3.size(); }

    // добавляет кубическую ось анизотропии
    // возвращает номер оси
    int add_aniso3(){ K3.emplace_back(); return K3.size()-1; }

    // доступ к кубической оси анизотропии
    MViewAniso& aniso3(int i){ return K3[i]; }
    const MViewAniso& aniso3(int i) const { return K3[i]; }
};
//------------------------------------------------------------------------------
// описание сдвига центра ячейки (магнитного момента)
// для сохранения данных о коррекции поля края
struct MViewCellOffset{
    int cell;                    // номер ячейки
    aiw::Vec<3, float> offset;  // сдвиг центра ячейки
};
//------------------------------------------------------------------------------
class MViewFormat{
protected:
    std::vector<aiw::Vec<3, float> > coords; // координаты магнитных моментов
    std::vector<int> l_indexes;               // распределение моментов по подрешеткам
    std::vector<MViewLattice> l_params;       // параметры подрешеток
    std::vector<MViewLink> exch_links;        // обменные связи
    std::vector<float> volumes;               // объемы ячеек
    std::vector<MViewCellOffset> offsets;     // ненулевые сдвиги центров ячеек
    std::vector<MViewHstoch> Hstoch;          // дефекты типа случайное поле
    bool pack_format, Hext_format;
    //aiw::sphere sph;
    std::vector<uint16_t> pack_v;
public:
    aiw::Vec<3> Hext; // внешнее поле
	double time;       // время кадра
	
    // число магнитных моментов в заголовке
    int size() const { return coords.size(); }

    // добавляет магнитный момент в заголовок
    // возвращает номер магнитного момента
    int add( aiw::Vec<3, float> coord,  // координаты момента
             int lattice=-1,             // номер подрешетки
             float volume=0.             // объем ячейки
             );

    // возвращает координаты момента i
    aiw::Vec<3, float> get_coord(int i) const { return coords[i]; }

    // возвращает номер подрешетки момента i
    int get_lattice(int i) const { return l_indexes.size()? l_indexes[i]: -1; }

    // возвращает объем ячейки i
    float get_volume(int i) const { return volumes.size()? volumes[i]: 0; }

    // устанавливает сдвиг центра ячейки i
    void set_offset(int i, const aiw::Vec<3> &offset){
        offsets.emplace_back(); offsets.back().cell = i; offsets.back().offset = offset;
    }

    // возвращает число ненулевых сдвигов центров ячеек
    int offsets_size() const { return offsets.size(); }

    // возвращает k-й сдвиг центра ячейки
    MViewCellOffset get_offset(int k) const { return offsets[k]; }

    // добавляет обменную связь между моментами i и j
    // возвращает номер новой связи
    int add_exch_link(int i, int j, double J){ exch_links.emplace_back(); return exch_links.size()-1; }

    // возвращает число обменных связей
    uint64_t exch_links_size() const { return exch_links.size(); }

    // возвращает обменную связь номер k
    MViewLink get_exch_link(int k) const { return exch_links[k]; }

    // возвращает число подрешеток в заголовке
    int lattices_size() const { return l_params.size(); }

    // добавляет подрешетку, возвращает номер подрешетки
    int add_lattice(){ l_params.emplace_back(); return l_params.size()-1; }

    // доступ к подрешетке l
    MViewLattice& lattice(int l){ return l_params[l]; }
    const MViewLattice& lattice(int l) const { return l_params[l]; }

    // устанавливает дефект типа случайное поле для момента i
    void set_Hstoch(int i, const aiw::Vec<3> H){ Hstoch.emplace_back(); Hstoch.back().i = i; Hstoch.back().H = H; }

    // возвращает число дефектов типа случайное поле
    int Hstoch_size() const { return Hstoch.size(); }

    // возвращает дефект типа случайное поле номер k
    MViewHstoch Hstoch_size(int k) const { return Hstoch[k]; }

    // записывает заголовок в поток
    void dump_head(aiw::File &stream, bool pack=true, bool dumpHext=false);

    // читает заголовок из потока
    void load_head(aiw::File &stream);

    // записывает кадр в поток
    void dump_frame( aiw::Vec<3, float> *data,  // массив магнитных моментов
                     aiw::File &stream        // поток aiwlib
                     );

    // читает кадр из потока, возвращает true (если чтение прошло успешно)
    // либо false (если чтение не удалось, например по окончании потока)
	// устанавливает поля time и Hext
    bool load_frame( aiw::Vec<3, float> *data, // массив магнитных моментов
                     aiw::File &stream       // поток aiwlib
                     );
};
//------------------------------------------------------------------------------
#endif //MVIEW_FORMAT_HPP
