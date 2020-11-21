#include <pthread.h>
#include "jval.h"
#include "jval.c"
#include "dllist.h"
#include "dllist.c"
#include "elevator.h"

typedef struct {
	Dllist people_waiting;
	pthread_cond_t *cond;
} Sim_Global; 

int main() {
	/*
	Dllist people_waiting = new_dllist();
	Person *p = (Person *)malloc(sizeof(Person));
	p->to = 1;
	dll_append(people_waiting, new_jval_v(p));

	Dllist dllnode = dll_first(people_waiting);
	Person *p1 = (Person *)jval_v(dll_val(dllnode));
	printf("%d\n", dll_empty(people_waiting));
	printf("%d\n", p1->to);
	dll_delete_node(dllnode);
	printf("%d\n", dll_empty(people_waiting));
	printf("%d\n", p1->to);
	*/
	Elevator_Simulation *es = (Elevator_Simulation *)malloc(sizeof(Elevator_Simulation));
	Sim_Global *vars = (Sim_Global *)malloc(sizeof(Sim_Global));
	vars->people_waiting = new_dllist();
	pthread_cond_init(vars->cond, NULL);
	es->v = (void *)(vars);

	Dllist people_waiting = ((Sim_Global *)(es->v))->people_waiting;
	Person *p = (Person *)malloc(sizeof(Person));
	p->to = 1;
	printf("%d\n", dll_empty(((Sim_Global *)(es->v))->people_waiting));
	dll_append(people_waiting, new_jval_v(p));
	printf("%d\n", dll_empty(((Sim_Global *)(es->v))->people_waiting));
}