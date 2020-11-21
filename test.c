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

void initialize_simulation(Elevator_Simulation *es)
{
	//pthread_mutex_init(es->lock);
	Sim_Global *vars = (Sim_Global *)malloc(sizeof(Sim_Global));
	vars->people_waiting = new_dllist();
	vars->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(vars->cond, NULL);
	es->v = (void *)(vars);
}

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
	//Elevator_Simulation *es = (Elevator_Simulation *)malloc(sizeof(Elevator_Simulation));
	Elevator_Simulation ES, *es;
	es = &ES;
	es->nfloors = 5;
	es->nelevators = 2;
	es->interarrival_time = 1;
	es->door_time = 1;
	es->floor_to_floor_time = 5;
	es->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  	pthread_mutex_init(es->lock, NULL);
	es->npeople_started = 0;
	es->npeople_finished = 0;
	initialize_simulation(es);

	Dllist people_waiting1 = ((Sim_Global *)(es->v))->people_waiting;
	Dllist people_waiting2 = ((Sim_Global *)(es->v))->people_waiting;
	Person *p = (Person *)malloc(sizeof(Person));
	p->to = 2;
	printf("%d\n", dll_empty(people_waiting2));
	dll_append(people_waiting1, new_jval_v(p));
	printf("%d\n", dll_empty(people_waiting2));
	printf("\n");

	Dllist dllnode = dll_first(people_waiting2);
	Person *p1 = (Person *)jval_v(dll_val(dllnode));
	printf("%d\n", p1->to);
	printf("%d\n", dll_empty(people_waiting1));
	dll_delete_node(dllnode);
	printf("%d\n", p1->to);
	printf("%d\n", dll_empty(people_waiting1));
	//printf("%d\n", dll_empty(((Sim_Global *)(es->v))->people_waiting));
	//printf("%d\n", dll_empty(((Sim_Global *)(es->v))->people_waiting));
}