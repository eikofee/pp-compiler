%{
	#include "ppc3ac.tab.h"
	#include "ppc3ac.h"
	#define YY_DECL int yylex()
%}
%%

"array of" { return T_ar; }
"new array of" { return NewAr; }
"boolean" { return T_boo; }
"integer" { return T_int; }
"defun" { return Def; }
"defpro" { return Dep; }
":=" { return Af; }
"skip" { return Sk; }
"true" { return V_TRUE; }
"false" { return V_FALSE; }
";" { return Se; }
"if" { return If; }
"then" { return Th; }
"else" { return El; }
"var" { return Var; }
"while" { return Wh; }
"do" { return Do; }
"+" { return Pl; }
"-" { return Mo; }
"*" { return Mu; }
"and" { return And; }
"or" { return Or; }
"not" { return Not; }
"<" { return Lt; }
"=" { return Eq; }
[ \t] { }
[\n] { incr_line(); }

"[" { return B_OP; }
"]" { return B_CL; }
"(" { return P_OP; }
")" { return P_CL; }
"{" { return A_OP; }
"}" { return A_CL; }
"," { return S_C; }
":" { return S_DP; }
"/*"[^("/*")]*"*/" { ;} /* (dirty) allows comments on input files */
[0-9]* { yylval.ival = atoi(yytext); return I; }
[a-zA-Z\_][a-zA-Z0-9\_]* { yylval.sval = strdup(yytext); return T_str; }

%%
