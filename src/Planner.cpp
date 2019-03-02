#include "../include/Planner.hpp"
#include <pthread.h>
#include <limits.h>
#include "opencv/cv.h"
#include "../include/GUI.hpp"
#include <opencv2/highgui/highgui.hpp>

typedef void * (*THREADFUNCPTR)(void *);

vector< vector< double > > H;
double t=0;
State target;
Vehicle veh;

void threadedDubins( Dubins *a)
{
	Heuristic h;
	a->dubins_cost = h.Dubin_cost(a->initial,a->final,a->radius);
}

bool Planner::operator()(State a,State b)
{
	Dubins *ad,*bd;
	
	ad = new Dubins;
	bd = new Dubins;
	
	ad->initial = a, ad->final= target, ad->radius =  veh.min_radius;
	bd->initial = b, bd->final= target, bd->radius =  veh.min_radius;

	pthread_t thread1, thread2;
    pthread_create( &thread1, NULL, (THREADFUNCPTR)&threadedDubins,(void*)ad);
    pthread_create( &thread2, NULL, (THREADFUNCPTR)&threadedDubins,(void*)bd);

    pthread_join( thread1, NULL);
    pthread_join( thread2, NULL); 
	
    // cout<<"a->dubins_cost "<<ad->dubins_cost<<" b->dubins_cost "<<bd->dubins_cost<<endl;

	// Calculating max of Dubin's and Djikstra's 
	clock_t start_time=clock();
	double temp_a=max(H[(int)a.x][(int)a.y],ad->dubins_cost);
	double temp_b=max(H[(int)b.x][(int)b.y],bd->dubins_cost);
	clock_t end_time=clock();

	t+=(double)(end_time-start_time)/CLOCKS_PER_SEC;
	if(a.cost2d+temp_a >= b.cost2d+temp_b)
	{
		if(a.cost2d+temp_a > b.cost2d+temp_b)
			return true;
		else return (a.cost2d < b.cost2d);
	}
	else return false;

}

