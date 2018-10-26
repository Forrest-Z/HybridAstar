#ifndef MAP_CPP
#define MAP_CPP

#include "../include/Map.hpp"

Map::Map( bool **obs_map, State end,vector<vector<Point>> obs ){

	this->obs=obs;
	this->obs_map = obs_map;
	this->end = end;
	this->map_resolution = 10;
	initCollisionChecker();
}

bool Map::isReached(State current){
 	
 	/* In this we could have int in place of bool which tells distance between them so 
 	thst we could act accordingly && fabs(Curr.theta-End.theta)<5*/
 	//
	if( abs(current.x - end.x) < 1 && abs(current.y - end.y) < 1 && (abs(current.theta - end.theta) < 3.14/9 || abs(current.theta - 72 + end.theta) < 3.14/9))
		return true;
 	else return false;
}

void Map::initCollisionChecker(){
	acc_obs_map=new int*[MAPX];
	for(int i=0;i<MAPX;i++)
	{
		acc_obs_map[i]=new int[MAPY];
		for(int j=0;j<MAPY;j++)
			acc_obs_map[i][j]=obs_map[i][j];
	}

	for(int i=0;i<MAPX;i++)
		for(int j=1;j<MAPY;j++)
			acc_obs_map[i][j]=acc_obs_map[i][j-1]+acc_obs_map[i][j];

	for(int j=0;j<MAPY;j++)
		for(int i=1;i<MAPX;i++)
			acc_obs_map[i][j]=acc_obs_map[i-1][j]+acc_obs_map[i][j];

	return;
}

bool Map::checkCollision(State pos){

	vector <Point> vertices;
	
	if(pos.x*map_resolution>=MAPX || pos.x*map_resolution<0 || pos.y*map_resolution>=MAPY || pos.y*map_resolution<0 )
		return true;

	//first use a bounding box around car to check for collision in O(1) time
	int max_x, min_x, max_y, min_y;
	max_x = map_resolution * (pos.x+car.BOT_L*abs(cos(pos.theta))/2+car.BOT_W*abs(sin(pos.theta))/2) + 1;
	min_x = map_resolution * (pos.x-car.BOT_L*abs(cos(pos.theta))/2-car.BOT_W*abs(sin(pos.theta))/2) - 1;

	max_y= map_resolution * (pos.y+car.BOT_L*abs(sin(pos.theta))/2+car.BOT_W*abs(cos(pos.theta))/2) + 1;
	min_y= map_resolution * (pos.y-car.BOT_L*abs(sin(pos.theta))/2-car.BOT_W*abs(cos(pos.theta))/2) - 1;
	
	if(max_x>=MAPX || min_x<0 || max_y>=MAPY || min_y<0)
		return true;

	if(acc_obs_map[max_x][max_y]+acc_obs_map[min_x][min_y]==acc_obs_map[max_x][min_y]+acc_obs_map[min_x][max_y]){
		return false;
	}

	// brute force check through the car
	for(float i=-car.BOT_L/2.0;i<=car.BOT_L/2.0+0.001;i+=0.25)
		for(float j=-car.BOT_W/2.0;j<=car.BOT_W/2.0+0.001;j+=0.25)
		{
			int s = map_resolution * (pos.x+i*cos(pos.theta)+j*sin(pos.theta)) + 0.001;
			int t = map_resolution * (pos.y+i*sin(pos.theta)+j*cos(pos.theta)) + 0.001;

     		if(obs_map[s][t])
				return true;
		}
	return false;

}

