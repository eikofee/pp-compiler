#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ppina.h"

#define ERR_BUFFER_SIZE 256

pp_func f_context = NULL;
pp_func f_root = NULL;
pp_func f_current = NULL;

int in_fuction_call = 0;
int var_declaration = 0;
int total_errors = 0;
int line_position = 1;

pp_var current_arg = NULL;

pp_context main_context = NULL;

pp_stack base = NULL;

void stack_push(pp_value value)
{
	pp_stack s = (pp_stack) malloc(sizeof(struct s_pp_stack));
	s->value = value;
	
	s->prev = &(*base);
	base = s;
}

pp_value stack_pop()
{
	pp_stack a = &(*base);
	base = a->prev;
	
	return a->value;
}

void incr_line()
{
	line_position++;
}

pp_type syna_create_type(pp_type_id type, pp_type next)
{
	pp_type t = (pp_type) malloc(sizeof(struct s_pp_type));
	t->type = type;
	t->next = next;
	
	return t;
}

pp_value env_create_value(pp_type type, int value, pp_value next)
{
	pp_value v = (pp_value) malloc(sizeof(struct s_pp_value));
	v->type = type;
	v->value = value;
	v->members_count = 0;
	v->members = NULL;
	
	return v;
}

pp_value env_create_array(int size, pp_type type)
{
	pp_value value = env_create_value(syna_create_type(ARRAY, type), 0, NULL);
	value->members = (pp_value*) malloc(sizeof(pp_value) * size);
	value->members_count = size;
	for (int i = 0; i < size; i++)
	value->members[i] = env_create_value(type, 0, NULL);
	
	return value;
}

pp_var exe_add_variable(char* name, pp_context context, pp_type type)
{
	int new = 0;
	pp_var v = exe_get_variable(name, context);
	if (v == NULL)
	new = 1;
	
	if (new)
	v = (pp_var) malloc(sizeof(struct s_pp_var));
	
	v->name = strdup(name);
	//v->next = NULL; //Only value matters
	v->value = env_create_value(type, 0, NULL);
	v->type = type;
	v->func = 0;
	
	if (new)
	{
		
		if (context->current_context != NULL)
		context->current_context->next = v;
		
		if (context->context == NULL)
		context->context = v;
		
		context->current_context = v;
	}
	
	return v;
}

pp_var env_add_variable(char* name, pp_type type)
{
	pp_var v = (pp_var) malloc(sizeof(struct s_pp_var));
	v->name = strdup(name);
	v->type = type;
	v->next = NULL;
	v->value = env_create_value(type, 0, NULL);
	
	if (f_context->context_current != NULL)
	f_context->context_current->next = v;
	
	if (f_context->context == NULL)
	f_context->context = v;
	
	f_context->context_current = v;
	
	return v;
}

pp_var lcl_add_variable(pp_func func, char* name, pp_value value)
{
	pp_var v = (pp_var) malloc(sizeof(struct s_pp_var));
	v->name = strdup(name);
	v->type = value->type;
	v->next = NULL;
	v->value = env_create_value(value->type, 0, NULL);
	if (func->args_current != NULL)
	func->args_current->next = v;
	
	if (func->args == NULL)
	func->args = v;
	
	func->args_current = v;
	
	return v;
}

pp_func env_add_function(char* name, pp_type ret_type, pp_var args)
{
	pp_func f = (pp_func) malloc(sizeof(struct s_pp_func));
	f->name = strdup(name);
	f->ret_type = ret_type;
	f->args = args;
	f->args_current = f->args;
	f->context = NULL;
	f->context_current = NULL;
	f->body = NULL;
	if (f_current != NULL)
	f_current->next = f;
	
	if (f_root == NULL)
	f_root = f;
	
	f_current = f;
	
	return f;
}

pp_var env_add_lcl_variable(pp_var lcl_parent, char* name, pp_type type)
{
	pp_var v =  (pp_var) malloc(sizeof(struct s_pp_var));
	v->name = strdup(name);
	v->type = type;
	v->next = NULL;
	
	if (lcl_parent != NULL)
	lcl_parent->next = v;
	
	return v;
}

pp_func env_get_function(char* context_name, int decl)
{
	pp_func f = f_root;
	while (f != NULL && strcmp(f->name, context_name))
	f = f->next;
	
	if (f == NULL && !decl)
	{
		char* s = (char*) malloc(sizeof(char) * ERR_BUFFER_SIZE);
		sprintf(s, "Symbol '%s' not found, return type considered as null", context_name);
		err_display(s);
	}
	
	return f;
}

