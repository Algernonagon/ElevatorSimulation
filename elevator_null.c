/* elevator_null.c
   Null solution for the elevator threads lab.
   Jim Plank
   CS560
   Lab 2
   January, 2009
 */

#include <stdio.h>
#include <pthread.h>
#include "elevator.h"

typedef struct {
	Dllist people_waiting;
	pthread_cond_t *cond;
} Sim_Global;

void initialize_simulation(Elevator_Simulation *es)
{
	es->nfloors = nfloors;
	es->nelevators = nelevators;
	es->interarrival_time = interarrival_time;
	es->door_time = door_time;
	es->floor_to_floor_time = floor_to_floor_time;
	pthread_mutex_init(es->lock);

	Sim_Global *vars = (Sim_Global *)malloc(sizeof(Sim_Global));
	vars->people_waiting = new_dllist();
	pthread_cond_init(vars->cond, NULL);
	es->v = (void *)(vars);
}

void initialize_elevator(Elevator *e)
{
	e->onfloor = 1;
	e->door_open = 0;
	e->moving = 0;
	e->people = new_dllist();
	pthread_mutex_init(e->lock);
	pthread_cond_init(e->cond, NULL);
}

void initialize_person(Person *p)
{
	pthread_mutex_init(p->lock);
	pthread_cond_init(p->cond, NULL);
}

void wait_for_elevator(Person *p)
{
}

void wait_to_get_off_elevator(Person *p)
{
}

void person_done(Person *p)
{
}

void *elevator(void *arg)
{
	Elevator *e;
	e = (Elevator *)(arg);

	while(1) {
		pthread_mutex_lock(e->es->lock);
		Dllist people_waiting = ((Sim_Global *)(e->es->v))->people_waiting;
		pthread_cond_t *cond = ((Sim_Global *)(e->es->v))->cond;
		while(dll_empty(people_waiting)) {
			pthread_cond_wait(cond, e->es->lock);
		}
		Dllist dllnode = dll_first(people_waiting);
		Person *p = (Person *)jval_v(dll_val(dllnode));
		dll_delete_node(dllnode);
		pthread_mutex_unlock(e->es->lock);

		pthread_mutex_lock(p->lock);
		move_to_floor(e, p->from);
		open_door(e);
		p->e = e;
		pthread_cond_signal(p->cond);
		//pthread_mutex_unlock(p->lock);

		pthread_mutex_lock(e->lock);
		while(dll_empty(e->people)) {
			pthread_cond_wait(e->cond, e->lock);
		}
		pthread_mutex_unlock(e->lock);
		

		pthread_mutex_unlock(p->lock);
	}
}