// v2 is obstacle and v1 is bot
bool Map::helperSAT(vector <Point> v1,vector <Point> v2)
{

	double slope,theta,alpha;
	double dis;
	double rmin1,rmax1,rmin2,rmax2;

	for (int i=0;i<v1.size()-2;i++)
	{
		rmin1=rmin2=INT_MAX;
		rmax1=rmax2=INT_MIN;

		if( (v1[i+1].x==v1[i].x)) 
		{
			alpha=0;
			slope=0;
		}
		else if(v1[i+1].y-v1[i].y==0)
		{
			alpha=CV_PI/2;
			slope=INT_MAX;
		}
		else 
		{
			slope=(double)(v1[i+1].y-v1[i].y)/(v1[i+1].x-v1[i].x);
			slope=-1*(1/slope);
			if (slope<0)
				alpha=CV_PI+atan(slope);
			else
				alpha=atan(slope);
		}
		// cout<<v1[i].x<<" "<<v1[i].y<<" "<<v1[i+1].x<<" "<<v1[i+1].y<<endl;
		// cout<<"slope of the axis for this run : "<<slope<<" and alpha : "<<alpha<<endl;
		int count=0;
		for (int j=0;j<v1.size();j++)
		{
			// // cout<<atan((v1[j].y)/(v1[j].x))<<endl;
			if(v1[j].x==0)
				theta=alpha-CV_PI/2;
			else
				theta=alpha-atan((double)v1[j].y/(double)v1[j].x);
			if( theta>CV_PI/2 )
				theta=atan((double)v1[j].y/(double)v1[j].x)-alpha+CV_PI;

			dis=sqrt(v1[j].y*v1[j].y+v1[j].x*v1[j].x);
			// cout<<"Theta :"<<theta<<endl;
			// cout<<"atan(((double)v1[j].y)/(double)(v1[j].x)) :"<<atan(((double)v1[j].y)/(double)(v1[j].x))<<endl;			
			// cout<<"X: "<<v1[j].x<<" Y: "<<v1[j].y<<endl;
			// cout<<"dis*cos(theta): "<<dis*cos(theta)<<endl;
			rmin1=min(rmin1,dis*cos(theta));
			// cout<<rmin1<<endl;
			rmax1=max(rmax1,dis*cos(theta));
			// cout<<rmax1<<endl;
		}
		// cout<<"Ended"<<endl;
		for (int j=0;j<v2.size();j++)
		{
			if(v2[j].x==0)
				theta=alpha-CV_PI/2;
			else
				theta=alpha-atan((double)v2[j].y/(double)v2[j].x);
			if( theta>CV_PI/2 )
				theta=atan((double)v2[j].y/(double)v2[j].x)-alpha+CV_PI;

			dis=sqrt(v2[j].y*v2[j].y+v2[j].x*v2[j].x);
		//	// cout<<"D"<<endl;
			rmin2=min(rmin2,dis*cos(theta));
			// cout<<rmin2<<endl;
			rmax2=max(rmax2,dis*cos(theta));
			// cout<<rmax2<<endl;
		}

		if ( rmin1>rmax2 || rmin2>rmax1 ) 
		{
			// cout<<"Returned"<<endl;
			return false;
		}
		// we assume the line passes through origin and the slope is -1/slope
	}
	for (int i=0;i<v2.size()-2;i++)
	{
		rmin1=rmin2=INT_MAX;
		rmax1=rmax2=INT_MIN;

		if( (v2[i+1].x==v2[i].x)) 
		{
			alpha=0;
			slope=0;
		}
		else if(v2[i+1].y-v2[i].y==0)
		{
			alpha=CV_PI/2;
			slope=INT_MAX;
		}
		else 
		{
			slope=(double)(v2[i+1].y-v2[i].y)/(v2[i+1].x-v2[i].x);
			slope=-1*(1/slope);
			if (slope<0)
				alpha=CV_PI+atan(slope);
			else
				alpha=atan(slope);
		}
		// cout<<v2[i].x<<" "<<v2[i].y<<" "<<v2[i+1].x<<" "<<v2[i+1].y<<endl;
		// cout<<"slope of the axis for this run : "<<slope<<" and alpha : "<<alpha<<endl;
		int count=0;
		for (int j=0;j<v1.size();j++)
		{
			// // cout<<atan((v1[j].y)/(v1[j].x))<<endl;
			if(v1[j].x==0)
				theta=alpha-CV_PI/2;
			else
				theta=alpha-atan((double)v1[j].y/(double)v1[j].x);
			if( theta>CV_PI/2 )
				theta=atan((double)v1[j].y/(double)v1[j].x)-alpha+CV_PI;

			dis=sqrt(v1[j].y*v1[j].y+v1[j].x*v1[j].x);
			// cout<<"Theta :"<<theta<<endl;
			// cout<<"atan(((double)v1[j].y)/(double)(v1[j].x)) :"<<atan(((double)v1[j].y)/(double)(v1[j].x))<<endl;			
			// cout<<"X: "<<v1[j].x<<" Y: "<<v1[j].y<<endl;
			// cout<<"dis*cos(theta): "<<dis*cos(theta)<<endl;
			rmin1=min(rmin1,dis*cos(theta));
			// cout<<rmin1<<endl;
			rmax1=max(rmax1,dis*cos(theta));
			// cout<<rmax1<<endl;
		}
		// cout<<"Ended"<<endl;
		for (int j=0;j<v2.size();j++)
		{
			if(v2[j].x==0)
				theta=alpha-CV_PI/2;
			else
				theta=alpha-atan((double)v2[j].y/(double)v2[j].x);
			if( theta>CV_PI/2 )
				theta=atan((double)v2[j].y/(double)v2[j].x)-alpha+CV_PI;

			dis=sqrt(v2[j].y*v2[j].y+v2[j].x*v2[j].x);
		//	// cout<<"D"<<endl;
			rmin2=min(rmin2,dis*cos(theta));
			// cout<<rmin2<<endl;
			rmax2=max(rmax2,dis*cos(theta));
			// cout<<rmax2<<endl;
		}

		if ( rmin1>rmax2 || rmin2>rmax1 ) 
		{
			// cout<<"Returned"<<endl;
			return false;
		}
		// we assume the line passes through origin and the slope is -1/slope
	}
	return true;

	}