int env_get_args_number(pp_func f)
{
	int result = 0;
	pp_var v = f->args;
	while(v != NULL)
	{
		result++;
		v = v->next;
	}
	
	return result - 1;
}

pp_var exe_get_variable(char* name, pp_context context)
{
	pp_var v = context->context;
	while (v != NULL && strcmp(v->name, name))
	v = v->next;
	
	if (v == NULL)
	{
		if (context->tmp_context != NULL)
		{
			v = context->tmp_context;
			while (v != NULL && strcmp(v->name, name))
			v = v->next;
		}
	}
	
	return v;
}

pp_var env_get_variable(char* name, int decl, pp_var args)
{
	pp_func c = f_context;
	char* prev_context = strdup(c->name);
	pp_var v = c->args;
	int i = 0;
	while (v != NULL && strcmp(v->name, name))
	{
		v = v->next;
		i++;
	}
	
	if (v != NULL)
	{
		if (args == NULL)
		return v;
		else
		{
			pp_var r = args;
			while (i > 0)
			{
				r = r->next;
				i--;
			}
			
			return r;
		}
	}
	
	v = c->context;
	while (v != NULL && strcmp(v->name, name))
	v = v->next;
	
	if (v != NULL)
	return v;
	
	v = f_context->context;
	while (v != NULL && strcmp(v->name, name))
	v = v->next;
	
	if (v == NULL && !decl)
	{
		char* s = (char*) malloc(sizeof(char) * ERR_BUFFER_SIZE);
		sprintf(s, "Variable '%s' not found (current context : '%s')\n", name, prev_context);
		err_display(s);
	}
	
	if (v == NULL)
	v = env_add_variable(name, NONE);
	
	return v;
}

void display_args(pp_var lcl_root, int rank)
{
	if (lcl_root != NULL)
	{
		if (lcl_root->next != NULL)
		display_args(lcl_root->next, rank + 1);
		
		printf("%s :", lcl_root->name);
		pp_type t_current = lcl_root->type;
		while (t_current != NULL)
		{
			char s[9];
			switch (t_current->type) {
				case NONE:
				sprintf(s, "none");
				break;
				
				case INT:
				sprintf(s, "integer");
				break;
				
				case BOOL:
				sprintf(s, "boolean");
				break;
				
				case ARRAY:
				sprintf(s, "array of");
				break;
			}
			
			printf(" %s", s);
			t_current = t_current->next;
		}
		
		if (rank > 0)
		printf(", ");
	}
}

//AST

syna_node syna_create_node(int num_childs)
{
	syna_node n = (syna_node) malloc(sizeof(struct s_syna_node));
	n->type = NEMPTY;
	n->value = NULL;
	n->variable = NULL;
	n->function = NULL;
	n->value = NULL;
	n->opi = INONE;
	n->opb = BNONE;
	n->string = NULL;
	n->line_position = line_position;
	n->childs = (syna_node*) malloc(sizeof(struct s_syna_node) * num_childs);
	return n;
}

syna_node syna_opi_node(syna_node member_left, syna_node member_right, syna_opi op)
{
	syna_node n = syna_create_node(2);
	n->type = NOPI;
	n->childs[0] = member_left;
	n->childs[1] = member_right;
	n->value = env_create_value(syna_create_type(INT, NULL), 0, NULL);
	n->opi = op;
	
	return n;
}

syna_node syna_opb_node(syna_node member_left, syna_node member_right, syna_opb op)
{
	syna_node n = syna_create_node(2);
	n->type = NOPB;
	n->childs[0] = member_left;
	n->childs[1] = member_right;
	n->value = env_create_value(syna_create_type(BOOL, NULL), 0, NULL);
	n->opb = op;
	
	return n;
}

syna_node syna_p_node(syna_node content)
{
	syna_node n = syna_create_node(1);
	n->type = NPBA;
	n->childs[0] = content;
	
	return n;
}

syna_node syna_int_node(int value)
{
	syna_node n = syna_create_node(0);
	n->type = NVALUE;
	n->value = env_create_value(syna_create_type(INT, NULL), value, NULL);
	return n;
}

