                       
#ifndef Boascript_OOYACC_H    
#define Boascript_OOYACC_H    
                       
#ifdef YYSTACKSIZE                      
    #undef YYMAXDEPTH                   
    #define YYMAXDEPTH YYSTACKSIZE      
#else                                   
    #ifdef YYMAXDEPTH                   
        #define YYSTACKSIZE YYMAXDEPTH  
    #else                               
        #define YYSTACKSIZE 500         
        #define YYMAXDEPTH 500          
    #endif                              
#endif                                  
                                        
#line 43 "src/Boascript.y"                                                             



#include <BTypes.h>
#include <DateTime.h>
#include <Tokenizer.h>
/*#include <BoascriptException.h>*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#if defined(BOS_OSX)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
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
    VAR_NAME_LEN  = 16
};


/* Main struct.*/
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

    /* Constants.*/
    struct conNodeType
    {
        DataType value;
    };

    /* Identifiers.*/
    struct idNodeType
    {
        int i; /* Subscript to sym array.*/
    };
    
    struct varNodeType
    {
        char name[VAR_NAME_LEN];
    };
     
    /* Operators.*/
    struct oprNodeType
    {
        int              oper;  /* Operator.*/
        int              nops;  /* Number of operands.*/
        struct nodeType* op[1]; /* Operands (expandable).*/
    };

    nodeEnum type; /* Type of node.*/
    /* Union must be last entry in nodeType*/
    /* because operNodeType may dynamically increase.*/
    union
    {
        conNodeType con; /* Constants.*/
        idNodeType  id;  /* Identifiers.*/
        varNodeType var;
        oprNodeType opr; /* Operators.*/
    } u;
};

#line 167 "src/Boascript.y"                                                             
#line 127 "Boascript.tab.cpp"                                                             
typedef union
{
    DataType  Value;
    char      sIndex;                /* symbol table index*/
    char      varName[VAR_NAME_LEN]; /* variable name*/
    nodeType* nPtr;                  /* node pointer*/
} YYSTYPE;

                                
class Boascript                        
{                               


enum Boascript_Tokens
{
	NUM		 = 257,
	STRING		 = 258,
	VARIABLE		 = 259,
	VARSTR		 = 260,
	WHILE		 = 261,
	IF		 = 262,
	PRINT		 = 263,
	PRINTLN		 = 264,
	IFN		 = 265,
	IFS		 = 266,
	STRLEN		 = 267,
	SUBSTR		 = 268,
	REPLACE		 = 269,
	SUBSTITUTE		 = 270,
	TOSTR		 = 271,
	TONUM		 = 272,
	ABS		 = 273,
	ACOS		 = 274,
	ASIN		 = 275,
	ATAN		 = 276,
	ATAN2		 = 277,
	CEIL		 = 278,
	COS		 = 279,
	COSH		 = 280,
	EXP		 = 281,
	FABS		 = 282,
	FLOOR		 = 283,
	FMOD		 = 284,
	FREXP		 = 285,
	LDEXP		 = 286,
	LOG		 = 287,
	LOG10		 = 288,
	MODF		 = 289,
	POW		 = 290,
	SIN		 = 291,
	SINH		 = 292,
	SQRT		 = 293,
	CBRT		 = 294,
	TAN		 = 295,
	TANH		 = 296,
	MIN		 = 297,
	MAX		 = 298,
	RAND		 = 299,
	DATE		 = 300,
	MATCH		 = 301,
	PI		 = 302,
	SETPREC		 = 303,
	GETPREC		 = 304,
	COMMENT		 = 305,
	EXIT		 = 306,
	IFX		 = 307,
	ELSE		 = 308,
	PLUS_ASSIGN		 = 309,
	MINUS_ASSIGN		 = 310,
	MUL_ASSIGN		 = 311,
	DIV_ASSIGN		 = 312,
	MOD_ASSIGN		 = 313,
	AND		 = 314,
	OR		 = 315,
	EQ		 = 316,
	NE		 = 317,
	GE		 = 318,
	LE		 = 319,
	SHR		 = 320,
	SHL		 = 321,
	PREF_INC		 = 322,
	PREF_DEC		 = 323,
	UMINUS		 = 324,
	BNE		 = 325
};

                                
public:                         
                                
    Boascript(void* pExtraData = 0);   
    int      yyparse();         
    YYSTYPE* getLVal();         
                                
private:                        
                                
#line 319 "src/Boascript.y"                                                             



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
    memset(p->u.con.value.str, 0, MAX_STR_LEN);
    memmove(p->u.con.value.str, value.str, len);
    p->u.con.value.type =  DataType::typeStr;

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

    return p;
}


nodeType* setVar(std::string name)
{
#ifdef VARSTR_DEBUG
		std::cout << "setVar name = " << name << std::endl; 
#endif
    nodeType* p = 0;
    // Allocate node.
    size_t nodeSize = SIZEOF_NODETYPE + sizeof(nodeType::idNodeType);
    if ((p = (nodeType *)malloc(nodeSize)) == 0)
    {
        yyerror("out of memory");
    }

    // copy information.
    p->type = nodeType::typeVar;
    ::memset(&p->u.var.name[0], 0, VAR_NAME_LEN);
    ::memmove(&p->u.var.name[0], name.c_str(), name.size());

#ifdef VARSTR_DEBUG
		std::cout << "setVar p->u.var.name = " << p->u.var.name << std::endl; 
#endif
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
    m_inBuf     = 0;

    precision = 7;
}


