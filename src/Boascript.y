/* Boascript.y                  -*-YACC-*-                        */

//@PURPOSE: BoaScript -- a light, C-style scripting language and calculator.
//
//@CLASSES:
//  Boascript: parser and tree-walking interpreter for the language.
//
//@AUTHOR: Boris Rayskiy
//
//@SEE_ALSO: doc/reference.md
//
//@DESCRIPTION: This ooyacc grammar defines the BoaScript language together
// with its interpreter as a single self-contained C++ class, Boascript. A
// script is parsed into an AST which is then evaluated by ex().
//
// The language supports numbers (decimal, scientific, and 0x/0b/0o integer
// literals) and strings; single- and multi-character variables with plain
// and compound assignment; the usual arithmetic, comparison, logical, and
// bitwise operators with C-style precedence; control flow (if/else, while,
// for, case/when); user-defined functions with recursion and local scope;
// arrays, including nested / multi-dimensional ones; and a large set of math
// and string builtins. version()/about() report the build version and app
// description. See doc/reference.md for the full language reference.
//
///Usage:
//  Construct a Boascript and hand a script to run(); it returns the text the
//  script printed. The object is built with CALC_BATCH so output is captured
//  into a string rather than written to stdout.
//
//    #include <Boascript.tab.h>
//    using namespace BoriSoft;
//
//    Boascript bs;
//    std::string& out = bs.run("a = 2; b = 3; println sqrt(a * b);");
//    std::cout << out << std::endl;   // 2.4494897
//
//  run() is overloaded to also take and return a Column (a vector of script
//  strings / their outputs) for batch evaluation.

%{


#include <BTypes.h>
#include <DateTime.h>
#include <Tokenizer.h>
#include "Version.h"
//#include <BoascriptException.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <malloc.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>


#define CALC_BATCH


using namespace BoriSoft;


typedef Column::iterator ColIt;


enum Const
{
    SMALL_BUF_LEN = 32,
    MAX_STR_LEN   = 128,
    VAR_NAME_LEN  = SMALL_BUF_LEN + 1
};


// Main struct.
struct DataType
{
    enum typeEnum
    {
        typeDbl = 0,
        typeStr = 1,
        typeUnk = 2
    };

    typeEnum type;
    Double   dbl;
    char     str[MAX_STR_LEN + 1];
    
    friend std::ostream& operator << (std::ostream& os, const DataType& src)
    {
        os << "[ type = " << src.type;
        if (src.type == typeDbl)
        {
            os << ", dbl = " << src.dbl;
        }
        else if (src.type == typeStr)
        {
            os << ", str = " << src.str;
        }
        os << "]";
        return os;
    }
};


struct nodeType
{
    enum nodeEnum
    {
        typeCon, 
        typeId,
        typeVarCon,
        typeVar, 
        typeOpr 
    };

    // Constants.
    struct conNodeType
    {
        DataType value;
    };

    // Identifiers.
    struct idNodeType
    {
        int i; // Subscript to sym array.
    };
    
    struct varNodeType
    {
        char name[VAR_NAME_LEN];
    };
     
    // Operators.
    struct oprNodeType
    {
        int              oper;  // Operator.
        int              nops;  // Number of operands.
        struct nodeType* op[1]; // Operands (expandable).
    };

    nodeEnum type; // Type of node.
    // Union must be last entry in nodeType
    // because operNodeType may dynamically increase.
    union
    {
        conNodeType con; // Constants.
        idNodeType  id;  // Identifiers.
        varNodeType var;
        oprNodeType opr; // Operators.
    } u;
};


// A recursive array value: either a scalar number or a vector of nested
// values. This is what named arrays store, so arrays may be multi-
// dimensional (arrays of arrays).
struct ArrayVal
{
    bool                  isScalar;
    Double                num;
    std::vector<ArrayVal> arr;

    ArrayVal() : isScalar(true), num(0) {}
};


// Plain numeric matrix / vector, used by the matrix builtins.
typedef std::vector<std::vector<Double> > BMat;
typedef std::vector<Double>               BVec;


// A user-defined function: its parameter names and its body AST. The body
// node is owned by the parse arena (freed in Close()); the parameter names
// are kept here because AST nodes cannot hold C++ containers.
struct FuncDef
{
    std::vector<std::string> params;
    nodeType*                body;
};


// Thrown by a 'return' statement to unwind out of a function body; caught
// by the call site in ex().
struct BoaReturn
{
    DataType value;
};

%}


%union
{
    DataType  Value;
    char      sIndex;                // symbol table index
    char      varName[VAR_NAME_LEN]; // variable name
    nodeType* nPtr;                  // node pointer
    std::vector<std::string>* sList; // parameter-name list
    std::vector<nodeType*>*   nList; // argument-expression list
};


%token <Value> NUM STRING
%token <sIndex> VARIABLE
%token <varName> VARSTR
%token WHILE FOR IF PRINT PRINTLN IFN IFS
%token FUNC RETURN UCALL
%token CASE WHEN
%token INTGAUSS3
%token ARRAY_LIT ARRAY_GET ARRAY_SET ARRAY_MAX ARRAY_MIN
%token STRLEN SUBSTR REPLACE SUBSTITUTE TOSTR TONUM
%token ABS ACOS ASIN ATAN ATAN2 CEIL COS COSH EXP FABS FLOOR FMOD
%token FREXP LDEXP LOG LOG10 MODF POW SIN SINH SQRT CBRT TAN TANH
%token MIN MAX RAND DATE MATCH PI HYPOT UPPER LOWER
%token REVERSE FIND REPEAT CHARAT
%token SETPREC GETPREC
%token COMMENT EXIT
%nonassoc IFX
%nonassoc ELSE

%right PLUS_ASSIGN MINUS_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%right '?' ':' '!'
%left AND OR
%left '&' '|'
%left EQ NE
%left GE LE '>' '<'
%left SHR SHL
%left '+' '-'
%left '*' '/' '%'
%right '^'
%right PREF_INC PREF_DEC
%nonassoc UMINUS BNE

%type <nPtr>  stmt expr stmt_list funcdef for_init
%type <sList> params paramlist
%type <nList> args arglist arms elems elemlist indices

%%

program:
               function                 { return 0;                          }

function:
               function stmt            { execTop($2);                       }
               | /* NULL */
               ;

stmt:
               ';'                      { $$ = opr(';', 2, (nodeType*)0, (nodeType*)0);            }
               | COMMENT                { $$ = opr(COMMENT, 2, (nodeType*)0, (nodeType*)0);        }
               | expr ';'               { $$ = $1;                           }
               | PRINT expr ';'         { $$ = opr(PRINT, 1, $2);            }
               | PRINTLN expr ';'       { $$ = opr(PRINTLN, 1, $2);          }
               | VARIABLE '=' expr      { $$ = opr('=', 2, id($1), $3);      }
               | VARSTR '=' expr        { $$ = opr('=', 2, setVar($1), $3);  }
               | VARSTR '~' expr        { $$ = opr('~', 2, setVar($1), $3);  }
               | VARIABLE indices '=' expr
                     { $$ = arraySet(varOf($1), $2, $4);                     }
               | VARSTR indices '=' expr
                     { $$ = arraySet(std::string($1), $2, $4);               }
               | WHILE '(' expr ')' stmt   { $$ = opr(WHILE, 2, $3, $5);     }
               | FOR '(' for_init ';' expr ';' for_init ')' stmt
                                        { $$ = opr(FOR, 4, $3, $5, $7, $9);  }
               | RETURN expr ';'        { $$ = opr(RETURN, 1, $2);           }
               | CASE '(' expr ')' '{' arms '}'
                                        { $$ = caseStmt($3, $6);             }
               | funcdef                { $$ = $1;                           }
               | IF '(' expr ')' stmt %prec IFX 
                                        { $$ = opr(IF, 2, $3, $5);           }
               | IF '(' expr ')' stmt ELSE stmt
                                        { $$ = opr(IF, 3, $3, $5, $7);       }
               | '{' stmt_list '}'      { $$ = $2;                           }
               | EXIT                   { exit(0);                           }
               ;

stmt_list:
               stmt                     { $$ = $1;                           }
               | stmt_list stmt         { $$ = opr(';', 2, $1, $2);          }
               ;

// A user function definition. It is registered in the funcs table at parse
// time; the reduction yields a no-op statement so nothing runs until the
// function is called.
funcdef:
               FUNC VARSTR '(' params ')' stmt
                     { defFunc($2, $4, $6); $$ = opr(';', 2, (nodeType*)0, (nodeType*)0);          }
               | FUNC VARIABLE '(' params ')' stmt
                     { defFunc(varOf($2), $4, $6); $$ = opr(';', 2, (nodeType*)0, (nodeType*)0);   }
               ;

params:
               /* empty */              { $$ = new std::vector<std::string>(); }
               | paramlist              { $$ = $1;                           }
               ;

paramlist:
               VARSTR                   { $$ = new std::vector<std::string>();
                                          $$->push_back(std::string($1));    }
               | VARIABLE               { $$ = new std::vector<std::string>();
                                          $$->push_back(varOf($1));          }
               | paramlist ',' VARSTR   { $1->push_back(std::string($3));
                                          $$ = $1;                           }
               | paramlist ',' VARIABLE { $1->push_back(varOf($3));
                                          $$ = $1;                           }
               ;

// The init and post clauses of a for-loop: an assignment or a bare
// expression (e.g. i += 1).
for_init:
               VARIABLE '=' expr        { $$ = opr('=', 2, id($1), $3);      }
               | VARSTR '=' expr        { $$ = opr('=', 2, setVar($1), $3);  }
               | expr                   { $$ = $1;                           }
               ;

args:
               /* empty */              { $$ = new std::vector<nodeType*>(); }
               | arglist                { $$ = $1;                           }
               ;

arglist:
               expr                     { $$ = new std::vector<nodeType*>();
                                          $$->push_back($1);                 }
               | arglist ',' expr       { $1->push_back($3); $$ = $1;        }
               ;

// The elements of an array literal [ e1, e2, ... ] (possibly empty).
elems:
               /* empty */              { $$ = new std::vector<nodeType*>(); }
               | elemlist               { $$ = $1;                           }
               ;

elemlist:
               expr                     { $$ = new std::vector<nodeType*>();
                                          $$->push_back($1);                 }
               | elemlist ',' expr      { $1->push_back($3); $$ = $1;        }
               ;

// An index chain: [i], [i][j], [i][j][k], ...
indices:
               '[' expr ']'             { $$ = new std::vector<nodeType*>();
                                          $$->push_back($2);                 }
               | indices '[' expr ']'   { $1->push_back($3); $$ = $1;        }
               ;

// The arms of a case statement, flattened into (value, body) pairs. The
// default arm ("else:") is stored with a null value.
arms:
               /* empty */              { $$ = new std::vector<nodeType*>(); }
               | arms WHEN expr ':' stmt_list
                                        { $1->push_back($3);
                                          $1->push_back($5); $$ = $1;        }
               | arms ELSE ':' stmt_list
                                        { $1->push_back(0);
                                          $1->push_back($4); $$ = $1;        }
               ;

