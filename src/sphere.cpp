/**
 * Copyright (C) 2016 Antov V. Ivanov and Sergey Khilkov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

using namespace aiw{
    using Ind3 = Vec<3,uint64_t>;
    using Ind6 = Vec<6,uint64_t>;
    inline template<int N,typename T> Vec<N,T> shift( const Vec<N,T>& r, int i ) { 
        Vec<N,T> result;
        if (i>=N || i<=-N) i = i%N;
        if (i<0) I+=N;
        for(int j=i; j<N; j++) result[j-i]=r[j];
        for(int j=0; j<i; j++) result[N-i+j]=r[j];
        return result;
    }
    //---------------------------------------------------------------------------------------------------------
    inline Vec<3> refr( Vec<3> r, int i ) { int p = i%3; r[p] =- r[p]; return r; } 	//Отражение от i-й(mod 3) координатной плоскости
    //---------------------------------------------------------------------------------------------------------
    int current_rank=-1;
    Vec<3>* cell_centers[MAX_RANK] = INIT_ARR_ZERO;     // центры ячеек
    double*  cell_areas[MAX_RANK] = INIT_ARR_ZERO;       // площади ячеек
    Ind3* cell_vertex[MAX_RANK] = INIT_ARR_ZERO;      // индексы вершин ячейки
    Vec<3>* vertex = 0;           // координаты вершин
    Ind3* cell_neighbours[MAX_RANK] = INIT_ARR_ZERO;  // индексы соседних ячеек (для ячейки)
    Ind6* vertex_cells[MAX_RANK] = INIT_ARR_ZERO;     // индексы ячеек (для вершины)
    Vec<3>* normals[MAX_RANK] = INIT_ARR_ZERO;          // нормали (хранятся тройками?)
    void sph_init_table(int rank){
        //	WOUT(_R, AR);
        if( current_rank>=rank ) return;
        else {
            Vec<3> *tmp3 = vertex;
            vertex = new Vec<3>[N / 2 + 2];
            for(long i=0; i< (60 << 2*A) / 2 + 2; i++) vertex[i] = tmp3[i];
            delete [] tmp3;
            for(int i=current_rank; i<=rank; i++) arrs_init(rank); // инициализация массивов
        }
        current_rank=rank;
        //	WOUT(2);
    }
    void init_zero_rank(){
        // здесь должна быть инициализация массивов для 0-го уровня рекурсии.
        // Заполнение vertex, vertex_cells, cell_vertex
        // Здесь два типа вершин - вершины додекаэдра и центры граней,
        // их необходимо обрабатывать отдельно, примерно так, как это сделано в  incid
        // параллельно нужно будет проставить им номера, и задать их координаты
        //  вместе с этим задаются вершины соответствующие одной ячейке.
        const double _edge_arg = 9.-8.*cos(0.6*M_PI)+sqrt(13.-16.*cos(0.6*M_PI)); //длина ребра додекаэдра
        const double edge = 2.*sqrt(2./_edge_arg); //длина ребра додекаэдра
        const Vec<3> vect0[2] = {Vec<3>( .5*edge, 0, sqrt( 1. - .25*edge*edge ) ),
            Vec<3>(1./ sqrt(3) )}; //2 вершины додеккаэдра додекаэдра
        cell_vertex[0] = new Ind3[60];
        vertex_cells[0] = new Ind6[32];
        cell_neighbours[0] = new Ind3[60];
        //if (!vertex) vertex = new Vec<3>[32];//impossible
        int ver = 0;
        bool* tmp = new bool[60];
        cell_centers[0] = new Vec<3>[72];
        for (int i = 0; i < 60; i++) tmp[i] = 0;
        for (int i=0; i<12; i++) {
            indx<3> ind = Ind3(1,1,1);
            int ind3 = i>>2;
            ind[ (ind3+1)%3 ] = (i%2)*2 - 1;
            ind[ (ind3+2)%3 ] = (i%4) - ( ( ind[ (ind3+1)%3 ] + 1 ) >> 1 ) - 1;
            int l = ind*Ind3(1,1,1), j = (l<0) ? (ind3+2)%3 : (3 - ind3)%3;
            //пятиугольник по часовой стрелке.
            Vec<3> pentag[5];
            pentag[ (5-l)%5 ] = shift( vect0[0], (3+l*j)%3 )*ind;
            pentag[ (6-l)%5 ] = vect0[1]*ind;
            pentag[ (7-l)%5 ] = shift( vect0[0], (3+l*(j+1))%3 )*ind;
            pentag[ (8-l)%5 ] = refr( pentag[ (2 - (l+1)/2 - l)%5 ], ind3 ); 
            pentag[ (9-l)%5 ] = refr( pentag[ (6 - (l+1)/2 - l)%5 ], ind3 );
            Vec<3> cent(0.);
            //vctr<3> x  = pentag[0];
            for(int k=0; k<5; k++){
                if ( !tmp[5*i+k] ) {
                    //Задание треугольников инцидентных вершине.
                    Ind6 buf;
                    buf[0] = 5*i+k;
                    buf[5] = 5*i+((k+1)%5);
                    Vec<5,uint64_t> tmp2 = down_nb(ind|ind3|((k+1)%5));
                    buf[4] = id(tmp2);
                    tmp2[4] = (tmp2[4]+1)%5;
                    buf[3] = id(tmp2);
                    tmp2 = down_nb(tmp2);
                    buf[2] =  id(tmp2);
                    tmp2[4] = (tmp2[4]+1)%5;
                    buf[1] = id(tmp2);
                    vertex_cells[0][ver] = buf;
                    //Заполнение массива вершин
                    vertex[ver] = pentag[k];
                    ver++;
                    //Исключение дублирования вершин
                    tmp[buf[0]] = 1;
                    tmp[buf[2]] = 1;
                    tmp[buf[4]] = 1;
                    for(int p=0; p<6;p++){
                        cell_vertex[0][ buf[p] ][ (p%2)*2 ] = ver-1; 
                    }
                }
                cent += pentag[k];
            }
            vertex[ver]  = cent/cent.abs();
            cell_centers[0][60+i] = vertex[ver];
            vertex_cells[0][ver] = Ind6( 5*i, 5*i+1,5*i+2, 5*i+3, 5*i+4, -1);
            for(int p=0; p<5;p++){
                cell_vertex[0][ vertex_cells[0][ver][p] ][1] = ver;
            }
            ver++;
        }
        if (tmp) {delete [] tmp;tmp=0;}
        for(int i =0 ; i<32;i++){
            Ind6 cur = vertex_cells[0][i];
            if (cur[5]>0){
                for(int j=0; j<6;j++){
                    cell_neighbours[0][cur[j]][((j%2)*2+1)%3] = cur[(j+1)%6];//Здесь тоже есть лажа, но этого не видно, может её и нет?
                    cell_neighbours[0][cur[j]][((j%2)*2+2)%3] = cur[(j+5)%6];
                }
            } else {
                for(int j=0; j<5; j++){
                    cell_neighbours[0][cur[j]][2] = cur[(j+1)%5];
                    cell_neighbours[0][cur[j]][0] = cur[(j+4)%5];
                }
            }
        }

    }
    void arrs_init( int rank ){
        if (rank==0) init_zero_rank();
        else {
            if (rank>0){
                //тут нужна другая функция
                //Увеличивать быстродействе здесь будем потом
                //Заполнение vertex vertex_cells  cell_vertex
                int CurN = 60<<2*rank;
                int N_1 = CurN/4;//Для читаемости.
                int k = CurN/2 + 2;
                cell_vertex[rank] = new Ind3[CurN];
                vertex_cells[rank] = new Ind6[k];
                bool* tmp = new bool[N_1*3];//т.к. в нашем обходе вершины могут (и будут) встречаться дважды, мы будем проверять, что они ещё не пройдены
                for (int i = 0; i < N_1*3; i++) tmp[i] = 0;
                Ind3  cni[3] = {
                    Ind3(3,0,2),
                    Ind3(1,0,3),
                    Ind3(2,0,1)
                };//треугольники при вершине со стороны 0 ,1 или 2
                Ind3  vni[3] = {
                    Ind3(1,0,2),
                    Ind3(2,1,0),
                    Ind3(0,2,1)
                };// номера вершин в соответствующих треугольниках
                int exch[3] = {2, 1, 0};//перестановка (2,0)
                int ver = N_1/2+2;
                for ( int i = 0; i < N_1; i++ ){//цикл по всем центральным треугольникам нашей сетки
                    for (int num = 0; num < 3; num++ ){//цикл по номеру вершины центрального треугольника
                        if ( (!tmp[ 3*i + num ])){//&& (ver< k) ){
                            // необходимость в дополнительнм условии пропала
                            int nb  = cell_neighbours[rank-1][i][num];
                            //                    if (rank==8) WOUT(i, nb, num);
                            bool orient = (cell_vertex[rank-1][i][1]==cell_vertex[rank-1][nb][1]);
                            vertex[ver] = vertex[ cell_vertex[rank-1][i][(num+1)%3] ] + vertex[cell_vertex[rank-1][i][(num+2)%3]];
                            vertex[ver]*=1/vertex[ver].abs();
                            Ind3 ind(i*4);
                            Ind3 inb(nb*4);
                            vertex_cells[rank][ver] = (ind+cni[num])|(inb+cni[ exch[num]*orient +(1-orient)*num ]);//ещё пройтись по всем из vertex_cells  и записать в cell_vertex
                            for(int l = 0; l < 3; l++){
                                cell_vertex[rank][ vertex_cells[rank][ver][l] ] [ vni[num][l] ] = ver;
                                cell_vertex[rank][ vertex_cells[rank][ver][l+3] ] [  vni[exch[num]*orient +(1-orient)*num ][l] ] = ver;
                            }
                            tmp[3*nb+orient*exch[num] + (1-orient)*num] = 1;
                            tmp[3*i+num]=1;
                            ver++;
                        }
                    }
                }
                if ( tmp ){ delete [] tmp; tmp=0;}
                //для предыдущих уровней распространить vertex_cells  на текущий и дописать cell_vertex
                if (rank>1) {
                    int N_2 = N_1/4;
                    for(int i = 0; i< N_2/2+2; i++ ){
                        Ind6 cur  = vertex_cells[rank-1][i];
                        Ind6 next = Ind6(cur[0]%4,cur[1]%4,cur[2]%4, cur[3]%4, cur[4]%4, cur[5]>0?cur[5]%4:3 );
                        vertex_cells[rank][i] = cur * 4 + next;
                        for(int l = 0; l <(cur[5]<0?5:6); l++){
                            cell_vertex[rank][ vertex_cells[rank][i][l] ] [ next[l]-1 ] = i;
                        }//next[l] не может быть 0, т.к. 0 инцидентны только вершинам появившимся на текущем уровне
                    }
                    for(int i = N_2/2+2; i < N_1/2+2; i++ ){//Вершины появившиеся на предыдущем уровне рекурсии
                        Ind6 cur =vertex_cells[rank-1][i];
                        int num1 = 5 - (cur[0]%4) -(cur[2]%4), num2 = 5 - (cur[3]%4) - (cur[5]%4);
                        Ind6 next = (vni[num1]|vni[num2])+ indx<6>(1);
                        vertex_cells[rank][i] = cur * 4 +next;
                        for(int l =0 ; l<6; l++){
                            cell_vertex[rank][ vertex_cells[rank][i][l] ][next[l] -1] = i;
                        }//среди этих вершин заведомо нет центров граней
                    }
                } else {
                    for(int i=0; i< 32; i++){
                        Ind6 cur = vertex_cells[0][i];
                        Ind6 next = (cur[5]!=-1)? Ind6(1,3,1,3,1,3):Ind6(2,2,2,2,2,3);
                        vertex_cells[rank][i] = cur*4+next;
                        for(int l = 0; l < (cur[5]<0?5:6); l++){
                            cell_vertex[rank][ vertex_cells[rank][i][l] ] [ next[l] -1 ] = i;
                        }
                    }
                }
            }
        }
        mass_finish(rank);
    }
    //---------------------------------------------------------------------------------------------------------
    void base_sphere::mass_finish(int rank){
        int CurN = 60<<2*rank;
        if (rank!=0){
            if(!normals[rank]){
                normals[rank] = new Vec<3>[3 *  CurN/4];
            //нормали
                for(int k = 0; k <CurN/4 ; k ++){
                    Vec<3>  r0 = vertex[ cell_vertex[rank][4*k][0] ],r1 = vertex[ cell_vertex[rank][4*k][1] ], r2 = vertex[ cell_vertex[rank][4*k][2] ];
                    normals[rank][ 3*k ] = r2%r1;
                    normals[rank][ 3*k+1 ] = r0%r2;
                    normals[rank][ 3*k+2 ] = r1%r0;
                }
            }
        }
        if(!cell_areas[rank]){
            cell_areas[rank] = new double[CurN];
        }
        if(!cell_centers[rank]){
            cell_centers[rank] =  new Vec<3>[CurN];
        }
        //площади и центры
        for(int k=0; k<CurN ; k++){
            Vec<3>  r0 = vertex[ cell_vertex[rank][k][0] ],r1 = vertex[ cell_vertex[rank][k][1] ], r2 = vertex[ cell_vertex[rank][k][2] ];
            cell_areas[rank][k] = ((r0-r1)%(r0-r2)*0.5).abs();
            cell_centers[rank][k] = (r0+r1+r2)/(r0+r1+r2).abs();
        }
        //соседи 
        if(!cell_neighbours[rank]){
            cell_neighbours[rank] = new Ind3[CurN];
            for(int k =0; k< 32; k++){
                Ind6 cur  = vertex_cells[rank][k];
                if(cur[5] > 0) {
                    for(int i=0; i<6; i++){
                        int num = (i%2)*2;
                        cell_neighbours[rank][cur[i]][(num+2)%3] = cur[(i+5)%6];
                        cell_neighbours[rank][cur[i]][(num+1)%3] = cur[(i+1)%6];
                    }
                } else {
    //                int num = 1;
                    for(int i=0; i<5; i++){
                        cell_neighbours[rank][cur[i]][0] = cur[(i+4)%5];
                        cell_neighbours[rank][cur[i]][2] = cur[(i+1)%5];
                    }
                }
                
            }
            for(int k =32; k< CurN/8+2; k++){
                Ind6 cur  = vertex_cells[rank][k];
                for(int i =0; i<6;i++){
                    int num = cur[i]%4 - 1;//0 быть не должно
                    //нужно узнать номер вершины для удобного задания порядка
                    cell_neighbours[rank][cur[i]][(num+2)%3] = cur[(i+5)%6];
                    cell_neighbours[rank][cur[i]][(num+1)%3] = cur[(i+1)%6];
                    }
            }
            for(int k = CurN/8+2; k<CurN/2+2; k++){
                Ind6 cur = vertex_cells[rank][k];
                int num[2] ={5 - cur[0]%4 - cur[2]%4, 5 - cur[3]%4 - cur[5]%4};
                Ind3 vni[3] = {
                    Ind3(1,0,2),
                    Ind3(2,1,0),
                    Ind3(0,2,1)
                };// номера вершин в соответствующих треугольниках
                for(int i=0; i<3; i++){
                    cell_neighbours[rank][cur[i]][(vni[num[0]][i]+1)%3] = cur[(i+1)%6];
                    cell_neighbours[rank][cur[i]][(vni[num[0]][i]+2)%3] = cur[(i+5)%6];
                    cell_neighbours[rank][cur[i+3]][(vni[num[1]][i]+1)%3] = cur[(i+4)%6];
                    cell_neighbours[rank][cur[i+3]][(vni[num[1]][i]+2)%3] = cur[(i+2)%6];
                }
            }
        }
    }
}