void Load(std::string in)
{
    int len = in.size() + 1;
    if (!m_inBuf)
    {
        m_inBuf = (char *)malloc(len);
    }

    if (m_inBuf)
    {
        memset(m_inBuf, 0, len);
        memmove(m_inBuf, in.c_str(), len - 1);
    }
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
}


#endif // CALC_BATCH


private:


int GetChar(void)
{

#ifndef CALC_BATCH
    return getchar();
#else
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
    --m_bufInd;
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

    if (cp == EOF)
    {
       	return 0;
    }

    // String section.
    if (cp == '"')
    {
        memset(yylval.Value.str, 0, MAX_STR_LEN);
        int n = 0;

        while ((cp = GetChar()) != '"')
        {
            yylval.Value.str[n++] = cp;
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
        memset(buf, 0, SMALL_BUF_LEN);
        int n = 0;
	    int i = 0;
        while (cp != EOF && (::isalpha(cp) || ::isdigit(cp)))
	    {
            buf[n++] = cp;
            ++i;
            cp = GetChar();
        }
        UngetChar(cp);

        if (i == 1)
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
                {
                    std::ostringstream os;
                    os << buf << " : ";
                    yyerror("", os.str());
                }
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
        return sym[p->u.id.i];
        
    case nodeType::typeVar:
#ifdef VARSTR_DEBUG
        std::cout << "p->u.var.name = " << p->u.var.name << std::endl; 
#endif
        if (varStr.find(p->u.var.name) == varStr.end())
        {
            varStr[p->u.var.name] = d;
        }
        return varStr[p->u.var.name];
    
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
            return sym[p->u.opr.op[0]->u.id.i] = ex(p->u.opr.op[1]);
            
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
                    memset(d.str, 0, MAX_STR_LEN);
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
            sym[p->u.opr.op[0]->u.id.i] = d;
            return d;
                
        case MINUS_ASSIGN:
            d.dbl = ex(p->u.opr.op[0]).dbl - ex(p->u.opr.op[1]).dbl;
            sym[p->u.opr.op[0]->u.id.i] = d;
            return d;
                
        case MUL_ASSIGN:
            d.dbl = ex(p->u.opr.op[0]).dbl * ex(p->u.opr.op[1]).dbl;
            sym[p->u.opr.op[0]->u.id.i] = d;
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
                sym[p->u.opr.op[0]->u.id.i] = d;
            }
            return d;
               
        case MOD_ASSIGN:
            {
                DataType op0 = ex(p->u.opr.op[0]);
                DataType op1 = ex(p->u.opr.op[1]);
                if (op1.dbl)
                {
                    d.dbl = (Double)((int)op0.dbl / (int)op1.dbl);
                }
                else
                {
                    d.dbl = (Double)0;
                }
                sym[p->u.opr.op[0]->u.id.i] = d;
            }
            return d;
     
        case PREF_INC:
            d.dbl = ex(p->u.opr.op[0]).dbl + (Double)1;
            sym[p->u.opr.op[0]->u.id.i] = d;
            return d;

        case PREF_DEC:
            d.dbl = ex(p->u.opr.op[0]).dbl - (Double)1;
            sym[p->u.opr.op[0]->u.id.i] = d;
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
                
        // This is not exact solution for the cube root.
        // But very fast and good for approximations.
        // Experimental.
        case CBRT:
            {
                Double arg = (Double)ex(p->u.opr.op[0]).dbl;
                const unsigned int B1 = 715094163; 
                Double t = (Double)0; 
                unsigned int* pt = (unsigned int *)&t; 
                unsigned int* px = (unsigned int *)&arg;
                pt[isLittleEndian()] = px[isLittleEndian()] / 3 + B1; 
                d.dbl = t;
            }
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
                memset(d.str, 0, MAX_STR_LEN);
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
                memset(d.str, 0, MAX_STR_LEN);
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
                    memset(d.str, 0, MAX_STR_LEN);
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
                memset(d.str, 0, MAX_STR_LEN);
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
                memset(d.str, 0, MAX_STR_LEN);
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
                            memset(d.str, 0, MAX_STR_LEN);
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
                            memset(d.str, 0, MAX_STR_LEN);
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

#ifdef CALC_BATCH

    int   m_bufInd;
    char* m_inBuf;
    Cell  m_outBuf;

#endif // CALC_BATCH

#line 1625 "Boascript.tab.cpp"                                                             
                                
private:                        
                                
    int      yydebug;           
    int      yynerrs;           
    int      yyerrflag;         
    int      yychar;            
    short*   yyssp;             
    YYSTYPE* yyvsp;             
    YYSTYPE  yyval;             
    YYSTYPE  yylval;            
    short    yyss[YYSTACKSIZE]; 
    YYSTYPE  yyvs[YYSTACKSIZE]; 
    int      yym,yyn,yystate;   
    char*    yys;               
                                
};


#endif // Boascript_OOYACC_H
