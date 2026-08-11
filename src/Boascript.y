/* Calcmill.y                  -*-YACC-*-                        */

//@PURPOSE: Interpretating language, regular expressions evaluator.
//
//@CLASSES:
//  
//
//@AUTHOR: Boris Rayskiy
//
//@SEE_ALSO: 
//
//@DESCRIPTION: 
//
///Usage:
///
//   
//#ifndef CALC_BATCH
//
//    BoaScript c;
//    c.yyparse();
//
//#else
//
//    BoaScript c;
//
//    c.Init();
//    c.Load("a=sqrt(2); print a; //Comment is here");
//    c.yyparse();
//    c.Close();
//    std::cout << c.GetRes() << std::endl;

//    c.Init();
//    c.Load("a=sqrt(3); print a; //Another comment");
//    c.yyparse();
//    c.Close();
//    std::cout << c.GetRes() << std::endl;
//
//    std::cout << c.Calc("a=2; b=3; c=a*b; d = sqrt(c); print d;") 
//              << std::endl;
//    
//#endif // CALC_BATCH 

%{


#include <BTypes.h>
#include <DateTime.h>
#include <Tokenizer.h>
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
%token STRLEN SUBSTR REPLACE SUBSTITUTE TOSTR TONUM
%token ABS ACOS ASIN ATAN ATAN2 CEIL COS COSH EXP FABS FLOOR FMOD
%token FREXP LDEXP LOG LOG10 MODF POW SIN SINH SQRT CBRT TAN TANH
%token MIN MAX RAND DATE MATCH PI
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
%type <nList> args arglist arms

%%

program:
               function                 { return 0;                          }

function:
               function stmt            { execTop($2);                       }
               | /* NULL */
               ;

stmt:
               ';'                      { $$ = opr(';', 2, 0, 0);            }
               | COMMENT                { $$ = opr(COMMENT, 2, 0, 0);        }
               | expr ';'               { $$ = $1;                           }
               | PRINT expr ';'         { $$ = opr(PRINT, 1, $2);            }
               | PRINTLN expr ';'       { $$ = opr(PRINTLN, 1, $2);          }
               | VARIABLE '=' expr      { $$ = opr('=', 2, id($1), $3);      }
               | VARSTR '=' expr        { $$ = opr('=', 2, setVar($1), $3);  }
               | VARSTR '~' expr        { $$ = opr('~', 2, setVar($1), $3);  }
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
                     { defFunc($2, $4, $6); $$ = opr(';', 2, 0, 0);          }
               | FUNC VARIABLE '(' params ')' stmt
                     { defFunc(varOf($2), $4, $6); $$ = opr(';', 2, 0, 0);   }
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


nodeType* getVar(std::string name)
{
#ifdef VARSTR_DEBUG
        std::cout << "getVar name = " << name << std::endl; 
#endif
    nodeType* p = 0;
    // Allocate node.
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::conNodeType);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("out of memory");
    }

    p->type = nodeType::typeVarCon;

    if (varStr.find(name) == varStr.end())
    {
        varStr[name] = DataType();
    }

    ::memmove(&p->u.con.value, &varStr[name], sizeof(DataType));

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


void freeNode(nodeType* p)
{
    if (!p) return;

    if (p->type == nodeType::typeOpr)
    {
        for (int i = 0; i < p->u.opr.nops; i++)
        {
            freeNode(p->u.opr.op[i]);
        }
    }
    free(p);
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


std::string& Calc(const std::string in)
{
    Init();
    Load(in);
    yyparse();
    Close();
    return GetRes();
}


Column Calc(Column& in)
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


void Calc(const Column& in, Column& out)
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
    m_scopes.clear();

    // Clear any output left over from a previous Calc() on this object.
    m_outBuf.clear();

    precision = 7;
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
        UngetChar(cp);
        yylval.Value.dbl = ScanNum();
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
            if (strcmp(buf, "print") == 0)
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


int isLittleEndian(void)
{
    union Endian
    {
        unsigned char a[4];
        unsigned int  b;
    } endian;

    endian.b = 0x01020304;

    if (endian.a[0] < endian.a[3])
    {
        return 0;
    }
    return 1;
}


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
    if (!m_scopes.empty())
    {
        return m_scopes.back()[nameOfVar(v)];
    }

    DataType d;
    d.type = DataType::typeDbl;
    d.dbl  = (Double)0;

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
                std::map<std::string, FuncDef>::iterator it = funcs.find(fname);
                if (it == funcs.end())
                {
                    yyerror("undefined function: ", fname);
                    return d;
                }

                FuncDef& fn = it->second;

                // Evaluate the arguments in the current scope first.
                int nargs = p->u.opr.nops - 1;
                std::vector<DataType> argv;
                for (int k = 0; k < nargs; ++k)
                {
                    argv.push_back(ex(p->u.opr.op[k + 1]));
                }

                // Bind parameters into a fresh local frame.
                std::map<std::string, DataType> frame;
                for (size_t k = 0; k < fn.params.size(); ++k)
                {
                    frame[fn.params[k]] = (k < argv.size()) ? argv[k]
                                                            : DataType();
                }

                m_scopes.push_back(frame);
                DataType ret;
                ret.type = DataType::typeDbl;
                ret.dbl  = (Double)0;
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
            return Assign(p->u.opr.op[0], ex(p->u.opr.op[1]));
            
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
                    d.dbl  = op0.dbl + op1.dbl;
                }
                else if ((op0.type == DataType::typeStr) && 
                         (op1.type == DataType::typeStr))
                {
                    std::string str0 = std::string(op0.str);
                    std::string str1 = std::string(op1.str);
                    std::string str  = str0 + str1;
                    int len = (str.size() > MAX_STR_LEN) ? MAX_STR_LEN :
                                                           str.size();
                    // If the result has different type of data then
                    // default, please mention it.
                    d.type = DataType::typeStr;
                    // Copy data.
                    memset(d.str, 0, MAX_STR_LEN + 1);
                    memmove(d.str, str.c_str(), len);
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
    DataType                        sym['z' - 'a' + 1];
    std::map<std::string, DataType> varStr;

    // User-defined functions, by name.
    std::map<std::string, FuncDef>  funcs;

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