expr:
               NUM                      { $$ = conD($1);                     }
               | STRING                 { $$ = conS($1);                     }
               | VARIABLE               { $$ = id($1);                       }
               | VARSTR                 { $$ = setVar($1);                   }
               | VARSTR '(' args ')'    { $$ = callFunc(std::string($1), $3);}
               | VARIABLE '(' args ')'  { $$ = callFunc(varOf($1), $3);      }
               | '[' elems ']'          { $$ = arrayLit($2);                }
               | VARIABLE indices       { $$ = arrayGet(varOf($1), $2);     }
               | VARSTR indices         { $$ = arrayGet(std::string($1), $2); }
               | MAX  '(' expr ')'      { $$ = opr(ARRAY_MAX, 1, $3);       }
               | MIN  '(' expr ')'      { $$ = opr(ARRAY_MIN, 1, $3);       }
               | INTGAUSS3 '(' VARSTR ',' expr ',' expr ')'
                     { $$ = opr(INTGAUSS3, 3, setVar(std::string($3)), $5, $7); }
               | INTGAUSS3 '(' VARIABLE ',' expr ',' expr ')'
                     { $$ = opr(INTGAUSS3, 3, setVar(varOf($3)), $5, $7);       }
               | '(' expr ')'           { $$ = $2;                           }
               | '-' expr %prec UMINUS  { $$ = opr(UMINUS, 1, $2);           }
               | expr '+' expr          { $$ = opr('+', 2, $1, $3);          }
               | expr '-' expr          { $$ = opr('-', 2, $1, $3);          }
               | expr '*' expr          { $$ = opr('*', 2, $1, $3);          }
               | expr '/' expr          { $$ = opr('/', 2, $1, $3);          }
               | expr '%' expr          { $$ = opr('%', 2, $1, $3);          }
               | expr PLUS_ASSIGN expr  { $$ = opr(PLUS_ASSIGN,  2, $1, $3); }
               | expr MINUS_ASSIGN expr { $$ = opr(MINUS_ASSIGN, 2, $1, $3); }
               | expr MUL_ASSIGN expr   { $$ = opr(MUL_ASSIGN,   2, $1, $3); }
               | expr DIV_ASSIGN expr   { $$ = opr(DIV_ASSIGN,   2, $1, $3); }
               | expr MOD_ASSIGN expr   { $$ = opr(MOD_ASSIGN,   2, $1, $3); }
               | PREF_INC expr          { $$ = opr(PREF_INC, 1, $2);         }
               | PREF_DEC expr          { $$ = opr(PREF_DEC, 1, $2);         }
               | expr '<' expr          { $$ = opr('<', 2, $1, $3);          }
               | expr '>' expr          { $$ = opr('>', 2, $1, $3);          }
               | expr GE  expr          { $$ = opr(GE , 2, $1, $3);          }
               | expr LE  expr          { $$ = opr(LE , 2, $1, $3);          }
               | expr NE  expr          { $$ = opr(NE , 2, $1, $3);          }
               | expr EQ  expr          { $$ = opr(EQ , 2, $1, $3);          }
               | expr AND expr          { $$ = opr(AND, 2, $1, $3);          }
               | expr OR  expr          { $$ = opr(OR , 2, $1, $3);          }
               | expr '&' expr          { $$ = opr('&', 2, $1, $3);          }
               | expr '|' expr          { $$ = opr('|', 2, $1, $3);          }
               | '!' expr %prec BNE     { $$ = opr(BNE, 1, $2);              }
               | expr SHR  expr         { $$ = opr(SHR, 2, $1, $3);          }
               | expr SHL  expr         { $$ = opr(SHL, 2, $1, $3);          }
               | ABS   '(' expr ')'     { $$ = opr(ABS , 1, $3);             }
               | ACOS  '(' expr ')'     { $$ = opr(ACOS, 1, $3);             }
               | ASIN  '(' expr ')'     { $$ = opr(ASIN, 1, $3);             }
               | ATAN  '(' expr ')'     { $$ = opr(ATAN, 1, $3);             }
               | ATAN2 '(' expr ',' expr ')' { $$ = opr(ATAN2, 2, $3, $5);   }
               | CEIL  '(' expr ')'     { $$ = opr(CEIL, 1, $3);             }
               | COS   '(' expr ')'     { $$ = opr(COS , 1, $3);             }
               | COSH  '(' expr ')'     { $$ = opr(COSH, 1, $3);             }
               | EXP   '(' expr ')'     { $$ = opr(EXP , 1, $3);             }
               | FABS  '(' expr ')'     { $$ = opr(FABS, 1, $3);             }
               | FLOOR '(' expr ')'     { $$ = opr(FLOOR,1, $3);             }
               | FMOD  '(' expr ',' expr ')' { $$ = opr(FMOD, 2, $3, $5);    }
               | HYPOT '(' expr ',' expr ')' { $$ = opr(HYPOT, 2, $3, $5);   }
               | UPPER '(' expr ')'     { $$ = opr(UPPER, 1, $3);            }
               | LOWER '(' expr ')'     { $$ = opr(LOWER, 1, $3);            }
               | REVERSE '(' expr ')'   { $$ = opr(REVERSE, 1, $3);          }
               | FIND   '(' expr ',' expr ')' { $$ = opr(FIND, 2, $3, $5);   }
               | REPEAT '(' expr ',' expr ')' { $$ = opr(REPEAT, 2, $3, $5); }
               | CHARAT '(' expr ',' expr ')' { $$ = opr(CHARAT, 2, $3, $5); }
               | FREXP '(' expr ',' expr ')' { $$ = opr(FREXP,2, $3, $5);    }
               | LDEXP '(' expr ',' expr ')' { $$ = opr(LDEXP,2, $3, $5);    }
               | LOG   '(' expr ')'     { $$ = opr(LOG , 1, $3);             }
               | LOG10 '(' expr ')'     { $$ = opr(LOG10,1,$3);              }
               | MODF  '(' expr ',' expr ')' { $$ = opr(MODF,2, $3, $5);     }
               | POW   '(' expr ',' expr ')' { $$ = opr(POW ,2, $3, $5);     }
               | expr '^' expr          { $$ = opr(POW ,2, $1, $3);          }
               | SIN   '(' expr ')'     { $$ = opr(SIN , 1, $3);             }
               | SINH  '(' expr ')'     { $$ = opr(SINH, 1, $3);             }
               | SQRT  '(' expr ')'     { $$ = opr(SQRT, 1, $3);             }
               | CBRT  '(' expr ')'     { $$ = opr(CBRT, 1, $3);             }
               | TAN   '(' expr ')'     { $$ = opr(TAN , 1, $3);             }
               | TANH  '(' expr ')'     { $$ = opr(TANH, 1, $3);             }
               | RAND  '(' ')'          { $$ = opr(RAND, 0, 0);              }
               | MIN   '(' expr ',' expr ')' { $$ = opr(MIN, 2, $3, $5);     }
               | MAX   '(' expr ',' expr ')' { $$ = opr(MAX, 2, $3, $5);     }
               | expr '?' expr ':' expr      { $$ = opr('?', 3, $1, $3, $5); }
               | TONUM  '(' expr ')'    { $$ = opr(TONUM,  1, $3);           }
               | STRLEN '(' expr ')'    { $$ = opr(STRLEN, 1, $3);           }
               | TOSTR  '(' expr ')'    { $$ = opr(TOSTR, 1, $3);            }
               | SUBSTR '(' expr ',' expr ',' expr  ')'    
                     { $$ = opr(SUBSTR,    3, $3, $5, $7);                   }
               | REPLACE '(' expr ',' expr ',' expr ',' expr ')' 
                     { $$ = opr(REPLACE,   4, $3, $5, $7, $9);               }
               | SUBSTITUTE '(' expr ',' expr ',' expr ')' 
                     { $$ = opr(SUBSTITUTE,3, $3, $5, $7);                   }
               | MATCH '(' expr ',' expr ')' 
                     { $$ = opr(MATCH,     2, $3, $5);                       }
               | DATE '(' expr ')'      { $$ = opr(DATE, 1, $3);             }
               // For compatibility
               | IFN '(' expr ',', expr ',' expr ')' 
                     { $$ = opr(IFN, 3, $3, $5, $7);                         }
               | IFS '(' expr ',', expr ',' expr ')' 
                     { $$ = opr(IFN, 3, $3, $5, $7);                         }
               | SETPREC  '(' expr ')'  { $$ = opr(SETPREC, 1, $3);          }
               | GETPREC  '(' ')'       { $$ = opr(GETPREC, 0, 0);           }
               | PI '(' ')'             { $$ = opr(PI, 0, 0);                }
               ;

%%


#define SIZEOF_NODETYPE ((char *)&p->u.con - (char *)p)


nodeType* conD(DataType value)
{
    nodeType* p = 0;

    // Allocate node.
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::conNodeType);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("Out of memory");
    }

    // Copy information.
    p->type             = nodeType::typeCon;
    p->u.con.value.dbl  = value.dbl;
    p->u.con.value.type = DataType::typeDbl;

    m_nodes.push_back(p);
    return p;
}


nodeType* conS(DataType value)
{
    nodeType* p = 0;
    // Allocate node.
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::conNodeType);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("Out of memory");
    }

    // Copy information.
    p->type = nodeType::typeCon;
    int len = (strlen(value.str) > MAX_STR_LEN) ? MAX_STR_LEN : 
                                                  strlen(value.str);
    memset(p->u.con.value.str, 0, MAX_STR_LEN + 1);
    memmove(p->u.con.value.str, value.str, len);
    p->u.con.value.type =  DataType::typeStr;

    m_nodes.push_back(p);
    return p;
}


nodeType* setVar(std::string name)
{
#ifdef VARSTR_DEBUG
		std::cout << "setVar name = " << name << std::endl; 
#endif
    nodeType* p = 0;
    // Allocate node.
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::varNodeType);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("out of memory");
    }

    // copy information.
    p->type = nodeType::typeVar;
    size_t nameLen = (name.size() >= VAR_NAME_LEN) ? (VAR_NAME_LEN - 1)
                                                   : name.size();
    ::memset(&p->u.var.name[0], 0, VAR_NAME_LEN);
    ::memmove(&p->u.var.name[0], name.c_str(), nameLen);

#ifdef VARSTR_DEBUG
		std::cout << "setVar p->u.var.name = " << p->u.var.name << std::endl;
#endif
    m_nodes.push_back(p);
    return p;
}


nodeType* id(int i)
{
    nodeType* p = 0;
    // Allocate node.
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::idNodeType);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("out of memory");
    }

    // copy information.
    p->type   = nodeType::typeId;
    p->u.id.i = i;

    m_nodes.push_back(p);
    return p;
}


