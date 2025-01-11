#include <iostream>
#include <stdio.h>
#include <conio.h>
/*******************************************************************************************************/
/*					This Program has been written by Hossein Yarahmadi					               */
/*                  The Path Matrix includes three types of entities                                   */
/*                  0: means the obstacle  1:means the feasible moving  2:means the charge station     */
/* 																									   */
/*******************************************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <time.h>
#include <iomanip>

#define Row 9
#define Col 9
#define Action 8

#define EpisodeNumber 100
#define Alpha 0.5
#define Gama 0.9
#define EPSILON 0.1

#define AgentNumber 1
/**************************************************************************/
using namespace std;
/**************************************************************************/
void CreateMaze(int Maze[][Col])
{
	int i,j;
	int Item,Charge;
	
	cout<<"\n\t\t===The Maze: 0 means that it is not possible to cross===";
	cout<<"\n\t\t===The Maze: 1 means that it is possible to cross===";
	cout<<"\n\t\t===The Maze: 2 means the goal===";
	cout<<"\n================================================================================\n";
	for (i=0;i<Row;i++)
	{
		for(j=0;j<Col;j++)
		{
		  Item=rand()%(2);
		  Maze[i][j]=Item;
		  Charge=rand()%(10);
		  if (Charge==5) Maze[i][j]=2;
		}
	}
	
}
/**************************************************************************/
void PrintMatrixInt(int Matrix[][Col],char Text[])
{
	int i,j;
	cout<<"\n\t\t******* This is The: "<<Text<<"  *******\n\n";
	cout<<"\n\n";
	cout<<"\t\t\t\t    ";
	for(j=0;j<10;j++) cout<<j<<"   ";
	cout<<"\n";
	for (i=0;i<Row;i++)
	{
		cout<<"\t\t\t\t";
		cout<<"["<<i<<"]  ";
		for(j=0;j<Col;j++)
		{
		   cout<<Matrix[i][j]<<"   ";
		}
		cout<<"\n\n";
	}
}
/**************************************************************************/
void PrintMatrixFloat(float Matrix[][Col],char Text[])
{
	int i,j;
	cout<<"\n=============The "<<Text<<"  ===============";
	cout<<"\n\n";
	cout<<"\t\t\t\t    ";
	for(j=0;j<Col;j++) cout<<j<<"   ";
	cout<<"\n";
	for (i=0;i<Row;i++)
	{
		cout<<"\t\t\t\t";
		cout<<"["<<i<<"]  ";
		for(j=0;j<Col;j++)
		{
		   cout<<Matrix[i][j]<<"   ";
		}
		cout<<"\n\n";
	}
}
/**************************************************************************/
int CheckExit(int Matrix[][Col],int x,int y)
{
	int Arrive=0;
	if (Matrix[x][y]==2) Arrive=1;
	return(Arrive);
}
/**************************************************************************/
void SelectFirstPlace(int &x,int &y,int Maze[][Col])
{
do
	{
		x=rand()%(8); y=rand()%(8);
		
	}while (Maze[x][y]==0);	
}
/**************************************************************************/
float SelectAction(int Matrix[][Col],int &x,int &y, int &Acttype)
{
	int Act=0,i=0;
	float Reward;
	int ChangePos=0;
	
	Act=1+rand()%(8);
	//cout<<"\n\t\t The X is: "<<x;
	//cout<<"\n\t\t The Y is: "<<y;
	cout<<"\n\t\t The Act is: "<<Act;
	switch (Act)
	{
		case 1:
			{
				if((x!=0) && ((Matrix[x-1][y]==1)||(Matrix[x-1][y]==2))) {x=x-1; y=y;ChangePos=1;}
				break;
			}
		case 2:
			{
				if((x!=0) && (y!=Col-1) && ((Matrix[x-1][y+1]==1)||(Matrix[x-1][y+1]==2))) {x=x-1; y=y+1;ChangePos=1;}
				break;
			}
		case 3:
			{
				if ((y!=Col-1) && ((Matrix[x][y+1]==1)||(Matrix[x][y+1]==2))) {x=x; y=y+1;ChangePos=1;}
				break;
			}
		case 4:
			{
				if((x!=Row-1) && (y!=Col-1) && ((Matrix[x+1][y+1]==1)||(Matrix[x+1][y+1]==2))) {x=x+1; y=y+1;ChangePos=1;}
				break;
			}
		case 5:
			{
				if((x!=Row-1) && ((Matrix[x+1][y]==1)||(Matrix[x+1][y]==2))) {x=x+1; y=y;ChangePos=1;}
				break;
			}
		case 6:
			{
				if((x!=Row-1) && (y!=0) && ((Matrix[x+1][y-1]==1)||(Matrix[x+1][y-1]==2))) {x=x+1; y=y-1; ChangePos=1;}
				break;
			}
		case 7:
			{
				if((y!=0) && ((Matrix[x][y-1]==1)||(Matrix[x][y-1]==2))) {x=x; y=y-1; ChangePos=1;}
				break;
			}
		case 8:
			{
				if((x!=0) && (y!=0) && ((Matrix[x+1][y-1]==1)||(Matrix[x+1][y-1]==2))) {x=x-1; y=y-1; ChangePos=1;}
				break;
			}							
	}
	Acttype=Act-1;
	if (((Matrix[x][y]==1)||(Matrix[x][y]==2)) && (ChangePos==1)) Reward=10; else Reward=0;
    cout<<"\n\t\t The New X is: "<<x+1;
	cout<<"\n\t\t The New Y is: "<<y+1;	
    cout<<"\n\t\t The Reward in Function is: "<<Reward;	
    return(Reward);		
}
/**************************************************************************/
void DetectPath(int Maze[][Col],int Path[][Col],int x,int y)
{
	if(Maze[x][y]==1) Path[x][y]=1;
	if(Maze[x][y]==2) Path[x][y]=2;
}
/**************************************************************************/
int TestZero(float Q[][Col],int NextCell)
{
	int sw=1,j;
	for(j=0;j<Col;j++)
	{
		if(Q[NextCell][j]!=0) sw=0;
	}
	return(sw);
}
/**************************************************************************/
float MaxQ(float Q[][Col],int NextCell)
{
	int i,j,IndexAct;
	float Maxq=-1;
      if (TestZero(Q,NextCell)==1)
         {
    	  IndexAct=rand()%(8);
    	  Maxq=Q[NextCell][IndexAct];    	
	     }
		for(j=0;j<Col;j++)
		{
			if(Maxq<Q[NextCell][j]) Maxq=Q[NextCell][j];
		}
		
   return(Maxq)	;
}
/**************************************************************************/

