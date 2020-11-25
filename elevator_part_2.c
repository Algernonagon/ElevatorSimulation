/*elevator_part_2*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "elevator.h"

typedef struct {
	Dllist people_waiting;
	pthread_cond_t *cond;
} Sim_Global;

typedef struct {
	Dllist people_loading;
	Dllist people_leaving;
	int travel_dir;
} E_Vars;

typedef struct {
	int travel_dir;
} P_Vars;

void initialize_simulation(Elevator_Simulation *es)
{
	Sim_Global *vars = (Sim_Global *)malloc(sizeof(Sim_Global));
	vars->people_waiting = new_dllist();
	vars->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(vars->cond, NULL);
	es->v = (void *)(vars);
}

void initialize_elevator(Elevator *e)
{
	E_Vars *vars = (E_Vars *)malloc(sizeof(E_Vars));
	vars->people_loading = new_dllist();
	vars->people_leaving = new_dllist();
	vars->travel_dir = 1;
	e->v = (void *)(vars);
}

void initialize_person(Person *p)
{
	P_Vars *vars = (P_Vars *)malloc(sizeof(P_Vars));
	if(p->to - p->from > 0) vars->travel_dir = 1;
	else vars->travel_dir = -1;
	p->v = (void *)(vars);
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




// Notes from TA Office Hours

// while loop
// 1. check if anyone wants to get off on this floor
// 2. if yes, check if doors are closed
// 3. if closed, open the doors
// 4. get everyone off the elevator
// 5. check if anyone wants get on & in the direction the elevator is moving
// 6. check if the doors are closed (assuming someone wants to get on)
// 7. if closed, open doors
// 8. board everyone who wants to go
// 9. check if doors are open
// 10. if open, close the doors
// 11. move one floor in appropriate direction
// 12. if on first floor, change direction to up
// 13. if on top floor, change direction to down

// worry about direction of movement (add as local var into elevator)
// dllist accessible from elsewhere
// checks (people know whether elevator is on right floor)

void *elevator(void *arg)
{
	Elevator *e;
	e = (Elevator *)(arg);

	while(1) {
		Dllist people_waiting = ((Sim_Global *)(e->es->v))->people_waiting;
		pthread_cond_t *cond = ((Sim_Global *)(e->es->v))->cond;
		Dllist people_loading = ((E_Vars *)(e->v))->people_loading;
		Dllist people_leaving = ((E_Vars *)(e->v))->people_leaving;
		int e_travel_dir = ((E_Vars *)(e->v))->travel_dir;
		if(e->onfloor + e_travel_dir > e->es->nfloors || e->onfloor + e_travel_dir < 1) {
			pthread_mutex_lock(e->lock);
			e_travel_dir *= -1;
			((E_Vars *)(e->v))->travel_dir = e_travel_dir;
			pthread_mutex_unlock(e->lock);
		}

		//Block until there are people waiting and given signal, then add all people on same floor and going in same direction as elevator to people_loading
		//and remove them from people_waiting
		pthread_mutex_lock(e->es->lock);
		while(dll_empty(people_waiting)) {
			pthread_cond_wait(cond, e->es->lock);
		}
		Dllist dllnode;
		Person *p;
		int p_travel_dir; 
		dll_traverse(dllnode, people_waiting) {
			p = (Person *)jval_v(dll_val(dllnode));
			p_travel_dir = ((P_Vars *)(p->v))->travel_dir;
			if(e->onfloor == p->from && e_travel_dir == p_travel_dir) {
				pthread_mutex_lock(e->lock);
				dll_append(people_loading, new_jval_v(p));
				pthread_mutex_unlock(e->lock);
				dllnode = dllnode->blink;
				dll_delete_node(dllnode->flink);
			}
		}
		pthread_mutex_unlock(e->es->lock);

		//Check if there are any people who get off on this floor and append them to people_leaving
		pthread_mutex_lock(e->lock);
		dll_traverse(dllnode, e->people) {
			p = (Person *)jval_v(dll_val(dllnode));
			if(e->onfloor == p->to) {
				//pthread_mutex_lock(e->lock);
				dll_append(people_leaving, new_jval_v(p));
				//pthread_mutex_unlock(e->lock);
			}
		}
		pthread_mutex_unlock(e->lock);

		//If there are people who need to get on or leave and the door is closed, open the door
		if((!dll_empty(people_loading) || !dll_empty(people_leaving)) && e->door_open == 0) {
			open_door(e);
		}

		//Signal all people who get off on this floor
		dll_traverse(dllnode, people_leaving) {
			p = (Person *)jval_v(dll_val(dllnode));
			pthread_mutex_lock(p->lock);
			pthread_cond_signal(p->cond);
			pthread_mutex_unlock(p->lock);
		}

		//Wait for all people getting off to be done and remove them from people_leaving
		dll_traverse(dllnode, people_leaving) {
			p = (Person *)jval_v(dll_val(dllnode));
			pthread_mutex_lock(e->lock);
			while(p->ptr != NULL) {
				pthread_cond_wait(e->cond, e->lock);
			}
			pthread_mutex_unlock(e->lock);
			dllnode = dllnode->blink;
			dll_delete_node(dllnode->flink);
		}

		//Put self as elevator for all people getting on and signal them to get on elevator
		dll_traverse(dllnode, people_loading) {
			p = (Person *)jval_v(dll_val(dllnode));
			pthread_mutex_lock(p->lock);
			p->e = e;
			pthread_cond_signal(p->cond);
			pthread_mutex_unlock(p->lock);
		}
		
		//Wait for all people getting on elevator and remove them from people_loading
		dll_traverse(dllnode, people_loading) {
			p = (Person *)jval_v(dll_val(dllnode));
			pthread_mutex_lock(e->lock);
			while(p->ptr == NULL) {
				pthread_cond_wait(e->cond, e->lock);
			}
			pthread_mutex_unlock(e->lock);
			dllnode = dllnode->blink;
			dll_delete_node(dllnode->flink);
		}

		//Close door if it is open
		if(e->door_open == 1) {
			close_door(e);
		}
		//Move one floor
		move_to_floor(e, e->onfloor + e_travel_dir);

		
	}
}