nodeType* opr(int oper, int nops, ...)
{
    nodeType* p = 0;
    // Allocate node.
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::oprNodeType) +
               (nops - 1) * sizeof(nodeType*);

    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("out of memory");
    }

    // Copy information.
    p->type       = nodeType::typeOpr;
    p->u.opr.oper = oper;
    p->u.opr.nops = nops;

    va_list ap;
    va_start(ap, nops);
    for (int i = 0; i < nops; i++)
    {
        p->u.opr.op[i] = va_arg(ap, nodeType*);
    }
    va_end(ap);

    m_nodes.push_back(p);
    return p;
}


// The name of a single-letter variable, given its sym[] index.
std::string varOf(int i)
{
    return std::string(1, (char)('a' + i));
}


// Register a user function. The parameter-name list is consumed (deleted);
// the body node stays owned by the parse arena.
void defFunc(std::string name, std::vector<std::string>* prms, nodeType* body)
{
    FuncDef fn;
    fn.params = *prms;
    fn.body   = body;
    funcs[name] = fn;
    delete prms;
}


// Build a user-function call node: op[0] carries the function name and
// op[1..] the argument expressions. The argument list is consumed.
nodeType* callFunc(std::string name, std::vector<nodeType*>* argv)
{
    int nops = 1 + (int)argv->size();

    nodeType* p = 0;
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::oprNodeType) +
                      (nops - 1) * sizeof(nodeType*);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("out of memory");
    }

    p->type       = nodeType::typeOpr;
    p->u.opr.oper = UCALL;
    p->u.opr.nops = nops;
    p->u.opr.op[0] = setVar(name);
    for (int i = 0; i < (int)argv->size(); ++i)
    {
        p->u.opr.op[i + 1] = (*argv)[i];
    }

    delete argv;
    m_nodes.push_back(p);
    return p;
}


// Build a case statement node: op[0] is the switch value, followed by the
// (value, body) pairs from the arms list (a null value marks the default
// "else:" arm). The arms list is consumed.
nodeType* caseStmt(nodeType* sw, std::vector<nodeType*>* arms)
{
    int nops = 1 + (int)arms->size();

    nodeType* p = 0;
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::oprNodeType) +
                      (nops - 1) * sizeof(nodeType*);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("out of memory");
    }

    p->type        = nodeType::typeOpr;
    p->u.opr.oper  = CASE;
    p->u.opr.nops  = nops;
    p->u.opr.op[0] = sw;
    for (int i = 0; i < (int)arms->size(); ++i)
    {
        p->u.opr.op[i + 1] = (*arms)[i];
    }

    delete arms;
    m_nodes.push_back(p);
    return p;
}


// Build an operator node whose operands are taken from a vector (which is
// consumed), optionally preceded by a single leading operand. Used for the
// variable-arity array nodes.
nodeType* oprVec(int oper, nodeType* lead, std::vector<nodeType*>* items)
{
    int extra = (lead != 0) ? 1 : 0;
    int nops  = extra + (int)items->size();

    nodeType* p = 0;
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::oprNodeType) +
                      ((nops > 0 ? nops : 1) - 1) * sizeof(nodeType*);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("out of memory");
    }

    p->type       = nodeType::typeOpr;
    p->u.opr.oper = oper;
    p->u.opr.nops = nops;
    if (lead)
    {
        p->u.opr.op[0] = lead;
    }
    for (int i = 0; i < (int)items->size(); ++i)
    {
        p->u.opr.op[extra + i] = (*items)[i];
    }

    delete items;
    m_nodes.push_back(p);
    return p;
}


// An (anonymous) array literal: op[0..] are the element expressions.
nodeType* arrayLit(std::vector<nodeType*>* elems)
{
    return oprVec(ARRAY_LIT, 0, elems);
}


// Array element access: op[0] is the array name, op[1..] the index chain.
nodeType* arrayGet(std::string name, std::vector<nodeType*>* idx)
{
    return oprVec(ARRAY_GET, setVar(name), idx);
}


// Array element assignment: op[0] name, op[1] value, op[2..] the index chain.
nodeType* arraySet(std::string name, std::vector<nodeType*>* idx, nodeType* val)
{
    idx->insert(idx->begin(), val);
    return oprVec(ARRAY_SET, setVar(name), idx);
}


void yyerror(std::string msg)
{
#ifndef CALC_BATCH
    std::cout << msg << std::endl;
#else
    //THROW_BOA_SCRIPT_EXCEPTION(msg);
    m_outBuf += msg;
#endif
}


void yyerror(std::string tag, std::string msg)
{
#ifndef CALC_BATCH
    std::cout << tag << msg << std::endl;
#else
    //THROW_BOA_SCRIPT_EXCEPTION(tag + msg);
    m_outBuf += tag;
    m_outBuf += " ";
    m_outBuf += msg;
#endif
}


// *****************************************************************************
// ******************************** Lexer **************************************
// *****************************************************************************

public:


#ifdef CALC_BATCH


std::string& run(const std::string in)
{
    Init();
    Load(in);
    yyparse();
    Close();
    return GetRes();
}


Column run(Column& in)
{
    Column outStr;
    for (ColIt it = in.begin(); it != in.end(); ++it)
    {
        Init();
        Load(*it);
        yyparse();
        Close();
        outStr.push_back(GetRes());
    }
    return outStr;
}


void run(const Column& in, Column& out)
{
    for (size_t i = 0; i < out.size(); ++i)
    {
        if (i < in.size())
        {
            Init();
            Load(in[i]);
            yyparse();
            Close();
            out[i] = GetRes();
        }
        else
        {
            out[i] = "(empty)";
        }
    }
}


// ---- Interactive session API ---------------------------------------------
//
// run() resets every bit of state on each call. A session instead keeps
// variables, functions, arrays, and precision across runLine() calls, so an
// interactive interpreter (a REPL) can build up state incrementally. Call
// beginSession() once, runLine() per line, and endSession() when finished.

void beginSession(void)
{
    Init();
    // Give single-letter variables a defined initial value so that reading an
    // as-yet-unassigned one yields 0 rather than an indeterminate value.
    for (int i = 0; i <= 'z' - 'a'; ++i)
    {
        sym[i].type = DataType::typeDbl;
        sym[i].dbl  = (Double)0;
    }
}

std::string& runLine(std::string in)
{
    // Reset only the per-line parser state and the output buffer; variables,
    // functions, arrays, precision, and the accumulated node arena carry over
    // so definitions from earlier lines stay in effect.
    yynerrs   = 0;
    yyerrflag = 0;
    yychar    = (-1);
    yyssp     = yyss;
    yyvsp     = yyvs;
    *yyssp    = yystate = 0;
    m_scopes.clear();
    m_outBuf.clear();

    Load(in);
    yyparse();
    return GetRes();
}

void endSession(void)
{
    Close();   // free the input buffer and the accumulated node arena
}


~Boascript()
{
    //Close();
}


void Init(void)
{
    // Init internal infrastructure as same 
    // as in constructor.
    yynerrs   = 0;
    yyerrflag = 0;
    yychar    = (-1);

    yyssp     = yyss;
    yyvsp     = yyvs;
    *yyssp    = yystate = 0;

    m_bufInd    = 0;
    m_inLen     = 0;
    m_inBuf     = 0;

    // Release any nodes left over from a previous parse (e.g. if Close()
    // was not called after a syntax error).
    FreeNodes();

    // Functions reference arena nodes freed by FreeNodes(); drop them and
    // any leftover call frames before the next parse.
    funcs.clear();
    arrays.clear();
    m_scopes.clear();

    // Clear any output left over from a previous run() on this object.
    m_outBuf.clear();

    precision = 7;
    m_outColor.clear();
}


void Load(std::string in)
{
    int len = in.size() + 1;

    // Always (re)allocate so the buffer matches the current input size;
    // reusing an older, smaller buffer could overflow.
    if (m_inBuf)
    {
        free(m_inBuf);
        m_inBuf = 0;
    }
    m_inBuf = (char *)malloc(len);

    if (m_inBuf)
    {
        memset(m_inBuf, 0, len);
        memmove(m_inBuf, in.c_str(), len - 1);
    }

    m_inLen  = in.size();
    m_bufInd = 0;
}


std::string& GetRes(void)
{
    return m_outBuf;
}


// The output color requested by the script via the color() builtin, as the
// raw name passed ("" means default / off). A session persists it across
// runLine() calls; an interactive front-end may read it after each line to
// tint subsequent output. Headless callers can ignore it.
const std::string& outputColor(void) const
{
    return m_outColor;
}


void Close(void)
{
    if (m_inBuf)
    {
        free(m_inBuf);
        m_inBuf = 0;
    }

    FreeNodes();
}


// Release every AST node allocated during the last parse.
void FreeNodes(void)
{
    for (size_t k = 0; k < m_nodes.size(); ++k)
    {
        free(m_nodes[k]);
    }
    m_nodes.clear();
}


#endif // CALC_BATCH


private:


int GetChar(void)
{

#ifndef CALC_BATCH
    return getchar();
#else
    // Do not read past the end of the input buffer. Return the
    // end-of-input sentinel (0) once the terminator is reached and
    // leave m_bufInd pointing at it so repeated calls stay in bounds.
    if ((m_inBuf == 0) || (m_bufInd >= m_inLen))
    {
        return 0;
    }
    return *(m_inBuf + m_bufInd++);
#endif

}


Double ScanNum(void)
{
#ifndef CALC_BATCH
    double tmp;
    scanf("%lf", &tmp);
    return tmp;
#else
    std::string tmp = "";
    char prev = '\0';
    while((*(m_inBuf + m_bufInd) == '.') || 
          (*(m_inBuf + m_bufInd) == 'e') || 
          (*(m_inBuf + m_bufInd) == 'E') ||
          ((*(m_inBuf + m_bufInd) == '-') && ((prev == 'e') || (prev == 'E'))) ||
          ::isdigit(*(m_inBuf + m_bufInd)))
    {
        prev = *(m_inBuf + m_bufInd);
        tmp += *(m_inBuf + m_bufInd++);
    }
    std::istringstream out(tmp);
    Double t;
    out >> t;
    return t;
#endif // CALC_BATCH
}


// Read an integer literal in the given base (2, 8 or 16), consuming digits
// valid for that base. The "0x"/"0b"/"0o" prefix has already been read.
long ScanBasedInt(int base)
{
    long value = 0;
    for (;;)
    {
        int c = GetChar();
        int digit;
        if ((c >= '0') && (c <= '9'))      digit = c - '0';
        else if ((c >= 'a') && (c <= 'f')) digit = c - 'a' + 10;
        else if ((c >= 'A') && (c <= 'F')) digit = c - 'A' + 10;
        else                               { UngetChar(c); break; }

        if (digit >= base)
        {
            UngetChar(c);
            break;
        }
        value = value * base + digit;
    }
    return value;
}


void UngetChar(int c)
{
#ifndef CALC_BATCH
    ungetc(c, stdin);
#else
    // Ungetting the end-of-input sentinel is a no-op: GetChar() does not
    // advance past the terminator, so there is nothing to put back.
    if ((c != 0) && (c != EOF) && (m_bufInd > 0))
    {
        --m_bufInd;
    }
#endif // CALC_BATCH
}