syna_node syna_var_node(char* name)
{
	syna_node n = syna_create_node(0);
	n->type = NVAR;
	n->string = strdup(name);
	
	return n;
}

syna_node syna_bool_node(int value)
{
	syna_node n = syna_create_node(0);
	n->type = NVALUE;
	n->value = env_create_value(syna_create_type(BOOL, NULL), value, NULL);
	
	return n;
}

syna_node syna_array_node(syna_node member, syna_node index)
{
	//Find a way to get an object from an array 1d+
	syna_node n = syna_create_node(2);
	n->type = NARRAY;
	n->childs[0] = member;
	n->childs[1] = index;
	
	return n;
}

syna_node syna_branch_node(syna_node left, syna_node right)
{
	syna_node n = syna_create_node(2);
	n->type = NBRANCH;
	n->childs[0] = left;
	n->childs[1] = right;
	
	return n;
}

syna_node syna_ITE_node(syna_node cond, syna_node th, syna_node el)
{
	syna_node n = syna_create_node(3);
	n->type = NITE;
	n->childs[0] = cond;
	n->childs[1] = th;
	n->childs[2] = el;
	
	return n;
}

syna_node syna_WD_node(syna_node cond, syna_node d)
{
	syna_node n = syna_create_node(2);
	n->type = NWD;
	n->childs[0] = cond;
	n->childs[1] = d;
	
	return n;
}

syna_node syna_aaf_node(syna_node dest, syna_node value)
{
	syna_node n = syna_create_node(2);
	n->type = NAAF;
	n->childs[0] = dest;
	n->childs[1] = value;
	
	return n;
}

syna_node syna_vaf_node(syna_node dest, syna_node value)
{
	syna_node n = syna_create_node(2);
	n->type = NVAF;
	n->childs[0] = dest;
	n->childs[1] = value;
	
	return n;
}

syna_node syna_skip_node()
{
	syna_node n = syna_create_node(0);
	n->type = NSKIP;
	
	return n;
}

syna_node syna_a_node(syna_node content)
{
	syna_node n = syna_create_node(1);
	n->type = NPBA;
	n->childs[0] = content;
	
	return n;
}

syna_node syna_empty_node()
{
	syna_node n = syna_create_node(0);
	
	return n;
}

syna_node syna_expr_node(syna_node expr)
{
	syna_node n = syna_create_node(1);
	n->type = NEXPR;
	n->childs[0] = expr;
	
	return n;
}

syna_node syna_vdef_node(syna_node dest, syna_node type)
{
	syna_node n = syna_create_node(2);
	n->type = NVDEF;
	n->childs[0] = dest;
	n->childs[1] = type;
	n->value = type->value;
	
	return n;
}

syna_node syna_type_node(pp_type_id type, syna_node next)
{
	syna_node n = syna_create_node(1);
	n->type = NTYPE;
	n->childs[0] = next;
	n->value = env_create_value(syna_create_type(type, NULL), 0, NULL);
	return n;
}

syna_node syna_pdef_node(char* name, syna_node args)
{
	syna_node n = syna_create_node(1);
	n->type = NPDEF;
	n->string = strdup(name);
	n->childs[0] = args;
	
	return n;
}

syna_node syna_fdef_node(char* name, syna_node args, syna_node ret)
{
	syna_node n = syna_create_node(2);
	n->type = NFDEF;
	n->string = strdup(name);
	n->childs[0] = args;
	n->childs[1] = ret;
	
	return n;
}

syna_node syna_root_node(syna_node vart, syna_node ld, syna_node c)
{
	syna_node n = syna_create_node(3);
	n->type = NROOT;
	n->childs[0] = vart;
	n->childs[1] = ld;
	n->childs[2] = c;
	
	return n;
}

syna_node syna_new_var_node(char* name)
{
	syna_node n = syna_create_node(0);
	n->type = NNVAR;
	n->string = strdup(name);
	
	return n;
}

syna_node syna_pbody_node(syna_node def, syna_node def_vars, syna_node body)
{
	syna_node n = syna_create_node(3);
	n->type = NPBODY;
	n->childs[0] = def;
	n->childs[1] = def_vars;
	n->childs[2] = body;
	
	return n;
}

syna_node syna_fbody_node(syna_node def, syna_node def_vars, syna_node body)
{
	syna_node n = syna_create_node(3);
	n->type = NFBODY;
	n->childs[0] = def;
	n->childs[1] = def_vars;
	n->childs[2] = body;
	
	return n;
}

