#include <ctype.h>
#include <i386/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define SUCCESS 0
#define FAILURE 1
#define MAX_INPUT_SIZE 1000
#define MAX_IDENT_SIZE 50
#define STR_(X) #X
#define STR(X) STR_(X)
typedef char bool;

enum {
  T_UNKNOWN = 0,
  T_INT,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_EOF,
  T_LPAR,
  T_RPAR,
  T_PRINT,
  T_SEMI
};

enum { A_INT = 1, A_ADD, A_SUB, A_MUL, A_DIV, A_EOF };

int operation_priority[] = {0, 0, 10, 10, 20, 20, 0};

static int op_priority(int tokentype) {
  int priority = operation_priority[tokentype];
  if (priority == 0) {
    printf("Syntax error\n");
    exit(1);
  }
  return priority;
}

struct token {
  int type;
  int value;
  char c;
	char * word;
};

struct ASTNode {
  int op;
  struct ASTNode *left;
  struct ASTNode *right;
  int value; // for int literals
};

// ARGS
bool file_input;
bool print_AST;
bool print_tokens;
bool debug;
bool interpret;
bool output_file_name_given;

// INPUT
char *file_name;
FILE *Input_file;
char *input;
int size;
int ch = 0;

// OUTPUT
char *output_file_name;
FILE *Output_file;

struct token *current_token;

// FUNCTIONS
struct ASTNode *binary_expression(int pp);
void rpar();

int read_input(char *filename) {
  Input_file = fopen(filename, "r");
  if (Input_file == NULL)
    return FAILURE;
  fseek(Input_file, 0L, SEEK_END);
  size = ftell(Input_file);
  rewind(Input_file);
  input = malloc(size * sizeof(char));
  fread(input, sizeof(char), size, Input_file);
  fclose(Input_file);
  return SUCCESS;
}

// LEXER

char *token_to_string(int token_type) {
  switch (token_type) {
  case T_EOF:
    return "T_EOF";
  case T_UNKNOWN:
    return "Unrecognized";
  case T_INT:
    return "T_INT";
  case T_PLUS:
    return "T_PLUS";
  case T_MINUS:
    return "T_MINUS";
  case T_STAR:
    return "T_STAR";
  case T_SLASH:
    return "T_SLASH";
  case T_LPAR:
    return "T_LPAR";
  case T_RPAR:
    return "T_RPAR";
case T_PRINT:
    return "T_PRINT";
case T_SEMI:
    return "T_SEMI";
  }

  return "unreachable";
}

char next() {
  if (ch > size)
    exit(FAILURE);
  char c = input[ch];
  ch++;
  return c;
}

void previous() {
  if (ch == 0)
    exit(FAILURE);
  ch--;
}

char skip() {
  char c = next();
  while (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r') {
    c = next();
  }
  return c;
}

int parse_int(char c) {
  int value = 0;
  while (isdigit(c)) {
    value = value * 10 + (c - '0');
    c = next();
  }
  previous();
  return value;
}

char *parse_identifier(char c) {
  char *ident = malloc(sizeof(char) * MAX_IDENT_SIZE);
  int i = 0;
  while (isalnum(c) || c == '_') {
    if (i >= MAX_IDENT_SIZE - 1) {
      fprintf(stderr, "ERROR: Max identifier size reached\n");
      exit(1);
    }
    ident[i] = c;
    c = next();
    ++i;
  }
  previous();
  ident[i] = '\0';
  return ident;
}

static int keyword(char *identifier) {
  switch (*identifier) {
  case 'p':
    if (strcmp(identifier, "print") == 0) {
      return T_PRINT;
    }
    break;
  }
  return 0;
}

int scan_token(struct token *token) {
  char c = skip();

  switch (c) {
  case '\0':
    token->type = T_EOF;
    break;
  case '+':
    token->type = T_PLUS;
    break;
  case '-':
    token->type = T_MINUS;
    break;
  case '*':
    token->type = T_STAR;
    break;
  case '/':
    token->type = T_SLASH;
    break;
  case '(':
    token->type = T_LPAR;
    break;
  case ')':
    token->type = T_RPAR;
    break;
case ';':
    token->type = T_SEMI;
    break;
  default:
    if (isdigit(c)) {
      int number = parse_int(c);
      token->type = T_INT;
      token->value = number;
      break;
    } else if (isalpha(c) || c == '_') {
      char *identifier = parse_identifier(c);
      token->type = keyword(identifier);
	token->word = identifier;
      if (token->type != 0)
        break;
    }
    token->type = T_UNKNOWN;
    token->value = 0;
    token->c = c;
    break;
  }
  if (print_tokens)
    printf("token : %s, value : %d\n", token_to_string(token->type),
           token->type == T_INT ? token->value : 0);
  return 1;
}