int yylex(void)
{
    int cp;

    // Ignore whitespaces, tabs and new lines.
    // Get first nonwhite character.
    while (((cp = GetChar()) == ' ') || (cp == '\t') || (cp == '\n'));

    // Ignore comments.
    if (cp == '/')
    {
        int cn;
        if ((cn = GetChar()) == '/')
        {
            while (((cp = GetChar()) != '\n') && (cp != '\0') && (cp != EOF));
            return COMMENT;
        }
        else
        {
            UngetChar(cn);
        }
    }

    if ((cp == EOF) || (cp == 0))
    {
       	return 0;
    }

    // String section.
    if (cp == '"')
    {
        memset(yylval.Value.str, 0, MAX_STR_LEN + 1);
        int n = 0;

        while (((cp = GetChar()) != '"') && (cp != 0) && (cp != EOF))
        {
            if (n < MAX_STR_LEN)
            {
                yylval.Value.str[n++] = cp;
            }
        }
#ifdef CALC_DEBUG
        printf("%s\n", yylval.Value.str);
#endif
        yylval.Value.type = DataType::typeStr;
        return STRING;
    }

    // Char starts a number => parse the number.
    if (cp == '.' || ::isdigit(cp))
    {
        // A leading '0' may introduce a based integer literal:
        // 0x.. (hex), 0b.. (binary), 0o.. (octal).
        if (cp == '0')
        {
            int c2 = GetChar();
            int base = 0;
            if ((c2 == 'x') || (c2 == 'X')) base = 16;
            else if ((c2 == 'b') || (c2 == 'B')) base = 2;
            else if ((c2 == 'o') || (c2 == 'O')) base = 8;

            if (base != 0)
            {
                yylval.Value.dbl  = (Double)ScanBasedInt(base);
                yylval.Value.type = DataType::typeDbl;
                return NUM;
            }
            UngetChar(c2);
        }

        UngetChar(cp);
        yylval.Value.dbl = ScanNum();
        yylval.Value.type = DataType::typeDbl;
#ifdef CALC_DEBUG
        std::cout << yylval.Value.dbl << std::endl;
#endif
        return NUM;
    }

    // Char starts an identifier => read the name.
    if (::isalpha(cp))
    {
        char buf[SMALL_BUF_LEN + 1];
        memset(buf, 0, SMALL_BUF_LEN + 1);
        int n = 0;
	    int i = 0;
        while ((cp != EOF) && (cp != 0) && (::isalpha(cp) || ::isdigit(cp)))
	    {
            if (n < SMALL_BUF_LEN)
            {
                buf[n++] = cp;
            }
            ++i;
            cp = GetChar();
        }
        UngetChar(cp);

        // Single-letter variables map to sym[buf[0] - 'a']; only 'a'..'z'
        // are valid indices, so reject anything else instead of indexing
        // out of bounds.
        if ((i == 1) && (buf[0] >= 'a') && (buf[0] <= 'z'))
        {
             yylval.sIndex = buf[0] - 'a';
            
#ifdef CALC_DEBUG
             printf("%c\n", buf[0]);
#endif
             return VARIABLE;
        }
        else
        {

#ifdef CALC_DEBUG
            printf("%s\n", buf);
#endif
            if ((strcmp(buf, "true") == 0) || (strcmp(buf, "false") == 0))
            {
                // Boolean literals are just the numbers 1 and 0.
                yylval.Value.dbl  = (strcmp(buf, "true") == 0) ? 1.0 : 0.0;
                yylval.Value.type = DataType::typeDbl;
                return NUM;
            }
            else if (strcmp(buf, "print") == 0)
            {
                return PRINT;
            }
            else if (strcmp(buf, "println") == 0)
            {
                return PRINTLN;
            }
            else if (strcmp(buf, "if") == 0)
            {
                return IF;
            }
            else if (strcmp(buf, "while") == 0)
            {
                return WHILE;
            }
            else if (strcmp(buf, "for") == 0)
            {
                return FOR;
            }
            else if (strcmp(buf, "func") == 0)
            {
                return FUNC;
            }
            else if (strcmp(buf, "return") == 0)
            {
                return RETURN;
            }
            else if (strcmp(buf, "intgauss3") == 0)
            {
                return INTGAUSS3;
            }
            else if (strcmp(buf, "case") == 0)
            {
                return CASE;
            }
            else if (strcmp(buf, "when") == 0)
            {
                return WHEN;
            }
            else if (strcmp(buf, "else") == 0)
            {
                return ELSE;
            }
            else if (strcmp(buf, "abs") == 0)
            {
                return ABS;
            }
            else if (strcmp(buf, "acos") == 0)
            {
                return ACOS;
            }
            else if (strcmp(buf, "asin") == 0)
            {
                return ASIN;
            }
            else if (strcmp(buf, "atan") == 0)
            {
                return ATAN;
            }
            else if (strcmp(buf, "atan2") == 0)
            {
                return ATAN2;
            }
            else if (strcmp(buf, "ceil") == 0)
            {
                return CEIL;
            }
            else if (strcmp(buf, "cos") == 0)
            {
                return COS;
            }
            else if (strcmp(buf, "cosh") == 0)
            {
                return COSH;
            }
            else if (strcmp(buf, "exp") == 0)
            {
                return EXP;
            }
            else if (strcmp(buf, "fabs") == 0)
            {
                return FABS;
            }
            else if (strcmp(buf, "floor") == 0)
            {
                return FLOOR;
            }
            else if (strcmp(buf, "fmod") == 0)
            {
                return FMOD;
            }
            else if (strcmp(buf, "frexp") == 0)
            {
                return FREXP;
            }
            else if (strcmp(buf, "ldexp") == 0)
            {
                return LDEXP;
            }
            else if (strcmp(buf, "log") == 0)
            {
                return LOG;
            }
            else if (strcmp(buf, "log10") == 0)
            {
                return LOG10;
            }
            else if (strcmp(buf, "modf") == 0)
            {
                return MODF;
            }
            else if (strcmp(buf, "pow") == 0)
            {
                return POW;
            }
            else if (strcmp(buf, "hypot") == 0)
            {
                return HYPOT;
            }
            else if (strcmp(buf, "upper") == 0)
            {
                return UPPER;
            }
            else if (strcmp(buf, "lower") == 0)
            {
                return LOWER;
            }
            else if (strcmp(buf, "reverse") == 0)
            {
                return REVERSE;
            }
            else if (strcmp(buf, "find") == 0)
            {
                return FIND;
            }
            else if (strcmp(buf, "repeat") == 0)
            {
                return REPEAT;
            }
            else if (strcmp(buf, "charat") == 0)
            {
                return CHARAT;
            }
            else if (strcmp(buf, "sin") == 0)
            {
                return SIN;
            }
            else if (strcmp(buf, "sinh") == 0)
            {
                return SINH;
            }
            else if (strcmp(buf, "sqrt") == 0)
            {
                return SQRT;
            }
            else if (strcmp(buf, "cbrt") == 0)
            {
                return CBRT;
            }
            else if (strcmp(buf, "tan") == 0)
            {
                return TAN;
            }
            else if (strcmp(buf, "tanh") == 0)
            {
                return TANH;
            }
            else if (strcmp(buf, "rand") == 0)
            {
                return RAND;
            }
            else if (strcmp(buf, "min") == 0)
            {
                return MIN;
            }
            else if (strcmp(buf, "max") == 0)
            {
                return MAX;
            }
            // String functions.
            else if (strcmp(buf, "strlen") == 0)
            {
                return STRLEN;
            }
            else if (strcmp(buf, "substr") == 0)
            {
                return SUBSTR;
            }
            else if (strcmp(buf, "replace") == 0)
            {
                return REPLACE;
            }
            else if (strcmp(buf, "substitute") == 0)
            {
                return SUBSTITUTE;
            }
            else if (strcmp(buf, "match") == 0)
            {
                return MATCH;
            }
            else if (strcmp(buf, "exit") == 0)
            {
                return EXIT;
            }
            else if (strcmp(buf, "tostr") == 0)
            {
                return TOSTR;
            }
            else if (strcmp(buf, "tonum") == 0)
            {
                return TONUM;
            }
            else if (strcmp(buf, "date") == 0)
            {
                return DATE;
            }
            // For compatibility with previous versions.
            else if (strcmp(buf, "ifn") == 0)
            {
                return IFN;
            }
            else if (strcmp(buf, "ifs") == 0)
            {
                return IFS;
            }
            else if (strcmp(buf, "setprec") == 0)
            {
                return SETPREC;
            }
            else if (strcmp(buf, "getprec") == 0)
            {
                return GETPREC;
            }
            else if (strcmp(buf, "pi") == 0)
            {
                return PI;
            }
            else
            {
				//::memset(&yylval.varName, 0, VAR_NAME_LEN);
				//::memmove(&yylval.varName, buf, strlen(buf)); 
				//return VARSTR;
                // Any other identifier is a named (multi-character)
                // variable. Copy the name, guarding against overflow.
                size_t bl = strlen(buf);
                if (bl >= VAR_NAME_LEN)
                {
                    bl = VAR_NAME_LEN - 1;
                }
                ::memset(&yylval.varName, 0, VAR_NAME_LEN);
                ::memmove(&yylval.varName, buf, bl);
                return VARSTR;
            }
        }
    } // if (::isalpha(cp))

    int nc = GetChar();
    // Parse two symbol operators.
    if ((cp == '+') && (nc == '=')) return PLUS_ASSIGN;
    if ((cp == '-') && (nc == '=')) return MINUS_ASSIGN;
    if ((cp == '*') && (nc == '=')) return MUL_ASSIGN;
    if ((cp == '/') && (nc == '=')) return DIV_ASSIGN;
    if ((cp == '%') && (nc == '=')) return MOD_ASSIGN;
    if ((cp == '+') && (nc == '+')) return PREF_INC;
    if ((cp == '-') && (nc == '-')) return PREF_DEC;
    if ((cp == '>') && (nc == '=')) return GE;
    if ((cp == '<') && (nc == '=')) return LE;
    if ((cp == '=') && (nc == '=')) return EQ;
    if ((cp == '!') && (nc == '=')) return NE;
    if ((cp == '&') && (nc == '&')) return AND;
    if ((cp == '|') && (nc == '|')) return OR;
    if ((cp == '<') && (nc == '<')) return SHL;
    if ((cp == '>') && (nc == '>')) return SHR;

    // Single symbol operator (not alphabet character).
#ifdef CALC_DEBUG
    printf("%c\n", cp);
#endif

    UngetChar(nc);
    return cp;
}


//******************************************************************************
//******************************** Interpreter *********************************
//******************************************************************************


// The name of a variable lvalue, regardless of its representation: named
// variables carry it directly, single-letter ones derive it from the index.
std::string nameOfVar(nodeType* v)
{
    if (v->type == nodeType::typeVar)
    {
        return std::string(v->u.var.name);
    }
    return std::string(1, (char)('a' + v->u.id.i));
}


