#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <stdlib.h>
using namespace std;
const int maxn = 2000000;
char s[maxn];
int nxt[maxn];

void getNextVal_Opt(char *s){
	int len = strlen(s);
	nxt[0] = -1;
	int j = 0, k = -1;
	while(j < len - 1){
		if(k == -1 || s[j] == s[k]){
			k++;
			j++;
			if(s[j] != s[k])
				nxt[j] = k;
			else
				nxt[j] = nxt[k];
		}
		else
			k = nxt[k];
	}
}

void getNextVal_UnOpt(char *s){
	int len = strlen(s);
	nxt[0] = -1;
	int j = 0, k = -1;
	while(j < len){
		if(k == -1 || s[j] == s[k]){
			k++;
			j++;
			nxt[j] = k;
		}
		else
			k = nxt[k];
	}
}

int main(){
	while(cin >> s){
		if(s[0] == '.')
			break;
		getNextVal_UnOpt(s);
		int len = strlen(s);
		int ans = len % (len - nxt[len]) ? 1 : len / (len - nxt[len]);
		printf("%d\n", ans);
	}
	return 0;
}