// PARSER

struct ASTNode *make_ast_node(int op, struct ASTNode *left,
                              struct ASTNode *right, int value) {
  struct ASTNode *node = malloc(sizeof(struct ASTNode));
  if (!node) {
    fprintf(stderr, "Unable to allocate memory for ASTNode.\n");
    exit(FAILURE);
  }
  node->op = op;
  node->left = left;
  node->right = right;
  node->value = value;
  return node;
}

struct ASTNode *make_ast_leaf(int op, int value) {
  return make_ast_node(op, NULL, NULL, value);
}

struct ASTNode *make_ast_unary(int op, struct ASTNode *child, int value) {
  return make_ast_node(op, child, NULL, value);
}

struct ASTNode *primary() {
  struct ASTNode *node;
  switch (current_token->type) {
  case T_INT:
    node = make_ast_leaf(A_INT, current_token->value);
    scan_token(current_token);
    return node;
  case T_LPAR:
    scan_token(current_token);
    node = binary_expression(0);
    rpar();
    return node;
  default:
    fprintf(stderr, "Syntax error on token : %s\n",
            token_to_string(current_token->type));
    exit(1);
  }
}

struct ASTNode *binary_expression(int pp) { // previous priority
  struct ASTNode *left, *right;
  left = primary();
  if (current_token->type == T_EOF || current_token->type == T_RPAR ||
      current_token->type == T_SEMI)
    return left;

  int token_type = current_token->type;
  while (op_priority(token_type) > pp) {
    scan_token(current_token);
    right = binary_expression(op_priority(token_type));
    left = make_ast_node(token_type, left, right, 0);

    token_type = current_token->type;
    if (token_type == T_EOF || current_token->type == T_RPAR ||
        token_type == T_SEMI) {
      return left;
    }
  }
  return left;
}

// AST

double interpret_AST(struct ASTNode *tree) {
  switch (tree->op) {
  case A_ADD:
    return interpret_AST(tree->left) + interpret_AST(tree->right);
  case A_SUB:
    return interpret_AST(tree->left) - interpret_AST(tree->right);
  case A_MUL:
    return interpret_AST(tree->left) * interpret_AST(tree->right);
  case A_DIV:
    return interpret_AST(tree->left) / interpret_AST(tree->right);
  case A_INT:
    return tree->value;
  }
  exit(1);
}

void free_ast(struct ASTNode *tree) {
  if (tree->left)
    free_ast(tree->left);
  if (tree->right)
    free_ast(tree->right);
  free(tree);
}

int levels[1000] = {0};
void print_ast(struct ASTNode *tree, int level) {
  int i;
  if (tree == NULL)
    return;

  for (i = 0; i < level; i++)
    if (i == level - 1)
      printf("%s───", levels[level - 1] ? "├" : "└");
    else
      printf("%s   ", levels[i] ? "│" : "  ");
  printf("%s ", token_to_string(tree->op));
  if (tree->op == A_INT)
    printf("%d", tree->value);
  printf("\n");
  levels[level] = 1;
  print_ast(tree->left, level + 1);
  levels[level] = 0;
  print_ast(tree->right, level + 1);
}

void parse_arguments(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "-file") == 0) {
      file_input = 1;
      file_name = argv[++i];
    }
    if (strcmp(argv[i], "-ast") == 0) {
      print_AST = 1;
    }
    if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-debug") == 0) {
      print_tokens = 1;
    }
    if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "-output") == 0) {
      output_file_name_given = 1;
      output_file_name = argv[++i];
    }
  }
}
// COMPILATION
u8 free_registers[4] = {1, 1, 1, 1};
char *registers[4] = {"r8", "r9", "r10", "r11"};

u8 allocate_register() {
  for (int i = 0; i < 4; i++) {
    if (free_registers[i] == 1) {
      free_registers[i] = 0;
      return i;
    }
  }
  printf("Ran out ouf registers.\n");
  exit(1);
}

void free_register(u8 reg) {
  if (free_registers[reg] == 1)
    exit(1);
  free_registers[reg] = 1;
}

void free_all() {
  for (int i = 0; i < 4; i++) {
    free_registers[i] = 1;
  };
}

