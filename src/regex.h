typedef struct regexstate RegexState;
struct regexstate {
  List* expr;
  int index;
  char c;
  char quant;
};

RegexState* newRegex(char* regex);
void freeRegex(RegexState* restate);