// Read a variable's current value. Inside a function call the innermost
// local frame is used; at top level the globals (sym[] / varStr) are used.
DataType ReadVar(nodeType* v)
{
    DataType d;
    d.type = DataType::typeDbl;
    d.dbl  = (Double)0;

    // A name bound to an array reads as 0 in scalar context.
    std::map<std::string, ArrayVal>::iterator ai = arrays.find(nameOfVar(v));
    if (ai != arrays.end())
    {
        d.dbl = numOf(ai->second);
        return d;
    }

    if (!m_scopes.empty())
    {
        return m_scopes.back()[nameOfVar(v)];
    }

    if (v->type == nodeType::typeVar)
    {
        if (varStr.find(v->u.var.name) == varStr.end())
        {
            varStr[v->u.var.name] = d;
        }
        return varStr[v->u.var.name];
    }
    return sym[v->u.id.i];
}


// Assign a value to a variable lvalue. Inside a function call the innermost
// local frame is written; at top level named variables go to the varStr map
// and single-letter ones to the sym[] array.
DataType Assign(nodeType* lval, const DataType& val)
{
    if (!lval)
    {
        return val;
    }

    if (!m_scopes.empty())
    {
        m_scopes.back()[nameOfVar(lval)] = val;
        return val;
    }

    if (lval->type == nodeType::typeVar)
    {
        varStr[lval->u.var.name] = val;
    }
    else
    {
        sym[lval->u.id.i] = val;
    }
    return val;
}


// Array reductions len/sum/avg/prod are dispatched by name at call time
// (rather than reserved as keywords, so those words remain usable as
// ordinary variables). Returns true and fills 'out' when fname is a
// reduction and 'arg' denotes an array value. len() is the outermost
// dimension; sum/avg/prod fold over every scalar leaf recursively.
bool tryArrayReduce(const std::string& fname, nodeType* arg, DataType& out)
{
    if (!isArrayNode(arg))
    {
        return false;
    }
    ArrayVal v = evalArr(arg);

    out.type = DataType::typeDbl;
    if (fname == "len")
    {
        out.dbl = v.isScalar ? (Double)1 : (Double)v.arr.size();
    }
    else if (fname == "sum")
    {
        out.dbl = sumVal(v);
    }
    else if (fname == "avg")
    {
        int c = countLeaves(v);
        out.dbl = (c == 0) ? (Double)0 : sumVal(v) / (Double)c;
    }
    else if (fname == "prod")
    {
        out.dbl = prodVal(v);
    }
    else
    {
        return false;
    }
    return true;
}


// Invoke a user-defined function by name with already-evaluated arguments.
// A fresh local frame binds the parameters (missing arguments default to 0);
// the body runs until it returns or falls off the end.
DataType invokeFunc(const std::string& name, const std::vector<DataType>& argv)
{
    DataType ret;
    ret.type = DataType::typeDbl;
    ret.dbl  = (Double)0;

    std::map<std::string, FuncDef>::iterator it = funcs.find(name);
    if (it == funcs.end())
    {
        yyerror("undefined function: ", name);
        return ret;
    }

    FuncDef& fn = it->second;

    std::map<std::string, DataType> frame;
    for (size_t k = 0; k < fn.params.size(); ++k)
    {
        frame[fn.params[k]] = (k < argv.size()) ? argv[k] : DataType();
    }

    m_scopes.push_back(frame);
    try
    {
        ex(fn.body);
    }
    catch (BoaReturn& r)
    {
        ret = r.value;
    }
    m_scopes.pop_back();
    return ret;
}


// Call a one-argument user function with a numeric argument and return its
// numeric result. Used by the intgauss3 builtin to sample the integrand.
double callF1(const std::string& name, double x)
{
    std::vector<DataType> argv(1);
    argv[0].type = DataType::typeDbl;
    argv[0].dbl  = x;
    return invokeFunc(name, argv).dbl;
}


// Wrap a std::string as a string-typed DataType (truncated to MAX_STR_LEN).
DataType makeStr(const std::string& s)
{
    DataType d;
    int len = (s.size() > MAX_STR_LEN) ? MAX_STR_LEN : (int)s.size();
    d.type = DataType::typeStr;
    memset(d.str, 0, MAX_STR_LEN + 1);
    memmove(d.str, s.c_str(), len);
    return d;
}


// The string form of a value: string values as-is, numbers formatted the
// same way tostr() formats them. Used by '+' when one operand is a string.
std::string asStr(const DataType& v)
{
    if (v.type == DataType::typeStr)
    {
        return std::string(v.str);
    }
    std::ostringstream os;
    os << v.dbl;
    return os.str();
}


// ---- Array value helpers -------------------------------------------------

// Scalar value of an array value (0 for a non-scalar array).
Double numOf(const ArrayVal& v) { return v.isScalar ? v.num : (Double)0; }

// Recursive sum of every scalar leaf.
Double sumVal(const ArrayVal& v)
{
    if (v.isScalar) return v.num;
    Double s = 0;
    for (size_t i = 0; i < v.arr.size(); ++i) s += sumVal(v.arr[i]);
    return s;
}

// Recursive product of every scalar leaf.
Double prodVal(const ArrayVal& v)
{
    if (v.isScalar) return v.num;
    Double p = 1;
    for (size_t i = 0; i < v.arr.size(); ++i) p *= prodVal(v.arr[i]);
    return p;
}

// Number of scalar leaves (for avg).
int countLeaves(const ArrayVal& v)
{
    if (v.isScalar) return 1;
    int c = 0;
    for (size_t i = 0; i < v.arr.size(); ++i) c += countLeaves(v.arr[i]);
    return c;
}

// Collect every scalar leaf (for max/min).
void flattenLeaves(const ArrayVal& v, std::vector<Double>& out)
{
    if (v.isScalar) { out.push_back(v.num); return; }
    for (size_t i = 0; i < v.arr.size(); ++i) flattenLeaves(v.arr[i], out);
}

// Nested, bracketed rendering: [[1, 2, 3], [4, 5, 6]].
void printArray(const ArrayVal& v, std::ostringstream& os)
{
    if (v.isScalar) { os << v.num; return; }
    os << "[";
    for (size_t i = 0; i < v.arr.size(); ++i)
    {
        if (i) os << ", ";
        printArray(v.arr[i], os);
    }
    os << "]";
}

// True when a node denotes an array value: an array literal, an element
// access, or a bare variable that names a defined array.
bool isArrayNode(nodeType* p)
{
    if (!p) return false;
    if (p->type == nodeType::typeOpr)
    {
        if ((p->u.opr.oper == ARRAY_LIT) || (p->u.opr.oper == ARRAY_GET))
        {
            return true;
        }
        // A matrix builtin that produces an array (inverse/rotate/...).
        if (p->u.opr.oper == UCALL)
        {
            std::string fname = p->u.opr.op[0]->u.var.name;
            if ((funcs.find(fname) == funcs.end()) && isMatArrayBuiltin(fname))
            {
                return true;
            }
        }
    }
    if (((p->type == nodeType::typeId) || (p->type == nodeType::typeVar)) &&
        (arrays.find(nameOfVar(p)) != arrays.end()))
    {
        return true;
    }
    return false;
}

// Navigate an ARRAY_GET node (op[0] name, op[1..] index chain) and return the
// value found there (scalar 0 if out of range or over-indexed).
ArrayVal getElem(nodeType* p)
{
    ArrayVal zero;
    std::map<std::string, ArrayVal>::iterator it =
        arrays.find(p->u.opr.op[0]->u.var.name);
    if (it == arrays.end()) return zero;

    ArrayVal* cur = &it->second;
    for (int k = 1; k < p->u.opr.nops; ++k)
    {
        int i = (int)ex(p->u.opr.op[k]).dbl;
        if (cur->isScalar || i < 0 || i >= (int)cur->arr.size()) return zero;
        cur = &cur->arr[i];
    }
    return *cur;
}

// Assign into an ARRAY_SET node (op[0] name, op[1] value, op[2..] index chain).
void setElem(nodeType* p)
{
    ArrayVal val = evalArr(p->u.opr.op[1]);
    ArrayVal* cur = &arrays[p->u.opr.op[0]->u.var.name];
    for (int k = 2; k < p->u.opr.nops; ++k)
    {
        int i = (int)ex(p->u.opr.op[k]).dbl;
        if (cur->isScalar || i < 0 || i >= (int)cur->arr.size()) return;
        cur = &cur->arr[i];
    }
    *cur = val;
}

// Evaluate a node to an array value: array literals build a nested value,
// element accesses navigate, bare array names return the stored value, and
// anything else is wrapped as a scalar.
ArrayVal evalArr(nodeType* p)
{
    ArrayVal v;
    if (!p) return v;

    if (p->type == nodeType::typeOpr)
    {
        if (p->u.opr.oper == ARRAY_LIT)
        {
            v.isScalar = false;
            for (int k = 0; k < p->u.opr.nops; ++k)
            {
                v.arr.push_back(evalArr(p->u.opr.op[k]));
            }
            return v;
        }
        if (p->u.opr.oper == ARRAY_GET)
        {
            return getElem(p);
        }
        if (p->u.opr.oper == UCALL)
        {
            std::string fname = p->u.opr.op[0]->u.var.name;
            if (funcs.find(fname) == funcs.end())
            {
                ArrayVal out;
                if (tryMatArrayBuiltin(fname, p, out)) return out;
            }
        }
    }

    if ((p->type == nodeType::typeId) || (p->type == nodeType::typeVar))
    {
        std::map<std::string, ArrayVal>::iterator it =
            arrays.find(nameOfVar(p));
        if (it != arrays.end()) return it->second;
    }

    v.isScalar = true;
    v.num      = (Double)ex(p).dbl;
    return v;
}


// ---- Matrix builtins -----------------------------------------------------
//
// Matrices are ordinary 2D arrays. These builtins convert an array value to a
// plain numeric matrix, run the linear algebra, and convert the result back;
// they are dispatched by name (like the reductions) so they compose, e.g.
// det(submatrix(m, 0, 0)).

BMat toMat(const ArrayVal& v)
{
    BMat m;
    for (size_t i = 0; i < v.arr.size(); ++i)
    {
        BVec r;
        for (size_t j = 0; j < v.arr[i].arr.size(); ++j)
        {
            r.push_back(v.arr[i].arr[j].num);
        }
        m.push_back(r);
    }
    return m;
}

BVec toVec(const ArrayVal& v)
{
    BVec r;
    for (size_t i = 0; i < v.arr.size(); ++i) r.push_back(numOf(v.arr[i]));
    return r;
}

ArrayVal fromMat(const BMat& m)
{
    ArrayVal x;
    x.isScalar = false;
    for (size_t i = 0; i < m.size(); ++i)
    {
        ArrayVal row;
        row.isScalar = false;
        for (size_t j = 0; j < m[i].size(); ++j)
        {
            ArrayVal s;
            s.num = m[i][j];
            row.arr.push_back(s);
        }
        x.arr.push_back(row);
    }
    return x;
}

ArrayVal fromVec(const BVec& v)
{
    ArrayVal x;
    x.isScalar = false;
    for (size_t i = 0; i < v.size(); ++i)
    {
        ArrayVal s;
        s.num = v[i];
        x.arr.push_back(s);
    }
    return x;
}

