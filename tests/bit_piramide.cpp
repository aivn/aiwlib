#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "../include/aiwlib/bit_piramide"
using namespace aiw;

int main(int argc, const char **argv){
	BitPiramide<2, 5> P; int errors = 0;

	P.fill(0); 
	{
		std::vector<bool> V(P.size, false);
		for(uint64_t i=0; i<P.size/4; i++){ int j = random()%P.size; P.setY(j); V[j] = true; }
		for(uint64_t i=0; i<P.size; i++) if(P.get(i)!=V[i]){ printf("%ld %i %i\n", i, P.get(i), bool(V[i])); errors++; }
		P.out2std();
	}
	
	uint64_t nb = ~uint64_t(0);
	for(uint64_t i=0; i<P.size; i++){
		if(P.less(i)!=nb) printf("i=%lu nb=%lu less=%lu\n", i, P.less(i), nb);
		if(P.get(i)) nb = i;
	}

	nb = ~uint64_t(0);
	for(uint64_t i=P.size-1; i; i--){
		if(P.more(i)!=nb) printf("i=%lu nb=%lu more=%lu\n", i, P.more(i), nb);
		if(P.get(i)) nb = i;
	}
	//--------------------------------------------------------------------------
	P.fill(1);
	{
		std::vector<bool> V(P.size, true);
		for(uint64_t i=0; i<2*P.size; i++){ int j = random()%P.size; P.setN(j); V[j] = false; }
		for(uint64_t i=0; i<P.size; i++) if(P.get(i)!=V[i]){ printf("%ld %i %i\n", i, P.get(i), bool(V[i]));  errors++; }
		P.out2std();
	}
	
	nb = ~uint64_t(0);
	for(uint64_t i=0; i<P.size; i++){
		if(P.less(i)!=nb) printf("i=%lu nb=%lu less=%lu\n", i, P.less(i), nb);
		if(P.get(i)) nb = i;
	}

	nb = ~uint64_t(0);
	for(uint64_t i=P.size-1; i; i--){
		if(P.more(i)!=nb) printf("i=%lu nb=%lu more=%lu\n", i, P.more(i), nb);
		if(P.get(i)) nb = i;
	}
	printf("\n%i ERRORS\n", errors);

	return 0;
}
