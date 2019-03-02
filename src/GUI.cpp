#include "../include/GUI.hpp"

GUI::GUI(int rows, int cols, float scale)
{
	this->rows = rows * scale;
	this->cols = cols * scale;
	this->scale = scale;
	display=Mat(cv::Size(rows, cols), CV_8UC3, Scalar(220,220,220));
}

void GUI::draw_obstacles(vector< vector<Point> > polygon)
{
	for(int i=0; i < polygon.size(); i++)
		for(int j=0; j < polygon[i].size(); j++)
			polygon[i][j].x *= scale, polygon[i][j].y *= scale;
	
	cv::drawContours(display,polygon,-1,Scalar(120,120,120),-1);
	return;
}

void GUI::draw_car(State state, Vehicle car)
{
	RotatedRect rotRect = RotatedRect(Point2f(state.y*scale, state.x*scale), Size2f(car.BOT_W*scale, car.BOT_L*scale), 180-state.theta*180/M_PI);
	Point2f vert[4];
	rotRect.points(vert);
	for(int i=0;i<4;i++)
		line(display, vert[i], vert[(i+1)%4], Scalar(200, 0, 0));
	
	return;
}

void GUI::draw_dubins( vector<State> Path )
{
	for(vector<State>::iterator next= Path.begin(); next!=Path.end();next++)
		display.at<Vec3b>(next->x*scale,next->y*scale) = { 255, 0, 0 };		

	return ;
}

void GUI::draw_tree(State state, State next)
{
	Point root,child;
	root.x=state.y*scale;
	root.y=state.x*scale;
	child.x=next.y*scale; 
	child.y=next.x*scale;
	line(display, root, child, Scalar(0,0,255), 1);
	
	return;
}

void GUI::show(int t=0){
	imshow("Display", display);
	waitKey(t);
	return;
}


