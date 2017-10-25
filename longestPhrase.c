#include <stdio.h>
#define MAXLINE 1000

int getLine(char line[], int maxline);
void copy(char to[], char from[]);

int main(int argc, char *argv[]){
	int len;
	int max = 0;
	char line[MAXLINE];
	char longest[MAXLINE];
	FILE *fp;
	if (argc == 2){
		if((fp = fopen(argv[1], "r")) == NULL){
			printf("Can't open %s\n", argv[1]);
			return 1;
		}
		else{
			while (fgets(line, sizeof(line), fp)){
				int i;
				//A silly way to use i as a count from the first character to the end of the line
				for(i = 0; i<MAXLINE-1 && line[i]!=EOF && line[i]!='\n'; i++){}
				//We need to increment i one more time to store \n and \0
				line[i] = '\n';
				i++;
				line[i] = '\0';
				//If this is the longest line, max is now the count and we copy over the char line[] to char[] longest
				if(i > max){
					max = i;
					copy(longest, line);
				}
			}
			if (max > 0){
				printf("The longest phrase\n-------------------------\n%s", longest);
			}
		}
		fclose(fp);
	}
	else{
		printf("Keep entering phrases, and I will pick the longest one. Hit CTRL + D when you are finished.\n");
		while ((len = getLine(line, MAXLINE)) > 0){
			if (len > max){
				max = len;
				copy(longest, line);
			}
		}
		if (max > 0){
			printf("\nThe longest phrase\n-------------------------\n%s", longest);
		}
	}
	return 0;
}
int getLine(char line[], int lim){
	int character, i;
	//From first character to MAXLINE, End of File, or End Line Character, place each character from stdin into the line array.
	for(i = 0; (i<lim-1 && (character=getchar())!=EOF && character!='\n'); i++){
		line[i] = character;
	}
	if (character == '\n'){
		line[i] = character;
		i++;
	}
	line[i] = '\0';
	return i;
}
void copy(char to[], char from[]){
	int i = 0;
	while ((to[i] = from[i]) != '\0'){i++;}
}
