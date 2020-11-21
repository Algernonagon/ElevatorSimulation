/*elevator_part_2*/

#include <stdio.h>
#include <pthread.h>
#include "elevator.h"

typedef struct {
	Dllist people_waiting;
	pthread_cond_t *cond;
} Sim_Global;

void initialize_simulation(Elevator_Simulation *es)
{
	Sim_Global *vars = (Sim_Global *)malloc(sizeof(Sim_Global));
	vars->people_waiting = new_dllist();
	pthread_cond_init(vars->cond, NULL);
	es->v = (void *)(vars);
}

void initialize_elevator(Elevator *e)
{
}

void initialize_person(Person *p)
{
}

void wait_for_elevator(Person *p)
{	
	Dllist people_waiting = ((Sim_Global *)(p->es->v))->people_waiting;
	pthread_cond_t *cond = ((Sim_Global *)(p->es->v))->cond;
	
	//Append person to wait list and signal elevators
	pthread_mutex_lock(p->es->lock);
	dll_append(people_waiting, new_jval_v(p));
	pthread_cond_signal(cond);
	pthread_mutex_unlock(p->es->lock);

	//Block until assigned an elevator and given signal to get on
	pthread_mutex_lock(p->lock);
	while(p->e == NULL) {
		pthread_cond_wait(p->cond, p->lock);
	}
	pthread_mutex_unlock(p->lock);
}

void wait_to_get_off_elevator(Person *p)
{
	//Signal elevator to move to person's 'to' destination
	pthread_mutex_lock(p->e->lock);
	pthread_cond_signal(p->e->cond);
	pthread_mutex_unlock(p->e->lock);

	//Block until elevator at destination
	pthread_mutex_lock(p->lock);
	while(p->e->onfloor != p->to) {
		pthread_cond_wait(p->cond, p->lock);
	}
	pthread_mutex_unlock(p->lock);
}

void person_done(Person *p)
{
	//Signal elevator that person is done
	pthread_mutex_lock(p->e->lock);
	pthread_cond_signal(p->e->cond);
	pthread_mutex_unlock(p->e->lock);
}

void *elevator(void *arg)
{
	Elevator *e;
	e = (Elevator *)(arg);

	while(1) {
		Dllist people_waiting = ((Sim_Global *)(e->es->v))->people_waiting;
		pthread_cond_t *cond = ((Sim_Global *)(e->es->v))->cond;

		//Block until there are people waiting and given signal, then take a person off the wait list
		pthread_mutex_lock(e->es->lock);
		while(dll_empty(people_waiting)) {
			pthread_cond_wait(cond, e->es->lock);
		}
		Dllist dllnode = dll_first(people_waiting);
		Person *p = (Person *)jval_v(dll_val(dllnode));
		dll_delete_node(dllnode);
		pthread_mutex_unlock(e->es->lock);

		//Move to person's floor, put self as person's elevator, then signal the person to get on
		pthread_mutex_lock(p->lock);
		move_to_floor(e, p->from);
		open_door(e);
		p->e = e;
		pthread_cond_signal(p->cond);
		pthread_mutex_unlock(p->lock);
		
		//Block until person is on elevator and signaled to move, then take them to their destination
		pthread_mutex_lock(e->lock);
		while(dll_empty(e->people)) {
			pthread_cond_wait(e->cond, e->lock);
		}
		pthread_mutex_unlock(e->lock);
		close_door(e);
		move_to_floor(e, p->to);
		open_door(e);

		//Signal person to get off
		pthread_mutex_lock(p->lock);
		pthread_cond_signal(p->cond);
		pthread_mutex_unlock(p->lock);

		//Block until person gets off and signals they are done.
		pthread_mutex_lock(e->lock);
		while(!dll_empty(e->people)) {
			pthread_cond_wait(e->cond, e->lock);
		}
		pthread_mutex_unlock(e->lock);
		close_door(e);
		//pthread_mutex_unlock(p->lock);
	}
}