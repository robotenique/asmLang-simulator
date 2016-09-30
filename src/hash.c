#include <stdio.h>
#include <string.h>

const int MAXTABLE = 1e7;

char word[50];
char table[10000000][50];
int p = 1e7 - 9;

long long po(int x, int e){
	long long res = 1;
	for(int i=0; i<e; i++) res = (res * x) % p;
	return res;
}

int hash(char* c){
	int t = 0, res = 0;

	for(; c[t]; t++);

	for(int i=0; i<t; i++){
		if(c[i] >= '0' && c[i] <= '9') res = (res + (c[i]-'0'+1)*po(70, i)) % p;
		else if(c[i] >= 'A' && c[i] <= 'Z') res = (res + (c[i]-'A'+11)*po(70, i)) % p;
		else if(c[i] >= 'a' && c[i] <= 'z') res = (res + (c[i]-'a'+37)*po(70, i)) % p;
		else if(c[i] == '-') res = (res + (c[i]-'-'+63)*po(70, i)) % p;
		else if(c[i] == '_') res = (res + (c[i]-'_'+64)*po(70, i)) % p;
		else if(c[i] == '.') res = (res + (c[i]-'.'+65)*po(70, i)) % p;
	}

	return res;
}

/*int main(){
	scanf("%s", word);

	int h = hash(word);
	while(strcmp(table[h], word) == 0) h = (h + 1) % MAXTABLE;
	strcpy(table[h], word);

	printf("%d\n", h);

	return 0;
}*/