Double matDet(BMat a)
{
    int n = (int)a.size();
    Double det = 1.0;
    for (int i = 0; i < n; ++i)
    {
        int p = i;
        for (int r = i + 1; r < n; ++r) if (fabs(a[r][i]) > fabs(a[p][i])) p = r;
        if (fabs(a[p][i]) < 1e-12) return 0.0;
        if (p != i) { std::swap(a[p], a[i]); det = -det; }
        det *= a[i][i];
        for (int r = i + 1; r < n; ++r)
        {
            Double f = a[r][i] / a[i][i];
            for (int c = i; c < n; ++c) a[r][c] -= f * a[i][c];
        }
    }
    return det;
}

BMat matInverse(BMat a)
{
    int n = (int)a.size();
    BMat inv(n, BVec(n, 0.0));
    for (int i = 0; i < n; ++i) inv[i][i] = 1.0;
    for (int i = 0; i < n; ++i)
    {
        int p = i;
        for (int r = i + 1; r < n; ++r) if (fabs(a[r][i]) > fabs(a[p][i])) p = r;
        std::swap(a[p], a[i]); std::swap(inv[p], inv[i]);
        Double dd = a[i][i];
        for (int c = 0; c < n; ++c) { a[i][c] /= dd; inv[i][c] /= dd; }
        for (int r = 0; r < n; ++r)
        {
            if (r == i) continue;
            Double f = a[r][i];
            for (int c = 0; c < n; ++c) { a[r][c] -= f * a[i][c]; inv[r][c] -= f * inv[i][c]; }
        }
    }
    return inv;
}

BMat matRotate(const BMat& m)   // 90 degrees clockwise
{
    if (m.empty()) return m;
    int rows = (int)m.size(), cols = (int)m[0].size();
    BMat r(cols, BVec(rows, 0.0));
    for (int i = 0; i < cols; ++i)
        for (int j = 0; j < rows; ++j)
            r[i][j] = m[rows - 1 - j][i];
    return r;
}

BMat matSubmatrix(const BMat& m, int ri, int cj)
{
    BMat r;
    for (int i = 0; i < (int)m.size(); ++i)
    {
        if (i == ri) continue;
        BVec row;
        for (int j = 0; j < (int)m[i].size(); ++j)
            if (j != cj) row.push_back(m[i][j]);
        r.push_back(row);
    }
    return r;
}

BVec matSolve(BMat a, BVec b)   // solves a x = b by Gaussian elimination
{
    int n = (int)a.size();
    for (int i = 0; i < n; ++i)
    {
        int p = i;
        for (int r = i + 1; r < n; ++r) if (fabs(a[r][i]) > fabs(a[p][i])) p = r;
        std::swap(a[p], a[i]); std::swap(b[p], b[i]);
        for (int r = i + 1; r < n; ++r)
        {
            Double f = a[r][i] / a[i][i];
            for (int c = i; c < n; ++c) a[r][c] -= f * a[i][c];
            b[r] -= f * b[i];
        }
    }
    BVec x(n, 0.0);
    for (int i = n - 1; i >= 0; --i)
    {
        Double s = b[i];
        for (int c = i + 1; c < n; ++c) s -= a[i][c] * x[c];
        x[i] = s / a[i][i];
    }
    return x;
}

// The matrix builtins that return an array value (as opposed to det, which
// returns a scalar).
bool isMatArrayBuiltin(const std::string& n)
{
    return (n == "inverse") || (n == "rotate") ||
           (n == "submatrix") || (n == "solve");
}

// Evaluate an array-returning matrix-builtin call node to an array value.
bool tryMatArrayBuiltin(const std::string& n, nodeType* p, ArrayVal& out)
{
    int nargs = p->u.opr.nops - 1;
    if ((n == "inverse") && (nargs == 1))
    {
        out = fromMat(matInverse(toMat(evalArr(p->u.opr.op[1]))));
        return true;
    }
    if ((n == "rotate") && (nargs == 1))
    {
        out = fromMat(matRotate(toMat(evalArr(p->u.opr.op[1]))));
        return true;
    }
    if ((n == "submatrix") && (nargs == 3))
    {
        int ri = (int)ex(p->u.opr.op[2]).dbl;
        int cj = (int)ex(p->u.opr.op[3]).dbl;
        out = fromMat(matSubmatrix(toMat(evalArr(p->u.opr.op[1])), ri, cj));
        return true;
    }
    if ((n == "solve") && (nargs == 2))
    {
        out = fromVec(matSolve(toMat(evalArr(p->u.opr.op[1])),
                               toVec(evalArr(p->u.opr.op[2]))));
        return true;
    }
    return false;
}


// Execute a top-level statement, absorbing a 'return' used outside any
// function rather than letting it escape as an uncaught exception.
void execTop(nodeType* p)
{
    try
    {
        ex(p);
    }
    catch (BoaReturn&)
    {
    }
}