void UpDateQTable(float Q[][Col],int Curx,int Cury,float R)
{
	int Nextx;
	Nextx=rand()%(8);
	Q[Curx][Cury]=Q[Curx][Cury]
	+Alpha*(R+Gama*(MaxQ(Q,Nextx))-Q[Curx][Cury]);
}
//*************************************************************************/
int SelectMaxQ(float Q[][Col],int x,int y,int Checkmatrix[][Col])
{
	float MaxItem=-1;
	int i,j,MaxX,MaxY;
	int SelectAction;
	int SelectI,SelectJ;
	Checkmatrix[x][y]==1;
			
	if ((x!=0) && (MaxItem<Q[x-1][y]) && (Checkmatrix[x-1][y]==0))
	{
	  MaxItem=Q[x-1][y];
	  SelectAction=1;
	  SelectI=x-1;SelectJ=y;
	}
	if ((x!=0) && (y!=Col-1) && (MaxItem<Q[x-1][y+1]) && (Checkmatrix[x-1][y+1]==0)) 
	{
	  MaxItem=Q[x-1][y+1];
	  SelectAction=2;
	  SelectI=x-1;SelectJ=y-1;
	}
	if ((y!=Col-1) && (MaxItem<Q[x][y+1]) && (Checkmatrix[x][y+1]==0)) 
	{
	  MaxItem=Q[x][y+1];
	  SelectAction=3;
	  SelectI=x;SelectJ=y+1;
	}
	if ((x!=Row-1) && (y!=Col-1) && (MaxItem<Q[x+1][y+1]) && (Checkmatrix[x+1][y+1]==0)) 
	{
	  MaxItem=Q[x+1][y+1];
	  SelectAction=4;
	  SelectI=x+1;SelectJ=y+1;
	}
	if ((x!=Row-1) && (MaxItem<Q[x+1][y]) && (Checkmatrix[x+1][y]==0)) 
	{
	  MaxItem=Q[x+1][y];
	  SelectAction=5;
	  SelectI=x+1;SelectJ=y;
	}
	if ((x!=Row-1) && (y!=0) && (MaxItem<Q[x+1][y-1]) && (Checkmatrix[x+1][y-1]==0)) 
	{
	  MaxItem=Q[x+1][y-1];
	  SelectAction=6;
	  SelectI=x+1;SelectJ=y-1;
	}
	if ((y!=0) && (MaxItem<Q[x][y-1]) && (Checkmatrix[x][y-1]==0)) 
	{
	  MaxItem=Q[x][y-1];
	  SelectAction=7;
	  SelectI=x;SelectJ=y-1;
	}
	if ((x!=0) && (y!=0) && (MaxItem<Q[x-1][y-1]) && (Checkmatrix[x-1][y-1]==0)) 
	{
	  MaxItem=Q[x-1][y-1];
	  SelectAction=8;
	  SelectI=x-1;SelectJ=y-1;
	}
	Checkmatrix[SelectI][SelectJ]=1;	
	return(SelectAction);
}
//*************************************************************************/
void SelectPath(float Q[][Col],int X,int Y,int Maze[][Col])
{
	int Step=1;
	int x,y;
	int QAction;
	int CheckMat[Row][Col]={0};
	x=X;y=Y;
	int FinalPath[Row][Col]={0};
	while(Maze[x][y]!=2)
	{   cout<<"\n\t"<<x<<" "<<y<<" "<<Maze[x][y];
		QAction=SelectMaxQ(Q,x,y,CheckMat);
		cout<<"\n\t\t The Selected Action IS: "<<QAction;
		switch (QAction)
		{
			case 1:
				   FinalPath[x-1][y]=Step;
				   x=x-1;y=y;Step++;
				   break;
			case 2:
				   FinalPath[x-1][y+1]=Step;
				   x=x-1;y=y+1;Step++;
				   break;	
			case 3:
				   FinalPath[x][y+1]=Step;
				   x=x;y=y+1;Step++;
				   break;
			case 4:
				   FinalPath[x+1][y+1]=Step; 
				   x=x+1;y=y+1;Step++;
				   break;	
			case 5:
				   FinalPath[x+1][y]=Step; 
				   x=x+1;y=y;Step++;
				   break;
			case 6:
				   FinalPath[x+1][y-1]=Step;
				   x=x+1;y=y-1;Step++;
				   break;
			case 7:
				   FinalPath[x][y-1]=Step;
				   x=x;y=y-1;Step++;
				   break;
			case 8:	
			       FinalPath[x-1][y-1]=Step;
			       x=x-1;y=y-1;Step++;
				   break; 
		}
	//getch();
	}
	FinalPath[x][y]=Step;
  PrintMatrixInt(FinalPath,"FinalPath");
}

