typedef struct regexstate RegexState;
struct regexstate {
  List* expr;
  int index;
  char c;
  char quant;
};

RegexState* newRegexSub(char* regex, int k);
RegexState* newRegex(char* regex);
