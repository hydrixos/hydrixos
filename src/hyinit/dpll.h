/*
 *
 * dpll.h
 *
 * (C)2007 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Global header file of init
 *
 */
#ifndef _DPLL_H
#define _DPLL_H

#include <hydrixos/list.h>

typedef unsigned	dpll_var_t;
typedef signed		dpll_lit_t;


#define DPLL_ASSIGN_TRUE		1
#define DPLL_ASSIGN_FALSE		0
#define DPLL_ASSIGN_UNDEF		-1

#define dpll_var(__lit)			((dpll_var_t)((__lit < 0) ? ((__lit) * -1) : ((__lit))))
#define dpll_lit_value(__lit)		((dpll_lit_t)((__lit < 0) ? (DPLL_ASSIGN_FALSE) : (DPLL_ASSIGN_TRUE)))

#define DPLL_CLSTATUS_UNDEFINED		0
#define DPLL_CLSTATUS_SATISFIED		1
#define DPLL_CLSTATUS_TAUTOLOGY		2

typedef struct dpll_clause_st {
	unsigned	num_literals;
	unsigned	free_literals;
	unsigned	true_literals;
	uint8_t		status;

	dpll_lit_t	*literals;

	list_t		ls;
	struct dpll_clause_st		**watchers;
}dpll_clause_t;

typedef struct {
	int8_t		*interpretation;
	dpll_lit_t	*assignment_history;

	dpll_clause_t		*units;
	struct dpll_clause_st	**watchers;

	unsigned	num_assignments;
	unsigned	num_variables;

	unsigned	num_clauses;
	unsigned	unsatisfied_clauses;
}dpll_solver_t;

#define DPLL_SAT	1
#define DPLL_UNSAT	0
#define DPLL_UNDEF	-1
#define DPLL_ERROR	-2

/* Solving functions */
int dpll_assign_variable(dpll_solver_t *solver, dpll_lit_t literal);
void dpll_undo_assignment(dpll_solver_t *solver, dpll_lit_t literal);

int dpll_propagate(dpll_solver_t *solver);

dpll_var_t dpll_select_variable(dpll_solver_t *solver);
dpll_var_t dpll_backtrack(dpll_solver_t *solver);

int dpll_solve(dpll_solver_t *solver);

dpll_lit_t* dpll_get_model(dpll_solver_t *solver);

/* Initialization functions */
dpll_solver_t* dpll_new_solver(void);

dpll_var_t dpll_new_variable(dpll_solver_t *solver);
dpll_clause_t* dpll_new_clause(dpll_solver_t *solver, dpll_lit_t *literals, int num_lits);


#endif