//*************************************************************************/
main()
{
	srand (time(NULL));
	int Maze[Row][Col]={0};
	int Path[Row][Col]={0};
	float QTable1[Row][Col]={0};
	
	int Arrival=0;
	int x1,y1,StartX1,StartY1;
	int ActType=0;
	float ActionReward=0;
	int Counter;
	int Iteration=0;
	
	int Nextx=0,Nexty=0;
	
	
	CreateMaze(Maze);
	PrintMatrixInt(Maze,"Maze");
	
	
	
	
	
	
	for(Counter=1;Counter<=EpisodeNumber;Counter++)
	{
	    SelectFirstPlace(x1,y1,Maze);
	    StartX1=x1;StartY1=y1;
	    cout<<"\n\tThe X is:"<<x1<<"\n\tThe Y is:"<<y1<<"\n\t The Item IS:"<<Maze[x1][y1];
	    Arrival=0;
	    Iteration=1;
		while((Arrival==0) && (Iteration<5000))
		{
			Arrival=CheckExit(Maze,x1,y1);
			
			ActionReward=SelectAction(Maze,x1,y1,ActType);
			DetectPath(Maze,Path,x1,y1);
			PrintMatrixInt(Maze,"Maze");
			PrintMatrixInt(Path,"Path");
			UpDateQTable(QTable1,x1,y1,ActionReward);
			PrintMatrixFloat(QTable1,"QTable Ag1");
			Iteration++;
			
			
		}
		cout<<"\n\n\t\t ---------The Counter Is: "<<Counter<<"\n";
		//getch();
		
	}
	cout<<"\n\t\t The Start Point Is: "<<StartX1<<"  "<<StartY1;
	SelectPath(QTable1,StartX1,StartY1,Maze);
	
	
}