u8 reg_add(u8 reg1, u8 reg2) {
  fprintf(Output_file, "\tadd %s, %s\n", registers[reg1], registers[reg2]);
  free_register(reg2);
  return reg1;
}

u8 reg_sub(u8 reg1, u8 reg2) {
  fprintf(Output_file, "\tsub %s, %s\n", registers[reg1], registers[reg2]);
  free_register(reg2);
  return reg1;
}

u8 reg_mul(u8 reg1, u8 reg2) {
  fprintf(Output_file, "\timul %s, %s\n", registers[reg1], registers[reg2]);
  free_register(reg2);
  return reg1;
}

u8 reg_div(u8 reg1, u8 reg2) {
  fprintf(Output_file,
          "\tmov rdx, 0\n"
          "\tmov rax, %s\n"
          "\tmov rcx, %s\n"
          "\tdiv rcx\n"
          "\tmov %s, rax\n",
          registers[reg1], registers[reg2], registers[reg1]);
  free_register(reg2);
  return reg1;
}

u8 reg_store(int value) {
  u8 reg = allocate_register();
  fprintf(Output_file, "\tmov %s, %d\n", registers[reg], value);
  return reg;
}

void print_reg(u8 reg) {
  fprintf(Output_file,
          "\tmov rdi, %s\n"
          "\tcall print\n",
          registers[reg]);
}

u8 compile_AST(struct ASTNode *tree) {
  u8 left_reg, right_reg;
  if (tree->left)
    left_reg = compile_AST(tree->left);
  if (tree->right)
    right_reg = compile_AST(tree->right);

  switch (tree->op) {
  case A_ADD:
    return reg_add(left_reg, right_reg);
  case A_SUB:
    return reg_sub(left_reg, right_reg);
  case A_MUL:
    return reg_mul(left_reg, right_reg);
  case A_DIV:
    return reg_div(left_reg, right_reg);
  case A_INT:
    return reg_store(tree->value);
  }
  exit(1);
}

void asm_header() {
  fprintf(Output_file, "global _main\n"
                       "section .text\n"
"extern _printf\n"
                       "string:\n"
                       "\tdb \"%%d\\n\"\n"
                       "print:\n"
                       "\tpush rbp\n"
                       "\tmov rbp, rsp\n"
                       "\tsub rsp, 16\n"
                       "\tmov [rbp-4], edi\n"
                       "\tmov [rbp-16], rsi\n"
                       "\tmov rsi, rdi\n"
                       "\tmov rdi, string\n"
                       "\tmov eax, 0\n"
                       "\tcall _printf\n"
                       "\tmov eax, 0\n"
                       "\tleave\n"
                       "\tret\n"
                       "_main:\n"
                       "\tpush rbp\n"
                       "\tmov rbp, rsp\n");
}

void asm_end() {
  fprintf(Output_file, "\tpop rbp\n"
                       "\tmov eax, 0\n"
                       "\tret\n");
}

// MISC
void match(int tokentype, char *name) {
  if (current_token->type == tokentype) {
    scan_token(current_token);
  } else {
    fprintf(stderr, "Expected token : %s, got : %d\n", name, current_token->type);
    exit(1);
  }
}

void semi() { match(T_SEMI, ";"); }

void rpar() { match(T_RPAR, ")"); }

void statements() {
  struct ASTNode *tree;
  u8 reg;
  while (1) {
    match(T_PRINT, "print");
    tree = binary_expression(0);
    reg = compile_AST(tree);
    free_all();
    semi();
    if (print_AST)
      print_ast(tree, 0);
	if(current_token->type == T_EOF) break;
  }
  free_ast(tree);
}

int main(int argc, char **argv) {

  if (!(current_token = malloc(sizeof(struct token))))
    return FAILURE;

  parse_arguments(argc, argv);
  if (file_input) {
    if (read_input(file_name) != SUCCESS)
      return FAILURE;
  } else {
    printf(">>> ");
    input = malloc(sizeof(char) * (MAX_INPUT_SIZE + 1));
    char c;
    scanf("%" STR(MAX_INPUT_SIZE) "[^\n]s", input);
    size = strlen(input);
  }

  scan_token(current_token);
  if (!output_file_name_given)
    output_file_name = "output.asm";
  Output_file = fopen(output_file_name, "wb");
  if (Output_file == NULL)
    return FAILURE;
  asm_header();
  statements();
  asm_end();
  // printf(" = %f\n", interpret_AST(tree));
  free(current_token);
  return EXIT_SUCCESS;
}