syna_node syna_call_func_node(char* name, syna_node args)
{
	syna_node n = syna_create_node(1);
	n->type = NFPCALL;
	n->string = strdup(name);
	n->childs[0] = args;
	
	return n;
}

syna_node syna_newarray_node(syna_node type, syna_node expr)
{
	syna_node n = syna_create_node(2);
	n->type = NNA;
	n->childs[0] = type;
	n->childs[1] = expr;
	return n;
}

void syna_link_args_to_func(pp_func func, syna_node args)
{
	switch (args->type) {
		case NBRANCH:
		syna_link_args_to_func(func, args->childs[1]);
		syna_link_args_to_func(func, args->childs[0]);
		break;
		
		case NVDEF:
		syna_execute(args->childs[0], NULL);
		syna_execute(args->childs[1], NULL);
		lcl_add_variable(func, args->childs[0]->string, args->childs[1]->value);
		break;
	}
}

void env_link_arguments(syna_node root, pp_context current_context, pp_context context, pp_func f)
{
	switch (root->type) {
		case NBRANCH:
		env_link_arguments(root->childs[1], current_context, context,f);
		env_link_arguments(root->childs[0], current_context, context,f);
		break;
		
		case NEMPTY:
		break;
		
		default:
		syna_execute(root, current_context);
		pp_var n = exe_add_variable(f->args_current->name, context, root->value->type);
		n->scope = LOCAL;
		n->value->value = root->value->value;
		n->value->members = root->value->members;
		n->value->members_count = root->value->members_count;
		f->args_current = f->args_current->next;
		break;
	}
	
}

char* err_display_type(pp_type type)
{
	char* result = (char*) malloc(sizeof(char) * ERR_BUFFER_SIZE);
	while (type != NULL)
	{
		char* s;
		switch (type->type) {
			case NONE:
			s = strdup("null");
			break;
			
			case INT:
			s = strdup("integer");
			break;
			
			case BOOL:
			s = strdup("boolean");
			break;
			
			case ARRAY:
			s = strdup("array of");
			break;
		}
		
		sprintf(result, "%s %s", result, s);
		type = type->next;
	}
	
	return result;
}

int err_check_type_rec(pp_type n, pp_type type)
{
	if (n->type != type->type)
	return 0;
	
	if (type->type == ARRAY)
	return err_check_type_rec(n->next, type->next);
	
	return 1;
	
}

int err_check_type(pp_type n, pp_type type)
{
	int e = err_check_type_rec(n, type);
	if (!e)
	{
		char* s = (char*) malloc(sizeof(char) * ERR_BUFFER_SIZE);
		sprintf(s, "Incorrect type : expected type%s but got%s.", err_display_type(type), err_display_type(n));
		err_display(s);
	}
	
	return e;
}

void err_check_single_argument(syna_node arg, pp_func f, int index, int max)
{
	pp_var v = f->args;
	int i = 0;
	while (i < max - index)
	{
		i++;
		v = v->next;
	}
	
	err_check_type(arg->value->type, v->type);
}

int err_check_arguments(syna_node arg_node, pp_context context, pp_func f, int rank, int rank_max, int check)
{
	if (arg_node->type == NEMPTY)
	rank--;
	
	int b1 = (rank == rank_max?1:0);
	int b2 = 0;
	
	if (rank > rank_max)
	{
		char* s = (char*) malloc(sizeof(char) * ERR_BUFFER_SIZE);
		sprintf(s, "Too many arguments : expected %d arguments for %s.", rank_max + 1, f->name);
		err_display(s);
		return 1;
	}
	
	switch (arg_node->type) {
		case NBRANCH:
		b1 = err_check_arguments(arg_node->childs[0], context, f, rank, rank_max, check);
		rank++;
		b2 = err_check_arguments(arg_node->childs[1], context, f, rank, rank_max, check);
		break;
		case NEMPTY:
		return b1;
		break;
		default:
		if (check)
		syna_check(arg_node, context);
		else
		syna_execute(arg_node, context);
		
		err_check_single_argument(arg_node, f, rank, rank_max);
		break;
	}
	
	return b1 + b2;
}

pp_context exe_create_context()
{
	pp_context c = (pp_context) malloc(sizeof(struct s_pp_context));
	c->context = NULL;
	c->current_context = NULL;
	c->return_value = NULL;
	
	if (main_context == NULL)
	main_context = c;
	
	return c;
}