bool Map::checkCollisionSat(State pos)
{
	bool collide=false;
	vector<Point> v1;

	// We are trying to find the four corner points of our bot 
	Point p1;
	p1.x = map_resolution * (pos.x-car.BOT_L*abs(cos(pos.theta))/2-car.BOT_W*abs(sin(pos.theta))/2) ;
	p1.y = map_resolution * (pos.y-car.BOT_L*abs(sin(pos.theta))/2+car.BOT_W*abs(cos(pos.theta))/2) ;
	v1.push_back(p1);

	Point p2;
	p2.x = map_resolution * (pos.x+car.BOT_L*abs(cos(pos.theta))/2-car.BOT_W*abs(sin(pos.theta))/2) ;
	p2.y = map_resolution * (pos.y+car.BOT_L*abs(sin(pos.theta))/2+car.BOT_W*abs(cos(pos.theta))/2) ;
	v1.push_back(p2);

	Point p3;
	p3.x = map_resolution * (pos.x+car.BOT_L*abs(cos(pos.theta))/2+car.BOT_W*abs(sin(pos.theta))/2) ;
	p3.y = map_resolution * (pos.y+car.BOT_L*abs(sin(pos.theta))/2-car.BOT_W*abs(cos(pos.theta))/2) ;
	v1.push_back(p3);

	Point p4;
	p4.x = map_resolution * (pos.x-car.BOT_L*abs(cos(pos.theta))/2+car.BOT_W*abs(sin(pos.theta))/2) ;
	p4.y = map_resolution * (pos.y-car.BOT_L*abs(sin(pos.theta))/2-car.BOT_W*abs(cos(pos.theta))/2) ;
	v1.push_back(p4);

	// // cout<<" Current state "<<pos.x<<" "<<pos.y<<" "<<pos.theta<<endl;
	// for (int i=0;i<v1.size();i++)
	// {
	// 	// cout<<"bots "<<v1[i].x<<" "<<v1[i].y<<" ";
	// }
	// // cout<<endl;
	for (int i = 0; i < obs.size() ; ++i)
	{
		if( helperSAT( v1 , obs[i] ) )
			{
				// cout<<"True"<<endl;
				return true;
			}
			// else // cout<<"False"<<endl;
	}

	return false;
}
#endif