vector<State> Planner::plan(State start, State end, Vehicle car, vector<vector<Point> > obs, GUI display, int rows, int cols)
{

	bool DEBUG = false;
	Map map(obs, end, rows, cols);                          //Object of Map class

	veh = car;
	target = end;                         
	
	if(DEBUG)
	{
	    display.draw_obstacles(obs);
	    display.draw_car(start, car);
	    display.draw_car(end, car);
	    display.show();
	}   


	// Djikstra Calculation
	clock_t time_begin= clock();
	h_obj.Dijkstra(map,end);
	clock_t time_end= clock();
	cout<<"Time: Dijkstra= "<<double(time_end-time_begin)/CLOCKS_PER_SEC<<endl;

	
	// Djikstra Copying
	time_begin= clock();
	H.resize(map.VISX, vector<double>(map.VISY));
	for(int i=0;i<map.VISX;i++)
	    for(int j=0;j<map.VISY;j++)
			H[i][j]=h_obj.h_vals[i][j].dis;
	time_end= clock();
	cout<<"Time: Dijkstra Copying= "<<double(time_end-time_begin)/CLOCKS_PER_SEC<<endl;
			

	// Memory freeing
	for(int i=0;i<map.VISX;i++)
		delete[] h_obj.h_vals[i];
	delete[] h_obj.h_vals;	


	// Array of states allocation
	time_begin= clock();
	vector< vector< vector< State > > > visited_state(map.VISX,vector< vector< State > >(map.VISY,vector< State >(map.MAP_THETA)));	
	time_end= clock();
	cout<<"Time: Array of States Allocation = "<<double(time_end-time_begin)/CLOCKS_PER_SEC<<endl;


	// To mark the visited states VISX, VISY and MAP_THETA are to be imported from the Map class
	time_begin= clock();
	vector< vector< vector< bool > > > visited(map.VISX,vector< vector< bool > >(map.VISY,vector< bool >(map.MAP_THETA,false)));
	time_end= clock();
	cout<<"Time: Visited Array of States Allocation = "<<double(time_end-time_begin)/CLOCKS_PER_SEC<<endl;


	priority_queue <State, vector<State>, Planner> pq;
	pq.push(start);

	// cout<<"Here"<<endl;

	double checkCollisionTime=0;
	double nextStatesTime=0;
	
	int count=0;
	while(!pq.empty())
	{
		if(DEBUG)
			cout<<"Inside While"<<endl;
		
		State current=pq.top();
		pq.pop();

		int grid_theta=((int)(current.theta*map.MAP_THETA/(2*M_PI)))%map.MAP_THETA; 
		if( visited[(int)current.x][(int)current.y][grid_theta] )
			continue;
		visited[(int)current.x][(int)current.y][grid_theta] = true;
		visited_state[(int)current.x][(int)current.y][grid_theta] = current;

		if(DEBUG)
			cout<<"Current State "<<current.x<<" "<<current.y<<" "<<current.theta<<endl;	

		// Checks if it has reached the goal
		if(map.isReached(current))
		{
			cout<<"Time :CollisionChecker= "<<checkCollisionTime<<endl;
			cout<<"Time :nextStates= "<<nextStatesTime<<endl;
			cout<<"Time :Dubins on Spot = "<<t<<endl;
			cout<<"REACHED!"<<endl;
			
			State temp=current;

			while( temp.parent != NULL )
			{
				path.push_back(temp);
				temp=*(temp.parent);
			}
			reverse(path.begin(), path.end());			
			return path;
		}

		if( count%4 != 3 )
		{

			time_begin=clock();
			vector<State> next=car.nextStates(&current);
			time_end=clock();
			nextStatesTime+=double(time_end-time_begin)/CLOCKS_PER_SEC;

			for(vector<State>::iterator it= next.begin(); it!=next.end();it++)
			{
				State nextS = *it;
				int next_theta=((int)(nextS.theta*180/(M_PI*5)))%72;
					
				if( visited[(int)nextS.x][(int)nextS.y][next_theta] )
					continue;
				
				time_begin=clock();
				if( !map.checkCollisionSat(nextS) )
				{
					time_end=clock();
					it->parent = &(visited_state[(int)current.x][(int)current.y][grid_theta]);
					it->cost2d = current.cost2d+1;
					
					if(DEBUG)
				    {
						display.draw_tree(current,nextS);
			        	display.show(5);
				    }
					
					pq.push(*it);
				}
				else
					time_end=clock();

				checkCollisionTime+=double(time_end-time_begin)/CLOCKS_PER_SEC;
			}

		}
		else
		{
			
			time_begin=clock();
			vector<State> Path = h_obj.DubinShot(current,end,car.min_radius);
			time_end=clock();
			nextStatesTime+=double(time_end-time_begin)/CLOCKS_PER_SEC;

			State prev=current,check=current;
			// cout<<map.isReached()
			for(vector<State>::iterator it= Path.begin(); it!=Path.end();it++)
			{
				State nextS = *it;
				if(map.isReached(nextS))
				{
					cout<<"Time :CollisionChecker= "<<checkCollisionTime<<endl;
					cout<<"Time :nextStates= "<<nextStatesTime<<endl;
					cout<<"Time :Dubins on Spot = "<<t<<endl;
					cout<<"REACHED!"<<endl;

					State temp=check;
					while( temp.parent != NULL )
					{
						path.push_back(temp);
						temp=*(temp.parent);
					}
					reverse(path.begin(), path.end());
					return path;
				
				}

				if( sqrt(pow(nextS.x-prev.x,2) + pow(nextS.y-prev.y,2)) < 2 )
					continue;
				
				time_begin=clock();
				if( !map.checkCollisionSat(nextS) )
				{

					if(DEBUG)
						cout<<"Not collided"<<endl;
					
					time_end=clock();

					int prev_theta=((int)(prev.theta*map.MAP_THETA/(M_PI*2)))%map.MAP_THETA;
					
					it->parent = &(visited_state[(int)prev.x][(int)prev.y][(int)prev_theta]);
					it->cost2d = prev.cost2d+1;
					
					it->gx=it->x,it->gy=it->y;

					int next_theta=((int)(nextS.theta*map.MAP_THETA/(M_PI*2)))%map.MAP_THETA;
					visited_state[(int)nextS.x][(int)nextS.y][(int)next_theta] = *it;

					if(DEBUG)
					{
						cout<<"Check nextS : "<<nextS.x<<" "<<nextS.y<<" "<<nextS.theta<<endl; 
						cout<<"Check prev : "<<prev.x<<" "<<prev.y<<" "<<prev.theta<<endl; 
						cout<<"Check parent : "<<(it->parent)->x<<" "<<(it->parent)->y<<" "<<(it->parent)->theta<<endl; 
					}

					check=*it;
					prev=nextS;
					checkCollisionTime+=double(time_end-time_begin)/CLOCKS_PER_SEC;
					pq.push(*it);
				}
				else
				{
				    if(DEBUG)
					{
						cout<<"Collided "<<endl;
						Path.erase(it,Path.end());
						display.draw_dubins( Path);
					    display.show(5);
					}

					time_end=clock();
					checkCollisionTime+=double(time_end-time_begin)/CLOCKS_PER_SEC;
					break;
				}

			}
		}
		if(DEBUG)
        	display.show(1);
		
		count++;
	}
	cout<<"Goal cannot be reached"<<endl;
	exit(0);
}