pp_context exe_copy_context(pp_context context)
{
	pp_context c = exe_create_context();
	pp_var v = context->context;
	while (v != NULL)
	{
		if (v->scope != LOCAL)
		{
			pp_var vd = (pp_var) malloc(sizeof(struct s_pp_var));
			vd->name = v->name;
			vd->type = v->type;
			vd->scope = v->scope;
			vd->next = NULL;
			vd->value = v->value;
			
			if (c->current_context != NULL)
			c->current_context->next = vd;
			
			if (c->context == NULL)
			c->context = vd;
			
			c->current_context = vd;
		}
		v = v->next;
	}
	
	return c;
}

void syna_execute(syna_node root, pp_context context)
{
	line_position = root->line_position;
	switch (root->type) {
		case NEMPTY:
		//eh
		break;
		
		case NROOT:
		var_declaration = 1;
		syna_execute(root->childs[0], context);
		var_declaration = 0;
		pp_context nc = exe_copy_context(context);
		syna_execute(root->childs[1], nc);
		//fprintf(stderr, "exec\n");
		syna_execute(root->childs[2], nc);
		break;
		
		case NOPI:
		{
			
			syna_execute(root->childs[0], context);
			pp_var __a__ = (pp_var) malloc(sizeof(struct s_pp_var));
			__a__->value = env_create_value(syna_create_type(INT, NULL), root->childs[0]->value->value, NULL);
			__a__->value->value = root->childs[0]->value->value;
			stack_push(__a__->value);
			syna_execute(root->childs[1], context);
			pp_var __b__ = (pp_var) malloc(sizeof(struct s_pp_var));
			__b__->value = env_create_value(syna_create_type(INT, NULL), root->childs[1]->value->value, NULL);
			__b__->value->value = root->childs[1]->value->value;
			stack_push(__b__->value);
			if (	err_check_type(root->childs[0]->value->type, syna_create_type(INT, NULL))
			&&	err_check_type(root->childs[1]->value->type, syna_create_type(INT, NULL)))
			{
				int b = stack_pop()->value;
				int a = stack_pop()->value;
				int c;
				switch (root->opi) {
					case PL:
					c = a + b;
					break;
					
					case MO:
					c = a - b;
					break;
					
					case MU:
					c = a * b;
					break;
				}
				
				pp_value v = env_create_value(syna_create_type(INT, NULL), c, NULL);
				root->value = v;
			}
		}
		break;
		
		case NOPB:
		{
			if (root->opb != NOT)
			{
				syna_execute(root->childs[0], context);
				pp_var __a__ = (pp_var) malloc(sizeof(struct s_pp_var));
				__a__->value = env_create_value(syna_create_type(INT, NULL), root->childs[0]->value->value, NULL);
				__a__->value->value = root->childs[0]->value->value;
				stack_push(__a__->value);
			}
			
			syna_execute(root->childs[1], context);
			pp_var __b__ = (pp_var) malloc(sizeof(struct s_pp_var));
			__b__->value = env_create_value(syna_create_type(INT, NULL), root->childs[1]->value->value, NULL);
			__b__->value->value = root->childs[1]->value->value;
			stack_push(__b__->value);
			
			int err_status = 1;
			err_status = (root->childs[1]->value != NULL ? 1 : 0);
			switch (root->opb) {
				case OR:
				case AND:
				err_status *= err_check_type(root->childs[0]->value->type, syna_create_type(BOOL, NULL));
				case NOT:
				err_status *= err_check_type(root->childs[1]->value->type, syna_create_type(BOOL, NULL));			
				break;
				
				case LT:
				case EQ:
				err_status *= err_check_type(root->childs[0]->value->type, syna_create_type(INT, NULL))
				* err_check_type(root->childs[1]->value->type, syna_create_type(INT, NULL));
				break;			
			}
			
			if (err_status)
			{
				int b = stack_pop()->value;
				int a = 0;
				if (root->opb != NOT)
				a = stack_pop()->value;
				
				int c;
				switch (root->opb) {
					case OR:
					c = a + b;
					break;
					
					case AND:
					c = a * b;
					break;
					
					case NOT:
					c = (b == 0 ? 1 : 0);
					break;
					
					case LT:
					c = (a < b ? 1 : 0);
					break;
					
					case EQ:
					c = (a == b ? 1 : 0);
					break;
				}
				
				pp_value v = env_create_value(syna_create_type(BOOL, NULL), c, NULL);
				root->value = v;
			}else{
				err_display("Cannot evalue expression (are members correctly initialized ?)");
				root->value == NULL;
			}
		}
		break;
		
		case NPBA:
		syna_execute(root->childs[0], context);
		root->value = root->childs[0]->value;
		break;
		
		case NVALUE:
		break;
		
		case NVAR:
		{
			pp_var v = exe_get_variable(root->string, context);
			if (v->func)
			err_display("Symbol is a function but is used as a variable");
			
			root->variable = v;
			root->value = v->value;
		}
		break;
		
		case NARRAY:
		syna_execute(root->childs[1], context);
		err_check_type(root->childs[1]->value->type, syna_create_type(INT, NULL));
		syna_execute(root->childs[0], context);
		root->value = env_create_value(root->childs[0]->value->type->next, 0, NULL);
		if (root->childs[0]->value == NULL)
		err_display("Value has not been initialized");
		
		if (root->childs[0]->value->members_count <= root->childs[1]->value->value)
		{
			char* s = (char*) malloc(sizeof(char) * ERR_BUFFER_SIZE);
			sprintf(s, "Index is out of bounds, array is of size %d but index is %d%s", root->childs[0]->value->members_count
			, root->childs[1]->value->value, (root->childs[0]->value->members_count == 0 ? " (is the array correctly initialized ?)": ""));
			err_display(s);
		}
		else
		root->value = root->childs[0]->value->members[root->childs[1]->value->value];
		break;
		
		case NNA:
		syna_execute(root->childs[0], context);
		syna_execute(root->childs[1], context);
		if (err_check_type(root->childs[1]->value->type, syna_create_type(INT, NULL)))
		root->value = env_create_array(root->childs[1]->value->value, root->childs[0]->value->type);
		break;
		
		case NBRANCH:
		syna_execute(root->childs[0], context);
		syna_execute(root->childs[1], context);
		break;
		
		case NITE:
		syna_execute(root->childs[0], context);
		syna_check(root->childs[1], context);
		syna_check(root->childs[2], context);
		if (err_check_type(root->childs[0]->value->type, syna_create_type(BOOL, NULL)))
		{
			if (root->childs[0]->value->value > 0)
			syna_execute(root->childs[1], context);
			else
			syna_execute(root->childs[2], context);
		}
		break;
		
		case NWD:
		syna_execute(root->childs[0], context);
		syna_check(root->childs[1], context);
		if (root->childs[0]->value != NULL && root->childs[0]->value->type != NULL && err_check_type(root->childs[0]->value->type, syna_create_type(BOOL, NULL)))
		while (root->childs[0]->value != NULL && root->childs[0]->value->value > 0)
		{
			syna_execute(root->childs[1], context);
			syna_execute(root->childs[0], context);
		}
		
		break;
		
		case NAAF:
		syna_execute(root->childs[0], context);
		syna_execute(root->childs[1], context);
		if (err_check_type(root->childs[1]->value->type, root->childs[0]->value->type))
		{
			if (root->childs[1]->value->type->type != ARRAY)
			root->childs[0]->value->value = root->childs[1]->value->value;
			else
			{
				root->childs[0]->value->members = root->childs[1]->value->members;
				root->childs[0]->value->members_count = root->childs[1]->value->members_count;
			}
		}
		break;
		
		case NVAF:
		syna_execute(root->childs[1], context);
		syna_execute(root->childs[0], context);
		if (err_check_type(root->childs[1]->value->type, root->childs[0]->value->type))
		{
			root->childs[0]->variable->value->value = root->childs[1]->value->value;
			root->childs[0]->variable->value->members = root->childs[1]->value->members;
			root->childs[0]->variable->value->members_count = root->childs[1]->value->members_count;
		}
		
		break;
		
		case NSKIP:
		//eh	
		break;
		
		case NVDEF:
		root->childs[0]->variable = exe_add_variable(root->childs[0]->string, context, root->value->type);
		if (root->childs[1] != NULL)
		syna_execute(root->childs[1], context);
		
		root->childs[0]->variable->type = root->value->type;
		break;
		
		case NTYPE:
		if (root->childs[0] != NULL)
		{
			syna_execute(root->childs[0], context);
			root->value->type->next = root->childs[0]->value->type;
		}
		break;
		
		case NPDEF:
		root->function = env_add_function(root->string, syna_create_type(NONE, NULL), root->variable);
		syna_link_args_to_func(root->function, root->childs[0]);
		break;
		
		case NFDEF:
		syna_execute(root->childs[1], context);
		root->value = root->childs[1]->value;
		root->function = env_add_function(root->string, root->value->type, root->variable);
		syna_link_args_to_func(root->function, root->childs[0]);
		break;
		
		case NPBODY:
		var_declaration = 1;
		syna_execute(root->childs[0], context);
		syna_execute(root->childs[1], context);
		var_declaration = 0;
		root->childs[0]->function->body = root->childs[2];
		break;
		
		case NFBODY:
		var_declaration = 1;
		syna_execute(root->childs[0], context);
		syna_execute(root->childs[1], context);
		var_declaration = 0;
		root->childs[0]->function->body = root->childs[2];
		context->tmp_context = root->childs[0]->function->args;
		pp_var ret = exe_add_variable(root->childs[0]->function->name, context, root->childs[0]->function->ret_type);
		ret->scope = LOCAL;
		syna_check(root->childs[2], context);
		ret->func = 1;
		break;
		
		case NFPCALL:
		{
			pp_func f = env_get_function(root->string, 0);
			root->value = env_create_value(f->ret_type, 0, NULL);
			if (!err_check_arguments(root->childs[0], context, f, 0, env_get_args_number(f), 0))
			{
				char* s = (char*) malloc(sizeof(char) * ERR_BUFFER_SIZE);
				sprintf(s, "Too few arguments for %s", f->name);
				err_display(s);
			}else{
				pp_context new_context = exe_copy_context(context);
				f->args_current = f->args;
				env_link_arguments(root->childs[0], context, new_context, f);
				pp_var ret = exe_add_variable(f->name, new_context, f->ret_type);
				ret->scope = LOCAL;
				syna_execute(f->body, new_context);
				root->value->value = ret->value->value;
			}
		}
		break;
	}
}