DataType ex(nodeType* p)
{
    DataType d;
    // By definition.
    d.type =  DataType::typeDbl;
    d.dbl  = (Double)0;

    if (!p) return d;

    switch(p->type)
    {
    
    case nodeType::typeVarCon:
    case nodeType::typeCon:
#ifdef VARSTR_DEBUG
        std::cout << "p->u.con.value = " << p->u.con.value << std::endl; 
#endif
        return p->u.con.value;
    
    case nodeType::typeId:
        return ReadVar(p);

    case nodeType::typeVar:
#ifdef VARSTR_DEBUG
        std::cout << "p->u.var.name = " << p->u.var.name << std::endl;
#endif
        return ReadVar(p);

    case nodeType::typeOpr:

        switch(p->u.opr.oper)
        {
        case WHILE:
            {
                DataType tmp = ex(p->u.opr.op[0]);
                while((Int32)tmp.dbl)
                {
                    ex(p->u.opr.op[1]);
                    tmp = ex(p->u.opr.op[0]);
                }
             }
            return d;

        case FOR:
            {
                // op[0] init, op[1] cond, op[2] post, op[3] body.
                ex(p->u.opr.op[0]);
                while ((Int32)ex(p->u.opr.op[1]).dbl)
                {
                    ex(p->u.opr.op[3]);
                    ex(p->u.opr.op[2]);
                }
            }
            return d;

        case RETURN:
            {
                BoaReturn r;
                r.value = ex(p->u.opr.op[0]);
                throw r;
            }

        case CASE:
            {
                // op[0] switch value; then (value, body) pairs, a null
                // value marking the default arm.
                DataType  sw       = ex(p->u.opr.op[0]);
                nodeType* elseBody = 0;
                for (int k = 1; k + 1 < p->u.opr.nops; k += 2)
                {
                    nodeType* val  = p->u.opr.op[k];
                    nodeType* body = p->u.opr.op[k + 1];
                    if (val == 0)
                    {
                        elseBody = body;
                        continue;
                    }
                    DataType v = ex(val);
                    bool match = (sw.type == DataType::typeStr &&
                                  v.type  == DataType::typeStr)
                                 ? (std::string(sw.str) == std::string(v.str))
                                 : (sw.dbl == v.dbl);
                    if (match)
                    {
                        ex(body);
                        return d;
                    }
                }
                if (elseBody)
                {
                    ex(elseBody);
                }
            }
            return d;

        case UCALL:
            {
                // op[0] carries the function name; op[1..] the arguments.
                std::string fname = p->u.opr.op[0]->u.var.name;
                int nargs = p->u.opr.nops - 1;

                // Zero-argument informational builtins (unless shadowed by a
                // user function of the same name).
                if ((nargs == 0) && (funcs.find(fname) == funcs.end()))
                {
                    if (fname == "version")
                    {
                        return makeStr(BOASCRIPT_VERSION);
                    }
                    if ((fname == "about") || (fname == "description"))
                    {
                        return makeStr(BOASCRIPT_DESCRIPTION);
                    }
                }

                // len/sum/avg/prod on a named array take precedence over a
                // user function only when no such user function is defined.
                if ((nargs == 1) && (funcs.find(fname) == funcs.end()))
                {
                    DataType red;
                    if (tryArrayReduce(fname, p->u.opr.op[1], red))
                    {
                        return red;
                    }
                    // len() also returns the length of a string (or of a
                    // number's string form), like strlen.
                    if (fname == "len")
                    {
                        d.dbl = (Double)asStr(ex(p->u.opr.op[1])).size();
                        return d;
                    }
                }

                // Builtins (unless shadowed by a user function of the name).
                if (funcs.find(fname) == funcs.end())
                {
                    // color(name) records a requested output color as session
                    // state; color() with no argument clears it. The value has
                    // no effect inside the library -- an interactive front-end
                    // (the boa REPL) reads outputColor() and tints its output.
                    if (fname == "color")
                    {
                        m_outColor = (nargs >= 1) ? asStr(ex(p->u.opr.op[1]))
                                                  : std::string();
                        return makeStr(m_outColor);
                    }
                    if ((nargs == 1) && (fname == "det"))
                    {
                        d.dbl = (Double)matDet(toMat(evalArr(p->u.opr.op[1])));
                        return d;
                    }
                    if ((nargs == 1) && (fname == "bitnot"))
                    {
                        d.dbl = (Double)(~(Int32)ex(p->u.opr.op[1]).dbl);
                        return d;
                    }
                    if ((nargs == 2) && (fname == "xor"))
                    {
                        d.dbl = (Double)((Int32)ex(p->u.opr.op[1]).dbl ^
                                         (Int32)ex(p->u.opr.op[2]).dbl);
                        return d;
                    }
                    // Array-returning matrix builtins used in scalar context
                    // read as 0 (like any whole array); the array value is
                    // produced by evalArr().
                    if (isMatArrayBuiltin(fname))
                    {
                        d.dbl = numOf(evalArr(p));
                        return d;
                    }
                }

                // Evaluate the arguments in the current scope first.
                std::vector<DataType> argv;
                for (int k = 0; k < nargs; ++k)
                {
                    argv.push_back(ex(p->u.opr.op[k + 1]));
                }
                return invokeFunc(fname, argv);
            }

        case INTGAUSS3:
            {
                // intgauss3(f, a, b): 3-point Gauss-Legendre integral of the
                // user function f over [a, b]. op[0] carries the function
                // name; op[1] and op[2] are the bounds. Nodes +/-sqrt(3/5)
                // and 0 with weights 5/9, 8/9, 5/9, mapped from [-1,1] to
                // [a,b] via x = h*xi + c. Exact for polynomials up to
                // degree 5.
                std::string fname = p->u.opr.op[0]->u.var.name;
                if (funcs.find(fname) == funcs.end())
                {
                    yyerror("undefined function: ", fname);
                    return d;
                }

                double a = (double)ex(p->u.opr.op[1]).dbl;
                double b = (double)ex(p->u.opr.op[2]).dbl;

                double h = (b - a) / 2.0;
                double c = (a + b) / 2.0;
                double s = sqrt(3.0 / 5.0);

                d.dbl = (Double)(h * (5.0 / 9.0 * callF1(fname, c - h * s)
                                    + 8.0 / 9.0 * callF1(fname, c)
                                    + 5.0 / 9.0 * callF1(fname, c + h * s)));
            }
            return d;

        // Arrays. op[0] of every array node carries the array name.

        case ARRAY_LIT:
            // An array literal in scalar context is 0 (see numOf); it is
            // realized as a value by evalArr where an array is expected.
            return d;

        case ARRAY_GET:
            // Element access in scalar context yields the scalar found there
            // (0 if it lands on a sub-array or out of range).
            d.dbl = numOf(getElem(p));
            return d;

        case ARRAY_SET:
            setElem(p);
            return d;

        case ARRAY_MAX:
        case ARRAY_MIN:
            {
                std::vector<Double> leaves;
                flattenLeaves(evalArr(p->u.opr.op[0]), leaves);
                Double m = leaves.empty() ? (Double)0 : leaves[0];
                for (size_t k = 0; k < leaves.size(); ++k)
                {
                    if ((p->u.opr.oper == ARRAY_MAX) ? (leaves[k] > m)
                                                     : (leaves[k] < m))
                    {
                        m = leaves[k];
                    }
                }
                d.dbl = m;
            }
            return d;

        case IF:
            {
                if ((int)ex(p->u.opr.op[0]).dbl)
                {
                    ex(p->u.opr.op[1]);
                }
                else if (p->u.opr.nops > 2)
                {
                    ex(p->u.opr.op[2]);
                }
            }
            return d;

        case PRINT:
        case PRINTLN:
            {
                // If the operand denotes an array value, print it in nested
                // bracket form ([[1, 2], [3, 4]]) instead of as a scalar.
                nodeType* arg = p->u.opr.op[0];
                if (isArrayNode(arg))
                {
                    ArrayVal av = evalArr(arg);
                    if (!av.isScalar)
                    {
                        std::ostringstream oss(std::ostringstream::out);
                        printArray(av, oss);
                        if (p->u.opr.oper == PRINTLN)
                        {
                            oss << std::endl;
                        }
                        m_outBuf += oss.str();
                        return d;
                    }
                }

                DataType tmp = ex(p->u.opr.op[0]);
#ifdef VARSTR_DEBUG
				std::cout << "DataType tmp = " << tmp << std::endl;
#endif
                if (tmp.type == DataType::typeDbl)
                {
#ifndef CALC_BATCH
                    std::cout << std::fixed;
                    if (precision <= 0) precision = 7;
                    std::cout << std::setprecision(precision);
                    std::cout << tmp.dbl;
                    if (p->u.opr.oper == PRINTLN)
                    {
                        std::cout << std::endl;
                    }
#else
                    std::ostringstream oss(std::ostringstream::out);
                    oss << std::fixed;
                    if (precision <= 0) precision = 7;
                    oss << std::setprecision(precision);
                    oss << tmp.dbl;
                    if (p->u.opr.oper == PRINTLN)
                    {
                       oss << std::endl;
                    }
                    m_outBuf += oss.str();
#endif // CALC_BATCH
                }
                else if (tmp.type == DataType::typeStr)
                {
#ifndef CALC_BATCH
                    std::cout << tmp.str;
                    if (p->u.opr.oper == PRINTLN)
                    {
                        std::cout << std::endl;
                    }
#else
                    std::ostringstream oss(std::ostringstream::out);
                    oss << tmp.str;
                    if (p->u.opr.oper == PRINTLN)
                    {
                       oss << std::endl;
                    }
                    m_outBuf += oss.str();
#endif // CALC_BATCH
                }
                else
                {
                    yyerror("Unknown type");
                }
            }
            return d;

        case ';': 
            ex(p->u.opr.op[0]);
            return ex(p->u.opr.op[1]);

        case COMMENT: 
            ex(p->u.opr.op[0]);
            return ex(p->u.opr.op[1]);

        case '=':
            {
                nodeType*   rhs  = p->u.opr.op[1];
                std::string name = nameOfVar(p->u.opr.op[0]);

                // An array-valued right-hand side (a literal, an element
                // access, a matrix-builtin result, or another array) is stored
                // in the array namespace; a scalar element access falls back
                // to a scalar assignment.
                if (isArrayNode(rhs))
                {
                    ArrayVal av = evalArr(rhs);
                    if (!av.isScalar)
                    {
                        arrays[name] = av;
                        return d;
                    }
                    arrays.erase(name);
                    DataType s;
                    s.type = DataType::typeDbl;
                    s.dbl  = av.num;
                    return Assign(p->u.opr.op[0], s);
                }

                // Scalar (or string) assignment sheds any array binding.
                arrays.erase(name);
                return Assign(p->u.opr.op[0], ex(rhs));
            }
            
        case '~':
            {
                DataType tmp = ex(p->u.opr.op[1]);
#ifdef VARSTR_DEBUG

                std::cout << "case ~: tmp = " << tmp 
                          << std::endl; 

                std::cout << "case ~: p->u.opr.op[0]->u.var.name = " 
                          << p->u.opr.op[0]->u.var.name 
                          << std::endl; 
#endif
                varStr[p->u.opr.op[0]->u.var.name] = tmp;

#ifdef VARSTR_DEBUG

                std::cout << "case ~: varStr[p->u.opr.op[0]->u.var.name] = " 
                          << varStr[p->u.opr.op[0]->u.var.name] 
                          << std::endl; 
#endif  
                return tmp;
            }

        // Arithmetic section.

        case UMINUS:
            {
                d.dbl = - ex(p->u.opr.op[0]).dbl;
            }
            return d;

        case '+':
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                if ((op0.type == DataType::typeDbl) &&
                    (op1.type == DataType::typeDbl))
                {
                    // Both numeric: add.
                    d.dbl = op0.dbl + op1.dbl;
                }
                else
                {
                    // Otherwise concatenate, stringifying any number.
                    return makeStr(asStr(op0) + asStr(op1));
                }
            }
            return d;

        case '-':
            d.dbl = ex(p->u.opr.op[0]).dbl - ex(p->u.opr.op[1]).dbl;
            return d;

        case '*':
            d.dbl = ex(p->u.opr.op[0]).dbl * ex(p->u.opr.op[1]).dbl;
            return d;

        case '/':
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                if (op1.dbl)
                {
                    d.dbl  = op0.dbl / op1.dbl;
                }
                else
                {
                    d.dbl = (Double)0;
                }
            }
            return d;

        case '%':
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                if (op1.dbl)
                {
                    d.dbl  = (Double)((Int32)op0.dbl % (Int32)op1.dbl);
                }
                else
                {
                    d.dbl = (Double)0;
                }
            }
            return d;

        case PLUS_ASSIGN:
            d.dbl = ex(p->u.opr.op[0]).dbl + ex(p->u.opr.op[1]).dbl;
            Assign(p->u.opr.op[0], d);
            return d;
                
        case MINUS_ASSIGN:
            d.dbl = ex(p->u.opr.op[0]).dbl - ex(p->u.opr.op[1]).dbl;
            Assign(p->u.opr.op[0], d);
            return d;
                
        case MUL_ASSIGN:
            d.dbl = ex(p->u.opr.op[0]).dbl * ex(p->u.opr.op[1]).dbl;
            Assign(p->u.opr.op[0], d);
            return d;
               
        case DIV_ASSIGN:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                if (op1.dbl)
                {
                    d.dbl = op0.dbl / op1.dbl;
                }
                else
                {
                    d.dbl = (Double)0;
                }
                Assign(p->u.opr.op[0], d);
            }
            return d;
               
        case MOD_ASSIGN:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                if (op1.dbl)
                {
                    d.dbl = (Double)((int)op0.dbl % (int)op1.dbl);
                }
                else
                {
                    d.dbl = (Double)0;
                }
                Assign(p->u.opr.op[0], d);
            }
            return d;
     
        case PREF_INC:
            d.dbl = ex(p->u.opr.op[0]).dbl + (Double)1;
            Assign(p->u.opr.op[0], d);
            return d;

        case PREF_DEC:
            d.dbl = ex(p->u.opr.op[0]).dbl - (Double)1;
            Assign(p->u.opr.op[0], d);
            return d;

        // Logical operations.

        case '>':
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = op0.dbl > op1.dbl;
                }
                else if ((op0.type == DataType::typeStr) && 
                         (op1.type == DataType::typeStr))
                {
                    d.dbl = std::string(op0.str) > std::string(op1.str);
                }
            }
            return d;

        case '<':
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = op0.dbl < op1.dbl;
                }
                else if ((op0.type == DataType::typeStr) && 
                         (op1.type == DataType::typeStr))
                {
                    d.dbl = std::string(op0.str) < std::string(op1.str);
                }
            }
            return d;

        case GE:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = op0.dbl >= op1.dbl;
                }
                else if ((op0.type == DataType::typeStr) && 
                         (op1.type == DataType::typeStr))
                {
                    d.dbl = std::string(op0.str) >= std::string(op1.str);
                }
            }
            return d;

        case LE:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = op0.dbl <= op1.dbl;
                }
                else if ((op0.type == DataType::typeStr) && 
                         (op1.type == DataType::typeStr))
                {
                    d.dbl = std::string(op0.str) <= std::string(op1.str);
                }
            }
            return d;

        case EQ:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = op0.dbl == op1.dbl;
                }
                else if ((op0.type == DataType::typeStr) && 
                         (op1.type == DataType::typeStr))
                {
                    d.dbl = std::string(op0.str) == std::string(op1.str);
                }
            }
            return d;

        case NE:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = op0.dbl != op1.dbl;
                }
                else if ((op0.type == DataType::typeStr) && 
                         (op1.type == DataType::typeStr))
                {
                    d.dbl = std::string(op0.str) != std::string(op1.str);
                }
            }
            return d;
        
        case AND:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = (Double)((Int32)op0.dbl && (Int32)op1.dbl);
                }
            }
            return d;

        case OR:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = (Double)((Int32)op0.dbl || (Int32)op1.dbl);
                }
            }
            return d;

        // Bit operations

        case '&':
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = (Double)((Int32)op0.dbl & (Int32)op1.dbl);
                }
            }
            return d;

        case '|':
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = (Double)((Int32)op0.dbl | (Int32)op1.dbl);
                }
            }
            return d;

        case BNE:
            {
                d.dbl = (Double)(!(Int32)ex(p->u.opr.op[0]).dbl);
            }
            return d;

        case SHR:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = (Double)((Int32)op0.dbl >> (Int32)op1.dbl);
                }
            }
            return d;

        case SHL:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);

                if ((op0.type == DataType::typeDbl) && 
                    (op1.type == DataType::typeDbl))
                {
                   d.dbl = (Double)((Int32)op0.dbl << (Int32)op1.dbl);
                }
            }
            return d;

        // Other numerical operations.

        case MIN:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                d.dbl = (op0.dbl < op1.dbl) ? op0.dbl : op1.dbl;
            }
            return d;

        case MAX:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                d.dbl = (op0.dbl > op1.dbl) ? op0.dbl : op1.dbl;
            }
            return d;

        case '?':
            return (ex(p->u.opr.op[0]).dbl ? 
                    ex(p->u.opr.op[1]) : 
                    ex(p->u.opr.op[2]));

        // Functions section.

        case ABS:
            {
                DataType op0 =  ex(p->u.opr.op[0]);
                d.dbl = ((op0.dbl > (Double)0) ? op0.dbl : - op0.dbl);
            }
            return d;

        case ACOS:
            d.dbl = (Double)acos((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case ASIN:
            d.dbl = (Double)asin((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case ATAN:
            d.dbl = (Double)atan((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case ATAN2:
            d.dbl = (Double)atan2((double)ex(p->u.opr.op[0]).dbl,
                                  (double)ex(p->u.opr.op[1]).dbl);
            return d;

        case CEIL:
            d.dbl = (Double)ceil((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case COS:
            d.dbl = (Double)cos((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case COSH:
            d.dbl = (Double)cosh((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case EXP:
            d.dbl = (Double)exp((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case FABS:
            d.dbl = (Double)fabs((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case FLOOR:
            d.dbl = (Double)floor((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case FMOD:
            d.dbl = (Double)fmod((double)ex(p->u.opr.op[0]).dbl,
                                 (double)ex(p->u.opr.op[1]).dbl);
            return d;

        case HYPOT:
            d.dbl = (Double)hypot((double)ex(p->u.opr.op[0]).dbl,
                                  (double)ex(p->u.opr.op[1]).dbl);
            return d;

        case FREXP:
            {
                double arg1 = (double)ex(p->u.opr.op[0]).dbl;
                int    arg2 = (int)ex(p->u.opr.op[1]).dbl;
                d.dbl = (Double)frexp(arg1, &arg2);
            }
            return d;

        case LDEXP:
            d.dbl = (Double)ldexp((double)ex(p->u.opr.op[0]).dbl,
                                  (int)ex(p->u.opr.op[1]).dbl);
            return d;

        case LOG:
            d.dbl = (Double)log((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case LOG10:
            d.dbl = (Double)(log((double)ex(p->u.opr.op[0]).dbl) / 
                             log((double)10));
            return d;

        case MODF:
            {
                double arg1 = (double)ex(p->u.opr.op[0]).dbl;
                double arg2 = (double)ex(p->u.opr.op[1]).dbl;
                d.dbl = (Double)modf(arg1, &arg2);
            }
            return d;

        case POW:
            d.dbl = (Double)pow((double)ex(p->u.opr.op[0]).dbl,
                                (double)ex(p->u.opr.op[1]).dbl);
            return d;

        case SIN:
            d.dbl = (Double)sin((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case SINH:
            d.dbl = (Double)sinh((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case SQRT:
            d.dbl = (Double)sqrt((double)ex(p->u.opr.op[0]).dbl);
            return d;
                
        case CBRT:
            d.dbl = (Double)cbrt((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case TAN:
            d.dbl = (Double)tan((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case TANH:
            d.dbl = (Double)tanh((double)ex(p->u.opr.op[0]).dbl);
            return d;

        case RAND:
            d.dbl = (Double)rand() / (Double)RAND_MAX;
            return d;
            
        case PI:
            d.dbl = (Double)3.1415926535897932384626433832795;
            return d;

            // Functions of string argument returning numbers.

        case TONUM:
            {
                std::istringstream is(ex(p->u.opr.op[0]).str);
                is >> d.dbl;
            }
            return d;

        case TOSTR:
            {
                std::ostringstream os;
                os << ex(p->u.opr.op[0]).dbl;
                std::string out = os.str();
                int len = (out.size() > MAX_STR_LEN) ?
                          MAX_STR_LEN : out.size();
                d.type = DataType::typeStr;
                memset(d.str, 0, MAX_STR_LEN + 1);
                memmove(d.str, out.c_str(), len);
            }
            return d;

        case UPPER:
        case LOWER:
            {
                std::string s = std::string(ex(p->u.opr.op[0]).str);
                for (size_t k = 0; k < s.size(); ++k)
                {
                    s[k] = (p->u.opr.oper == UPPER)
                           ? (char)::toupper((unsigned char)s[k])
                           : (char)::tolower((unsigned char)s[k]);
                }
                int len = (s.size() > MAX_STR_LEN) ? MAX_STR_LEN : s.size();
                d.type = DataType::typeStr;
                memset(d.str, 0, MAX_STR_LEN + 1);
                memmove(d.str, s.c_str(), len);
            }
            return d;

        case REVERSE:
            {
                std::string s = asStr(ex(p->u.opr.op[0]));
                std::reverse(s.begin(), s.end());
                return makeStr(s);
            }

        case FIND:
            {
                std::string s   = asStr(ex(p->u.opr.op[0]));
                std::string sub = asStr(ex(p->u.opr.op[1]));
                size_t pos = s.find(sub);
                d.dbl = (pos == std::string::npos) ? (Double)-1 : (Double)pos;
            }
            return d;

        case REPEAT:
            {
                std::string s = asStr(ex(p->u.opr.op[0]));
                int k = (int)ex(p->u.opr.op[1]).dbl;
                std::string out;
                for (int j = 0; j < k; ++j)
                {
                    out += s;
                    if (out.size() > MAX_STR_LEN) break;
                }
                return makeStr(out);
            }

        case CHARAT:
            {
                std::string s = asStr(ex(p->u.opr.op[0]));
                int i = (int)ex(p->u.opr.op[1]).dbl;
                return makeStr((i >= 0 && i < (int)s.size())
                               ? std::string(1, s[i]) : std::string());
            }

        case STRLEN:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                d.dbl = (Double)std::string(op0.str).size();
            }
            return d;

        case MATCH:
            {
                DataType op0 =  ex(p->u.opr.op[0]);
                DataType op1 =  ex(p->u.opr.op[1]);
                const std::string instr   = std::string(op0.str);
                const std::string pattern = std::string(op1.str);
                d.dbl = 1; //util::Tokenizer::match(instr, pattern);
            }
            return d;

        // Functions of string argument returning string

        case DATE:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                std::string out = util::DateTime::Get(op0.str);
                int len = (out.size() > MAX_STR_LEN) ? 
                          MAX_STR_LEN : out.size();
                d.type = DataType::typeStr;
                memset(d.str, 0, MAX_STR_LEN + 1);
                memmove(d.str, out.c_str(), len);
            }
            return d;

        case SUBSTR:
            {
                DataType op0 =  ex(p->u.opr.op[0]);
                DataType op1 =  ex(p->u.opr.op[1]);
                DataType op2 =  ex(p->u.opr.op[2]);
                if ((op1.type == DataType::typeDbl) && 
                    (op2.type == DataType::typeDbl))
                {
                    std::string in  = std::string(op0.str);
                    std::string out = in.substr((size_t)op1.dbl,
                                                (size_t)op2.dbl);
                    int len = (out.size() > MAX_STR_LEN) ? 
                              MAX_STR_LEN : out.size();
                    // If the result has different type of data then
                    // default, please mention it.
                    d.type = DataType::typeStr;
                    // Copy data.
                    memset(d.str, 0, MAX_STR_LEN + 1);
                    memmove(d.str, out.c_str(), len);
                }
            }
            return d;

        case REPLACE:
            {
                DataType    op0 =  ex(p->u.opr.op[0]);
                DataType    op1 =  ex(p->u.opr.op[1]);
                DataType    op2 =  ex(p->u.opr.op[2]);
                DataType    op3 =  ex(p->u.opr.op[3]);
                std::string s0 = std::string(op0.str);
                std::string s1 = std::string(op1.str);
                s0.replace((size_t)op2.dbl, (size_t)op3.dbl, s1);
                // Copy data.
                int len = (s0.size() > MAX_STR_LEN) ? MAX_STR_LEN : s0.size();
                d.type  = DataType::typeStr;
                memset(d.str, 0, MAX_STR_LEN + 1);
                memmove(d.str, s0.c_str(), len);
            }
            return d;

        case SUBSTITUTE:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                DataType op2 = ex(p->u.opr.op[2]);

                const std::string instr   = std::string(op0.str);
                const std::string pattern = std::string(op1.str);
                const std::string target  = std::string(op2.str);
                const std::string outstr  = util::Tokenizer::substitute(instr,
                                                                       pattern,
                                                                       target);
                // Copy data.
                int len = (outstr.size() > MAX_STR_LEN) ? 
                          MAX_STR_LEN : outstr.size();
                d.type  = DataType::typeStr;
                memset(d.str, 0, MAX_STR_LEN + 1);
                memmove(d.str, outstr.c_str(), len);
            }
            return d;

        // For compatibility with previous versions.
        case IFN:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                DataType op2 = ex(p->u.opr.op[2]);
                if (op0.type ==  DataType::typeDbl)
                {
                    if ((int)op0.dbl)
                    {
                        if (op1.type ==  DataType::typeDbl)
                        {
                            d.dbl = op1.dbl;
                        }
                        else if (op1.type ==  DataType::typeStr)
                        {
                            int len = (strlen(op1.str) > MAX_STR_LEN) ? 
                                      MAX_STR_LEN : strlen(op1.str);
                            d.type  = DataType::typeStr;
                            memset(d.str, 0, MAX_STR_LEN + 1);
                            memmove(d.str, op1.str, len);
                        }
                    }
                    else
                    {
                        if (op2.type == DataType::typeDbl)
                        {
                            d.dbl = op2.dbl;
                        }
                        else if (op2.type == DataType::typeStr)
                        {
                            int len = (strlen(op2.str) > MAX_STR_LEN) ? 
                                       MAX_STR_LEN : strlen(op2.str);
                            d.type  = DataType::typeStr;
                            memset(d.str, 0, MAX_STR_LEN + 1);
                            memmove(d.str, op2.str, len);
                        }
                    }
                }
            }
            return d;

        case SETPREC:
            precision = ex(p->u.opr.op[0]).dbl;
            d.dbl = (Double)precision;
            return d;

        case GETPREC:
            d.dbl = (Double)precision;
            return d;

        } // switch(p->u.opr.oper)
    } // switch(p->type)

    return d;
}

private:

    int                             precision;
    std::string                     m_outColor;   // color() request (name; "" = default)
    DataType                        sym['z' - 'a' + 1];
    std::map<std::string, DataType> varStr;

    // User-defined functions, by name.
    std::map<std::string, FuncDef>  funcs;

    // Named arrays, by name. Values may be nested (multi-dimensional).
    std::map<std::string, ArrayVal> arrays;

    // Local-variable frames, one per active function call. Empty at top
    // level, where the globals (sym[] / varStr) are used instead.
    std::vector<std::map<std::string, DataType> > m_scopes;

    // Every AST node allocated during a parse is registered here so it
    // can be released in Close(), including nodes left dangling when a
    // parse aborts on a syntax error.
    std::vector<nodeType*>          m_nodes;

#ifdef CALC_BATCH

    int   m_bufInd;
    int   m_inLen;
    char* m_inBuf;
    Cell  m_outBuf;

#endif // CALC_BATCH

