/*
elevator_part_1
THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE
WRITTEN BY OTHER STUDENTS. Nathan Yang & Kyle Wu
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "elevator.h"

typedef struct {
	Dllist people_waiting; //doubly-linked list of people waiting
	pthread_cond_t *cond; //condition var
} Sim_Global;


//  set up the global list and a condition variable for blocking elevators
void initialize_simulation(Elevator_Simulation *es)
{
	Sim_Global *vars = (Sim_Global *)malloc(sizeof(Sim_Global));
	vars->people_waiting = new_dllist();
	vars->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(vars->cond, NULL); // initialize cond as NULL
	es->v = (void *)(vars); //define v 
}

void initialize_elevator(Elevator *e)
{
}

void initialize_person(Person *p)
{
}


//append the person to the global list. Signal the condition variable for blocking elevators. 
// Block on the person’s condition variable.
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

// Unblock the elevator’s condition variable and block on the person’s condition variable.
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

// Unblock the elevator’s condition variable.
void person_done(Person *p)
{
	//Signal elevator that person is done
	pthread_mutex_lock(p->e->lock);
	pthread_cond_signal(p->e->cond);
	pthread_mutex_unlock(p->e->lock);
}


// Each elevator is a while loop. Check the global list and if
// it’s empty, block on the condition variable for blocking elevators. When the elevator gets a
// person to service, it moves to the appropriate floor and opens its door. It puts itself into
// the person’s e field, then signals the person and blocks until the person wakes it up. When
// it wakes up, it goes to the person’s destination floor, opens its door, signals the person and
// blocks. When the person wakes it up, it closes its door and re-executes its while loop.
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
		if(e->onfloor != p->from) {
			move_to_floor(e, p->from);
		}
		open_door(e);
		p->e = e;
		pthread_cond_signal(p->cond);
		pthread_mutex_unlock(p->lock);
		
		//Block until person is on elevator and signals to move, then take them to their destination
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
	}
}