void syna_check(syna_node root, pp_context context)
{
	line_position = root->line_position;
	switch (root->type) {
		case NEMPTY:
		//eh
		break;
		
		case NROOT:
		var_declaration = 1;
		syna_check(root->childs[0], context);
		var_declaration = 0;
		syna_check(root->childs[1], context);
		syna_check(root->childs[2], context);
		break;
		
		case NOPI:
		syna_check(root->childs[0], context);
		syna_check(root->childs[1], context);
		err_check_type(root->childs[0]->value->type, syna_create_type(INT, NULL));
		err_check_type(root->childs[1]->value->type, syna_create_type(INT, NULL));
		break;
		
		case NOPB:
		if (root->opb != NOT)
		syna_check(root->childs[0], context);
		
		syna_check(root->childs[1], context);
		int err_status = 1;
		switch (root->opb) {
			case OR:
			case AND:
			err_status = err_check_type(root->childs[0]->value->type, syna_create_type(BOOL, NULL));
			case NOT:
			err_status = err_check_type(root->childs[1]->value->type, syna_create_type(BOOL, NULL));			
			break;
			
			case LT:
			case EQ:
			err_status = err_check_type(root->childs[0]->value->type, syna_create_type(INT, NULL))
			* err_check_type(root->childs[1]->value->type, syna_create_type(INT, NULL));
			break;			
		}
		break;
		
		case NPBA:
		syna_check(root->childs[0], context);
		root->value = root->childs[0]->value;
		break;
		
		case NVALUE:
		break;
		
		case NVAR:
		{
			pp_var v = exe_get_variable(root->string, context);
			if (v->func)
			err_display("Symbol is a function but is used as a variable");
			root->value = env_create_value(v->value->type, v->value->value, NULL);	
		}
		break;
		
		case NARRAY:
		syna_check(root->childs[1], context);
		err_check_type(root->childs[1]->value->type, syna_create_type(INT, NULL));
		syna_check(root->childs[0], context);
		root->value = env_create_value(root->childs[0]->value->type->next, 0, NULL);
		break;
		
		case NNA:
		syna_check(root->childs[0], context);
		syna_check(root->childs[1], context);
		err_check_type(root->childs[1]->value->type, syna_create_type(INT, NULL));
		root->value = env_create_value(syna_create_type(ARRAY, root->childs[0]->value->type), 0, NULL);
		break;
		
		case NBRANCH:
		syna_check(root->childs[0], context);
		syna_check(root->childs[1], context);
		break;
		
		case NITE:
		syna_check(root->childs[0], context);
		err_check_type(root->childs[0]->value->type, syna_create_type(BOOL, NULL));
		syna_check(root->childs[1], context);
		syna_check(root->childs[2], context);
		break;
		
		case NWD:
		syna_check(root->childs[0], context);
		syna_check(root->childs[1], context);
		err_check_type(root->childs[0]->value->type, syna_create_type(BOOL, NULL));
		break;
		
		case NAAF:
		syna_check(root->childs[0], context);
		syna_check(root->childs[1], context);
		err_check_type(root->childs[1]->value->type, root->childs[0]->value->type);
		break;
		
		case NNVAR:
		break;
		
		case NVAF:
		syna_check(root->childs[1], context);
		syna_check(root->childs[0], context);
		err_check_type(root->childs[1]->value->type, root->childs[0]->value->type);
		break;
		
		case NSKIP:
		//eh	
		break;
		
		case NEXPR:
		
		break;
		
		case NVDEF:
		root->childs[0]->variable = env_get_variable(root->childs[0]->string, var_declaration, NULL);
		if (root->childs[1] != NULL)
		syna_check(root->childs[1], context);
		
		root->childs[0]->variable->type = root->value->type;
		break;
		
		case NTYPE: 
		if (root->childs[0] != NULL)
		{
			syna_check(root->childs[0], context);
			root->value->type->next = root->childs[0]->value->type;
		}
		break;
		
		case NFPCALL:
		{
			syna_check(root->childs[0], context);
			pp_func f = env_get_function(root->string, 0);
			if (f != NULL)
			{
				root->value = env_create_value(f->ret_type, 0, NULL);
				if (!err_check_arguments(root->childs[0], context, f, 0, env_get_args_number(f), 1))
				{
					char* s = (char*) malloc(sizeof(char) * ERR_BUFFER_SIZE);
					sprintf(s, "Too few arguments for %s", f->name);
					err_display(s);
				}
			}else{
				root->value->type = syna_create_type(NONE, NULL);
			}
		}
		break;
	}
}

void err_display(char* s)
{
	fprintf(stderr, "***ERROR l.%d : %s***\n", line_position, s);
	exe_stop();
	total_errors++;
}

void err_report()
{
	if (!total_errors)
	printf("No error found.\n");
	else
	printf("%d error%s found.\n", total_errors, (total_errors > 1 ? "s" : ""));
}

void env_display_value(pp_value v, int root)
{
	if (root)
	printf("Value : ");
	if (v == NULL || v->type == NULL)
	printf("undefined;\n");
	else{
		switch (v->type->type) {
			case INT:
			printf("%d", v->value);
			break;
			
			case BOOL:
			printf("%s", (v->value > 0 ? "TRUE" : "FALSE"));
			break;
			
			case ARRAY:
			printf("[");
			for (int i = 0; i < v->members_count; i++)
			{
				env_display_value(v->members[i], 0);
				if (i < v->members_count - 1)
				printf(", ");
			}
			
			printf("]");
			break;
		}
		
		if (root)
		printf(";\n");
	}
}

void env_display(pp_context context)
{
	printf("===== ENV =====\n");
	pp_var v_root = context->context;
	while (v_root != NULL)
	{
		if (strcmp(v_root->name, "__a__") && strcmp(v_root->name, "__b__"))
		{
			printf("Var %s of type", v_root->name);
			pp_type t_current = v_root->type;
			while (t_current != NULL)
			{
				char s[9];
				switch (t_current->type) {
					case NONE:
					sprintf(s, "none");
					break;
					
					case INT:
					sprintf(s, "integer");
					break;
					
					case BOOL:
					sprintf(s, "boolean");
					break;
					
					case ARRAY:
					sprintf(s, "array of");
					break;
				}
				
				printf(" %s", s);
				t_current = t_current->next;
			}
			printf("\n");
			env_display_value(v_root->value, 1);
		}
		v_root = v_root->next;
	}
	
	printf("===== END =====\n");
}

void exe_stop()
{
	fprintf(stderr, "Execution stopped.\n");
	exit(-1);
}

void env_report()
{
	env_display(main_context);
}