		                        /************************************************************************
		                         *																		*
		                         *		This Version Of Program Corrected at 10 Dec 2023                *
								 *                       and work correctly				                *
		                         *																		*
		                         ************************************************************************/
		                         /************************************************************************
		                         *																		*
		                         *		In This Version Of Program  all of Agents work Correctly        *
								 *																		*
		                         ************************************************************************/
		                         /************************************************************************
		                         *																		*
		                         *		In This Version Of Program  if any Agent Learned it exit        *
								 *																		*
		                         ************************************************************************/
								/************************************************************************
		                         *																		*
		                         *		In This Version Of Program  Chart Be Done Correclty             *
		                         *      any time can use of this version for comparing Criteria         *
		                         *      between three method include:                                   *
		                         *     1-PTST Method                                                    *								                                                                       *
								 *     2-T-MAS Method  		                                            *
								 *     3-T-KAg Method                                                   *
								 *																		*
		                         ************************************************************************/	
								/************************************************************************
		                         *											                            *
		                         *   In This Ver Of Program We want Improve Select Action By Q Table    *
								 *   Any Agent Select It's Action Via It's Q Table Using Greedy Method  *
								 *																		*
		                        /************************************************************************/	   
																 
								//*********************************************************************************************************//								                      
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <time.h>
#include <iomanip> 
//*********************************************************************************************************//
#define Num_Of_Agent 3
#define Num_Of_All_Agents 10 

#define Num_Of_Slices 9
#define Num_Of_Cells 9

#define Num_Of_Select 3 

#define Row 9   // Row Indicate The states ___Slices
#define Col 9   // Col Indicate The Actions ___Cells

#define Num_Of_Slice_Ag1 3 
#define Num_Of_Slice_Ag2 3
#define Num_Of_Slice_Ag3 3

#define Bound1 7
#define Bound2 3
#define Bound3 10

#define Alpha 0.1
#define Gamma 0.2
/******************************
 *							  *	
 *       Needed For Train	  *
 *							  *
 ******************************/
#define  Episode_Number_Train 1
#define  TrainEpisode 3
#define  Reward_Theroshold 20
/******************************
 *							  *	
 *       Needed For Test	  *
 *							  *
 ******************************/
#define Test_Episode 20
#define Reward_Theroshold_Proposed_Phase 50
#define Iteration  100
/**************************************
 *							  		  *	
 *       Needed For Proposed Method	  *
 *							  		  *
 **************************************/
/*#define  TS1   30  
#define  TS2   25
#define  TS3   20*/

#define  TS1   30 
#define  TS2   25
#define  TS3   10
/******************************
 *							  *	
 *       Needed For Result	  *
 *							  *
 ******************************/
#define Param 6
#define Res_Rec 100

#define T0 10    //Need For Certanity
#define Tmin  0  //Need For Certainty

#define CorrThreshold   5

/**************************************
 *							  		  *	
 *       Needed For Cell Selection	  *
 *							  		  *
 **************************************/
#define Threshold_select_cell  10
//*********************************************************************************************************//
using namespace std;
/***********************************************Functions************************************************ 
 *																										*
 *												Functions												*
 *																										*
 ***********************************************Functions************************************************/ 
 
//*********************************************************************************************************//
void SaveToFile(float Matrix[][Param+1])
{
	FILE *fp;
	fp=fopen("e:\MyResult.txt","w+t");
	if(fp==NULL)
	{
		cout<<"Can Not Open The File!!!";
		exit(0);
	}
	fwrite(Matrix,sizeof(float),Res_Rec*(Param)+1,fp);
	
}
//*********************************************************************************************************// 
void Print_Text(char Text[30])
{
	cout<<"\n\t\t-------------------------------------------------------------";
	cout<<"\n\t\t-                                                           -";
    cout<<"\n\t\t-                   "<<Text<<"                               ";
	cout<<"\n\t\t-                                                           -";    
	cout<<"\n\t\t-------------------------------------------------------------";
}
//*********************************************************************************************************//
void Reset_Matrix(float Res[][Param+1])
{
	int i,j;
	for(i=0;i<100;i++)
	{
		for(j=0;j<Param+1;j++)
		{
			Res[i][j]=0;
		}
	}
}
//*********************************************************************************************************// 
						/********************************************************
 						 *							  		  					*	
 						 *       This Function Print The Parameter Results	    *
 						 *							  		  					*
 						 ********************************************************/
void Print_Result(float Res[][Param+1])
{
   int i,j;
   Print_Text("The Result");
   cout<<"\n=============================================================================\n";
   
   cout<<"\n Learning\n";
   for(i=0;i<100;i++)
	{	
			cout<<"\t\t"<<Res[i][1];	
	 		cout<<"\n";
	}
	
	cout<<"\n Confidence\n";
	for(i=0;i<100;i++)
	{	
			cout<<"\t\t"<<Res[i][2];
	 		cout<<"\n";
	}
	
	cout<<"\n Expertness\n";
	for(i=0;i<100;i++)
	{	
			cout<<"\t\t"<<Res[i][3];
	 		cout<<"\n";
	}
	
	cout<<"\n Certainty\n";
	for(i=0;i<100;i++)
	{	
			cout<<"\t\t"<<Res[i][4];
	 		cout<<"\n";
	}
		
	cout<<"\n Efficiency\n";
	for(i=0;i<100;i++)
	{	
			cout<<"\t\t"<<Res[i][5];
	 		cout<<"\n";
	}	
	
	cout<<"\n Correcness\n";
	for(i=0;i<100;i++)
	{	
			cout<<"\t\t"<<Res[i][6];
	 		cout<<"\n";
	}			   		   
}

//*********************************************************************************************************//
void Print_Matrix_float(float Matrix[][Num_Of_Agent])
{
	int i,j;
	
	cout<<"\n\t\t==============================================";
	for(i=0;i<2;i++)
	{   cout<<"\n\t\t";
		for(j=0;j<Num_Of_Agent;j++)
		{   if(i==0) 
			cout<<Matrix[i][j]<<"\t\t";
			else
			cout<<Matrix[i][j]<<"\t";
		}
	cout<<"\n";
	}
}
//*********************************************************************************************************//
void Init_Q_Table(float Q[][Col])
{
	int i,j;
	for(i=0;i<Row;i++)
	{
		for(j=0;j<Col;j++)
		{
			Q[i][j]=0;
		}
	}
}
//*********************************************************************************************************//
						/********************************************************
 						 *							  		  					*	
 						 *       This Function Print The Q Tables			    *
 						 *							  		  					*
 						 ********************************************************/
void Print_Q_Table(float Q[][Col],int NumOfTable)
{
	int i,j;
	cout<<"\n----------------The Q Table Of "<<NumOfTable<<"  Is:----------\n\n";
	cout<<"\t\t\t";
	for(j=0;j<Col;j++) cout<<"["<<j<<"]   ";
	for(i=0;i<Row;i++)
	{   cout<<"\n\t ["<<i<<"]"<<"\t\t";
		for(j=0;j<Col;j++)
		{
			cout<<Q[i][j]<<"     ";
		}
	    cout<<"\n"	;
	}
}
//*********************************************************************************************************//
						/********************************************************
 						 *							  		  					*	
 						 *       This Function Print The Normalized Q table	    *
 						 *							  		  					*
 						 ********************************************************/
void PrintNormalizedQT(float Q[][Col],int NoOfAgent)
{
	float Max;
	int i,j;
	Max=Q[0][0];
	for(i=0;i<Row;i++)
	{
		for(j=0;j<Col;j++)
		{
			if(Max<Q[i][j]) Max=Q[i][j];			
		}
	}
	
	for(i=0;i<Row;i++)
	{  
	    for(j=0;j<Col;j++)
		{
          Q[i][j]=Q[i][j]/Max;
		}
	}
	cout<<"\n------------The Normalized Q Table  "<<NoOfAgent<<"  Is-----------\n";
    Print_Q_Table(Q,NoOfAgent)	;
}
//*********************************************************************************************************//
void CopyMatrix(float Q[][Col],float NQ[][Col])
{
	int i,j;
	for(i=0;i<Row;i++)
	{
		for(j=0;j<Col;j++)
		{
			NQ[i][j]=Q[i][j];
		}
	}
}
//*********************************************************************************************************//
void ResetList(int List[],int Size)
{
int i;

for(i=0;i<Size;i++)	 List[i]=0;
}
//*********************************************************************************************************//
void ResetListFloat(float List[],int Size)
{
int i;

for(i=0;i<Size;i++)	 List[i]=0;
}
//*********************************************************************************************************//
void Print_List(int List[],int Size,char Text[20])
{
int i;

    cout<<"\n--------"<<Text<<"   IS----------\n\n";
    cout<<"\t\t";
    for(i=0;i<Size;i++)	
       {
       	cout<<List[i]<<"    ";
	     }	
	cout<<"\n------------------------------------\n";
}
//*********************************************************************************************************//
void Print_ListFloat(float List[],int Size,char Text[20],int Slice,int CellSeleted)
{
int i;

    cout<<"\n--------"<<Text<<"   IS----------\n\n";
    cout<<"Slice IS: "<<Slice<<"\t\t";
    for(i=0;i<Size;i++)	
       {
       	cout<<List[i]<<"    ";
	     }	
	cout<<"         Cell Selected IS: "<<CellSeleted;
	cout<<"\n------------------------------------\n";
}
//*********************************************************************************************************//
Init_List_Of_Agent_Slice(int List_Of_Slice_Ag[],int Num_Of_Agent_Slice,int Possible_Slice_List[])
{
	int ItemIndex,i;
	
	for(i=0;i<Num_Of_Agent_Slice;i++)
	{
	ItemIndex=rand()%(Num_Of_Slices-0);
	while(Possible_Slice_List[ItemIndex]==-1)
	       {
	       	ItemIndex=rand()%(Num_Of_Slices-0);
		   }
	List_Of_Slice_Ag[i]=ItemIndex;
	Possible_Slice_List[ItemIndex]=-1;
	}
}
//*********************************************************************************************************//
int Select_Slice(int List[],int Num_Of_Selection,int Bound,int Fault[])
{
 	int ItemIndex;
	int PossibleItem;
	
	ItemIndex=rand()%(Bound-0);
	PossibleItem=List[ItemIndex];
	while((ItemIndex>=Num_Of_Selection)||(PossibleItem==-1)) 
	             {
	              ItemIndex=rand()%(Bound-0);
	              PossibleItem=List[ItemIndex];
	              Fault[0]++;
	             }	
		List[ItemIndex]=-1;
		return(PossibleItem);             
}
//*********************************************************************************************************//
int ZeroCounter(float Q[][Col],int Slice)
{
	int i,Counter=0;
	for(i=0;i<Col;i++)
	{
		if(Q[Slice][i]==0) Counter++;
	}
    return(Counter)	;
}
//*********************************************************************************************************//
int IS_There_Zero(float Q[][Col],int Slice)
{
	int Sw=0,j;
	
	for(j=0;j<Col;j++) if(Q[Slice][j]==0) Sw=1;
	return(Sw);
	
}
//*********************************************************************************************************//
int IS_There_Up_threshold(float Q[][Col],int Slice)
{
	int Sw=0,j;
	
	for(j=0;j<Col;j++) if(Q[Slice][j]>=Threshold_select_cell) Sw=1;
	return(Sw);		
}
//*********************************************************************************************************//
void CopyList(float Q[][Col],int Slice,float List[])
{
	int k;
	for(k=0;k<Col;k++)  List[k]=Q[Slice][k];
}

//*********************************************************************************************************//
int Select_Cell(int List[],float Q[][Col],int Slice)
{
	int i,j,k=0;
	int ItemIndex;
	int SelectIndex;
	int PossibleItem;
	float Possible_Action_List[Col];
	int Possible_Action_Index[Col];
	int TypeOfCells;
	
	int Num_Of_Zero;
	float MaxAction;
	
	ResetList(Possible_Action_Index,Col);
	ResetListFloat(Possible_Action_List,Col);
	
	Num_Of_Zero=ZeroCounter(Q,Slice);
	if (Num_Of_Zero==Col)  
	{
			ItemIndex=rand()%(Num_Of_Cells-0);//No one of Actin be selected
			TypeOfCells=1;  //1 Denote That All Cell Is Zero		
	}
	
	/************************************************************************************/
	else
	{
		//*******************************************************************//
		//*        select The Items Greater than  theroshold (10);			*//
		//*******************************************************************//  		
		if(IS_There_Up_threshold(Q,Slice)==1)
		   {
		    for(j=0;j<Col;j++)  
		   		 {
		   		  if(Q[Slice][j]>=Threshold_select_cell)
			         {
			   		  Possible_Action_Index[k]=j;
			   		  k++;	
			  		 } 		  		  	 
				 }  
   	        SelectIndex=rand()%(k-0);
		    ItemIndex=Possible_Action_Index[SelectIndex];
		    cout<<"\n\tthere is any item greater than thershold!!!\n";
		    TypeOfCells=2;// Denote That There IS some Cells Greater than thershold
		   }
 		//*******************************************************************//
		//*        checked the Items that not checked yet         			*//
		//*******************************************************************//   	
		else
		{
			if(IS_There_Zero(Q,Slice)==1) 
		      			{
		                 for(j=0;j<Col;j++)  
		                    {
		  		             if(Q[Slice][j]<Threshold_select_cell) 
							   {
							   	Possible_Action_Index[k]=j;
							   	k++;
							   }
		  			         
		   		            }	
		                SelectIndex=rand()%(k-0);
		                ItemIndex=Possible_Action_Index[SelectIndex];
						TypeOfCells=3;// Denote That There Are Some Cells That not verify yet		    		    		    
		               }				     					
		//***********************************************************************//
		//*there are some items that greater than zero and lower than theroshold*//
		//***********************************************************************//  		    					  				                  						        	
			else
			{
			 ItemIndex=rand()%(Num_Of_Cells-0);	
			 TypeOfCells=4;//Denote That There Some Cells That  Greater than Zero and Lower Than theroshold
			}	   		        	
				        								      		    				
         }
   }  
	     /************************************************************************************/
		 	   		
	switch 	(TypeOfCells)
	{
		case 1: cout<<"\nAll Of Cells Are Zero";
		        break;
		case 2: cout<<"\nThere Are Some Cells Greater Than theroshold";
		        break;  
		case 3: cout<<"\nThere Are Some Cells that not Verified Yet";
		        break;
		case 4: cout<<"\nThere All Cells That  Greater than Zero and Lower Than theroshold";
		        break;				      
	}
	
	cout<<"\nThe K IS: "<<k;	
	Print_List(Possible_Action_Index,k,"ACtion Possible Index");
	CopyList(Q,Slice,Possible_Action_List);
	Print_ListFloat(Possible_Action_List,Col,"The Slice Row: ",Slice,ItemIndex); 
	return(ItemIndex);
	
}
//*********************************************************************************************************//
                  /*##############################################################
                    #													 	     #
				    # 		       The Variable Definition Of Critic             #
				    #                      In Train Phase		 	             #	
				    #                                                            #
			        ##############################################################*/
//*********************************************************************************************************//
                    /********************************************************
                     *														*
                     *       Needed For  First Step To Ranking The Agents	*
                     *														*
                     ********************************************************/
float Envionment_Feedback(int Cell1,int Slice1,int Cell2,int Slice2,int Cell3,int Slice3)
{
	int R=0;
	if(Cell1==Slice1) R+=10; //else R-=1;
	if(Cell2==Slice2) R+=10; //else R-=1;
	if(Cell3==Slice3) R+=10; //else R-=1;
					
	return(R);
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Equal(float Reward,float List[])
{
	int i;
	for(i=0;i<Num_Of_Agent;i++)  List[i]=Reward/Num_Of_Agent;	
}
//*********************************************************************************************************//
                  /*##############################################################
                    #													 	     #
				    # 		       The Variable Definition Of Q Learning         #
				    #                      			 	                         #	
				    #                                                            #
			        ##############################################################*/
//*********************************************************************************************************//
float MaxQ(float Q[][Col],int NextSlice)
{
	int i,j;
	float Maxq=-1;

		for(j=0;j<Col;j++)
		{
			if(Maxq<Q[NextSlice][j]) Maxq=Q[NextSlice][j];
		}
		
   return(Maxq)	;
}
//---------------------------------------------------------------------------------------------------------//
void UpDate_QTable(float Q[][Col],int Slice,int Cell,float R,int Next_Slice)
{
	Q[Slice][Cell]=Q[Slice][Cell]+Alpha*(R+Gamma*(MaxQ(Q,Next_Slice))-Q[Slice][Cell]);
}
//*********************************************************************************************************//
                    /********************************************************
                     *														*
                     *       Needed For  First Step To Ranking The Agents	*
                     *														*
                     ********************************************************/
void SortMatrix(float Matrix[][Num_Of_Agent])
{
  int i,j;
  float TempIndex,TempValue;
  for(j=2;j>=0;j--)	
  {
  	for(i=0;i<j;i++)
  	{
  		if(Matrix[1][i]<Matrix[1][i+1])
  		{
  			TempIndex=Matrix[0][i];
  			TempValue=Matrix[1][i];
  			
  			Matrix[0][i]=Matrix[0][i+1];
  			Matrix[1][i]=Matrix[1][i+1];
  			
  			Matrix[0][i+1]=TempIndex;
  			Matrix[1][i+1]=TempValue;
  			
		  }
	  }
  }
  Print_Text("Sorted Agents Via Their Expertness");
  Print_Matrix_float(Matrix);  
}
//*********************************************************************************************************//
                  /*##############################################################
                    #													 	     #
				    # 		       The Variable Definition Of Critic             #
				    #                      In Propose Method		             #	
				    #                                                            #
			        ##############################################################*/
//*********************************************************************************************************//
                  
//*********************************************************************************************************//
void Critic_DB(float DBReward,float DBList[])
{
	
	DBList[0]=0.6*DBReward;
	DBList[1]=0.3*DBReward;
	DBList[2]=0.1*DBReward;
}
//---------------------------------------------------------------------------------------------------------//
void Critic_DB_Without_Ag1(float DBReward,float DBList[])
{
    DBList[1]=0.3*DBReward;
	DBList[2]=0.1*DBReward;	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_DB_Without_Ag2(float DBReward,float DBList[])
{
    DBList[0]=0.6*DBReward;
	DBList[2]=0.1*DBReward;	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_DB_Without_Ag3(float DBReward,float DBList[])
{
    DBList[0]=0.6*DBReward;
	DBList[1]=0.3*DBReward;	
}
//---------------------------------------------------------------------------------------------------------//

void Critic_DB_Without_Ag1_Ag2(float DBReward,float DBList[])
{
   	DBList[2]=0.1*DBReward;	
}
//---------------------------------------------------------------------------------------------------------//

void Critic_DB_Without_Ag1_Ag3(float DBReward,float DBList[])
{
   	DBList[1]=0.3*DBReward;	
}
//---------------------------------------------------------------------------------------------------------//

void Critic_DB_Without_Ag2_Ag3(float DBReward,float DBList[])
{
   	DBList[0]=0.6*DBReward;	
}
//*********************************************************************************************************//
                  /*##############################################################
                    #													 	     #
				    # 		                  PTST Method    	                 #	
				    #                                                            #
			        ##############################################################*/
//*********************************************************************************************************//
void Critic_Type_1(float Reward,float RList[])
{
	if(Reward>=TS1)    
	{    
	   RList[0]=TS1;
	   Reward-=TS1; 	
	}
    
	if(Reward>=TS2)    
	{
		RList[1]=TS2;
		Reward-=TS2;
    }   
	
	if(Reward>=TS3) 
	{
	  RList[2]=TS3;	
	  Reward-=TS3;
	}
    
/*	int TS;
	
	TS=TS1+TS2+TS3;
	
	ResetListFloat(RList,Num_Of_Agent);
	
	//if(Reward>=TS1)  
	   {
	   		   	
	   	RList[0]=TS1;
	   	Reward-=RList[0];
	   	RList[0]+=double(TS3*Reward)/double(TS);
	   	Reward-=RList[0];
	   	cout<<" \n The R1 IS:"<<RList[0];
	   	cout<<" \n The Reward IS: "<<Reward;
	   	
	   	}
	if(Reward>=TS2)   
	    {
	    cout<<" \n The Reward IS: "<<Reward;
	    RList[1]=TS2;
	    cout<<" \n The R2 IS:"<<RList[1];
	    Reward-=RList[1];
	    cout<<" \n The Reward IS: "<<Reward;
	    if (Reward>0)  
		    {
		    cout<<" \n The Reward IS: "<<Reward;
		    RList[1]+=double(TS2*Reward)/double(TS);
			cout<<" \n The R2 IS:"<<RList[1];	
		    Reward-=RList[1];
		    cout<<" \n The Reward IS: "<<Reward;
			} 			   			    
		}				
	if(Reward>=TS3)   
	    {
	    cout<<" \n The Reward IS: "<<Reward; 	
	    RList[2]=TS3;
	    cout<<" \n The R2 IS:"<<RList[2];
	    Reward-=RList[2];
	    cout<<" \n The Reward IS: "<<Reward; 	
	    if (Reward>0) 
		    {
			RList[2]+=double(TS1*Reward)/double(TS);
			cout<<" \n The Reward IS: "<<Reward; 
			Reward-=RList[2];
			cout<<" \n The R2 IS:"<<RList[2];
		   }
		cout<<" \n The R3 IS:"<<RList[2];
	   	cout<<" \n The Reward IS: "<<Reward;   	    
		}
			
	   if(Reward>0)	{
	   	             RList[0]+=Reward;
	   	             cout<<" \n The Reward IS: "<<Reward; 
	                 Reward-=RList[0];
	                 cout<<" \n The R2 IS:"<<RList[0];
	                }	
	   
	   if(Reward>0){
	   	            RList[1]+=Reward;
	   	            cout<<" \n The Reward IS: "<<Reward; 
	                Reward-=RList[1];
	                cout<<" \n The R2 IS:"<<RList[1];
	               }
	   
	   if(Reward>0){
	   	           RList[2]+=Reward;
	   	           cout<<" \n The Reward IS: "<<Reward;
	               Reward-=RList[2];
	               cout<<" \n The R2 IS:"<<RList[2];
	              }*/
	   
}
//---------------------------------------------------------------------------------------------------------//
Critic_Type_1_Without_Ag1(float Reward,float RList[])
{
	RList[0]=0;
	
	if(Reward>=TS2)    
	{
		RList[1]=TS2;
		Reward-=TS2;
    }   
	   
	if(Reward>=TS3) 
	{
	  RList[2]=TS3;	
	  Reward-=TS3;
	}
}
//---------------------------------------------------------------------------------------------------------//
Critic_Type_1_Without_Ag2(float Reward,float RList[])
{
	RList[1]=0;
	
	if(Reward>=TS1)    
	{
		RList[0]=TS1;
		Reward-=TS1;
    }   
	   
	if(Reward>=TS3) 
	{
	  RList[2]=TS3;	
	  Reward-=TS3;
	}
}
//---------------------------------------------------------------------------------------------------------//
Critic_Type_1_Without_Ag3(float Reward,float RList[])
{
	RList[2]=0;
	
	if(Reward>=TS1)    
	{
		RList[0]=TS1;
		Reward-=TS1;
    }   
	   
	if(Reward>=TS2) 
	{
	  RList[1]=TS2;	
	  Reward-=TS2;
	}
}
//---------------------------------------------------------------------------------------------------------//
Critic_Type_1_Without_Ag1_Ag2(float Reward,float RList[])
{
	RList[0]=0;RList[1]=0;
	
  
	if(Reward>=TS3) 
	{
	  RList[2]=TS3;	
	  Reward-=TS3;
	}
}
//---------------------------------------------------------------------------------------------------------//
Critic_Type_1_Without_Ag1_Ag3(float Reward,float RList[])
{
	RList[0]=0;RList[2]=0;
	
  
	if(Reward>=TS2) 
	{
	  RList[1]=TS2;	
	  Reward-=TS2;
	}
}
//---------------------------------------------------------------------------------------------------------//
Critic_Type_1_Without_Ag2_Ag3(float Reward,float RList[])
{
	RList[1]=0;RList[2]=0;
	
  
	if(Reward>=TS1) 
	{
	  RList[0]=TS1;	
	  Reward-=TS1;
	}
}
//---------------------------------------------------------------------------------------------------------//
//*********************************************************************************************************//
                  /*##############################################################
                    #													 	     #
				    # 		             T-MAS  Nethod	                         #	
				    #                                                            #
			        ##############################################################*/
//*********************************************************************************************************//
void Critic_Type_TS_Agents_priority(float Reward,float RList[])
{
	float r1,r2,r3;
	
	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}
	
	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}
	
	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
		RList[0]+=((r1*r1)/(r1*r1+r2*r2+r3*r3))*Reward;
		RList[1]+=((r2*r2)/(r1*r1+r2*r2+r3*r3))*Reward;
		RList[2]+=((r3*r3)/(r1*r1+r2*r2+r3*r3))*Reward;
		cout<<"\nr1 IS: "<<RList[0];
		cout<<"\nr2 IS: "<<RList[1];
		cout<<"\nr3 IS: "<<RList[2];
	//	getch();
	}	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Agents_priority_Without_Ag1(float Reward,float RList[])
     
{
	float r1,r2,r3;
	
	RList[0]=0;
/*	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}*/
	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}
	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
	//	RList[0]+=(r1*r1)/(r1*r1+r2*r2+r3*r3)*Reward;
		RList[1]+=((r2*r2)/(r1*r1+r2*r2+r3*r3))*Reward;
		RList[2]+=((r3*r3)/(r1*r1+r2*r2+r3*r3))*Reward;
	//	cout<<"\nr1 IS: "<<RList[0];
		cout<<"\nr2 IS: "<<RList[1];
		cout<<"\nr3 IS: "<<RList[2];
	//	getch();
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Agents_priority_Without_Ag2(float Reward,float RList[])
     
{
	float r1,r2,r3;
	
	RList[1]=0;
	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}
/*	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}*/
	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
		RList[0]+=(r1*r1)/(r1*r1+r2*r2+r3*r3)*Reward;
	//	RList[1]+=((r2*r2)/(r1*r1+r2*r2+r3*r3))*Reward;
		RList[2]+=((r3*r3)/(r1*r1+r2*r2+r3*r3))*Reward;
		cout<<"\nr1 IS: "<<RList[0];
	//	cout<<"\nr2 IS: "<<RList[1];
		cout<<"\nr3 IS: "<<RList[2];
	//	getch();
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Agents_priority_Without_Ag3(float Reward,float RList[])
     
{
	float r1,r2,r3;
	
	RList[2]=0;
	
	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}
	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}
/*	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}*/
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
		RList[0]+=(r1*r1)/(r1*r1+r2*r2+r3*r3)*Reward;
		RList[1]+=((r2*r2)/(r1*r1+r2*r2+r3*r3))*Reward;
	//	RList[2]+=((r3*r3)/(r1*r1+r2*r2+r3*r3))*Reward;
		cout<<"\nr1 IS: "<<RList[0];
		cout<<"\nr2 IS: "<<RList[1];
	//	cout<<"\nr3 IS: "<<RList[2];
	//	getch();
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Agents_priority_Without_Ag1_Ag2(float Reward,float RList[])
     
{
	float r1,r2,r3;
	
	RList[0]=0;RList[1]=0;
/*	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}*/
/*	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}*/
	
	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
	//	RList[0]+=(r1*r1)/(r1*r1+r2*r2+r3*r3);
	//	RList[1]+=(r2*r2)/(r1*r1+r2*r2+r3*r3);
		RList[2]+=((r3*r3)/(r1*r1+r2*r2+r3*r3))*Reward;
	//	cout<<"\nr1 IS: "<<RList[0];
	//	cout<<"\nr2 IS: "<<RList[1];
		cout<<"\nr3 IS: "<<RList[2];
	//	getch();
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Agents_priority_Without_Ag1_Ag3(float Reward,float RList[])
     
{
	float r1,r2,r3;
	
	RList[0]=0;RList[2]=0;
/*	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}*/
	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}
	
/*	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}*/
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
	//	RList[0]+=(r1*r1)/(r1*r1+r2*r2+r3*r3);
		RList[1]+=(r2*r2)/(r1*r1+r2*r2+r3*r3);
	//	RList[2]+=((r3*r3)/(r1*r1+r2*r2+r3*r3))*Reward;
	//	cout<<"\nr1 IS: "<<RList[0];
		cout<<"\nr2 IS: "<<RList[1];
	//	cout<<"\nr3 IS: "<<RList[2];
	//	getch();
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Agents_priority_Without_Ag2_Ag3(float Reward,float RList[])
     
{
	float r1,r2,r3;
	
	RList[1]=0;RList[2]=0;
	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}
/*	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}*/
	
/*	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}*/
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
		RList[0]+=(r1*r1)/(r1*r1+r2*r2+r3*r3);
	//	RList[1]+=(r2*r2)/(r1*r1+r2*r2+r3*r3);
	//	RList[2]+=((r3*r3)/(r1*r1+r2*r2+r3*r3))*Reward;
		cout<<"\nr1 IS: "<<RList[0];
	//	cout<<"\nr2 IS: "<<RList[1];
	//	cout<<"\nr3 IS: "<<RList[2];
	//	getch();
	}
	
}
//---------------------------------------------------------------------------------------------------------//
//*********************************************************************************************************//
                  /*##############################################################
                    #													 	     #
				    # 		             T-KAg Nethod	   			             #	
				    #                                                            #
			        ##############################################################*/
//*********************************************************************************************************//
void Critic_Type_TS_Expert_Agents_priority(float Reward,float RList[])
{
	float r1,r2,r3;
	
	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}
	
	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}
	
	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
		if(RList[0]>0)	RList[0]+=Reward;
		Reward=0;
	}
	if(Reward>0)
	{
	    if(RList[1]>0)	RList[1]+=Reward;
		Reward=0;	
	}
	if(Reward>0)
	{
	    if(RList[2]>0)	RList[2]+=Reward;
		Reward=0;	
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Expert_Agents_priority_Without_Ag1(float Reward,float RList[])
{
	float r1,r2,r3;
	
	RList[0]=0;
	/*if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}*/
	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}
	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
/*	if(Reward>0) 
	{
		if(RList[0]>0)	RList[0]+=Reward;
		Reward=0;
	}*/
	if(Reward>0)
	{
	    if(RList[1]>0)	RList[1]+=Reward;
		Reward=0;	
	}
	if(Reward>0)
	{
	    if(RList[2]>0)	RList[2]+=Reward;
		Reward=0;	
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Expert_Agents_priority_Without_Ag2(float Reward,float RList[])
{
	float r1,r2,r3;
	
	RList[1]=0;
	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}
	
/*	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}*/
	
	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
		if(RList[0]>0)	RList[0]+=Reward;
		Reward=0;
	}
	
/*	if(Reward>0)
	{
	    if(RList[1]>0)	RList[1]+=Reward;
		Reward=0;	
	}*/
	
	if(Reward>0)
	{
	    if(RList[2]>0)	RList[2]+=Reward;
		Reward=0;	
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Expert_Agents_priority_Without_Ag3(float Reward,float RList[])
{
	float r1,r2,r3;
	
	RList[2]=0;
	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}
	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}
	/*if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}*/
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
		if(RList[0]>0)	RList[0]+=Reward;
		Reward=0;
	}
	if(Reward>0)
	{
	    if(RList[1]>0)	RList[1]+=Reward;
		Reward=0;	
	}
/*	if(Reward>0)
	{
	    if(RList[2]>0)	RList[2]+=Reward;
		Reward=0;	
	}*/
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Expert_Agents_priority_Without_Ag1_Ag2(float Reward,float RList[])
{
	float r1,r2,r3;
	
	RList[0]=0;RList[1]=0;
	/*if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}*/
	/*if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}*/
	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
/*	if(Reward>0) 
	{
		if(RList[0]>0)	RList[0]+=Reward;
		Reward=0;
	}*/
/*	if(Reward>0)
	{
	    if(RList[1]>0)	RList[1]+=Reward;
		Reward=0;	
	}*/
	if(Reward>0)
	{
	    if(RList[2]>0)	RList[2]+=Reward;
		Reward=0;	
	}
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Expert_Agents_priority_Without_Ag1_Ag3(float Reward,float RList[])
{
	float r1,r2,r3;
	
	RList[0]=0;RList[2]=0;
	/*if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}*/
	if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}
/*	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}*/
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
/*	if(Reward>0) 
	{
		if(RList[0]>0)	RList[0]+=Reward;
		Reward=0;
	}*/
	if(Reward>0)
	{
	    if(RList[1]>0)	RList[1]+=Reward;
		Reward=0;	
	}
/*	if(Reward>0)
	{
	    if(RList[2]>0)	RList[2]+=Reward;
		Reward=0;	
	}*/
	
}
//---------------------------------------------------------------------------------------------------------//
void Critic_Type_TS_Expert_Agents_priority_Without_Ag2_Ag3(float Reward,float RList[])
{
	float r1,r2,r3;
	
	RList[1]=0;RList[2]=0;
	if(Reward>=TS1) 
	{
		RList[0]=TS1;
		Reward=Reward-TS1;
	}
	/*if(Reward>=TS2) 
	{
		RList[1]=TS2;
		Reward=Reward-TS2;
	}*/
/*	if(Reward>=TS3) 
	{
		RList[2]=TS3;
		Reward=Reward-TS3;
	}*/
	
	r1=RList[0]; r2=RList[1]; r3=RList[2];
	if(Reward>0) 
	{
		if(RList[0]>0)	RList[0]+=Reward;
		Reward=0;
	}
/*	if(Reward>0)
	{
	    if(RList[1]>0)	RList[1]+=Reward;
		Reward=0;	
	}*/
/*	if(Reward>0)
	{
	    if(RList[2]>0)	RList[2]+=Reward;
		Reward=0;	
	}*/
	
}
//---------------------------------------------------------------------------------------------------------//
//*********************************************************************************************************//
                  /*##############################################################
                    #													 	     #
				    # 		       The Variable Definition Of Environment        #
				    #                      In Propose Method		             #	
				    #                                                            #
			        ##############################################################*/
//*********************************************************************************************************//
float Envionment_Feedback_Agent1(int Slice,int Cell)
{
	float R=0;
	if(Slice==Cell) R=35;              //else R=-5;
	else R=0;
	
	return(R) ;
}
//---------------------------------------------------------------------------------------------------------//
float Envionment_Feedback_Agent2(int Slice,int Cell)
{
    float R=0;
	if(Slice==Cell) R=25;			   //else R=-5;	
	else R=0;
	
	return(R) ;
}
//---------------------------------------------------------------------------------------------------------//
float Envionment_Feedback_Agent3(int Slice,int Cell)
{
	float R=0;
	if(Slice==Cell) R=20;			   //else R=-5;	
	else R=0;
	
	return(R) ;
}
//---------------------------------------------------------------------------------------------------------//
                  /*##############################################################
                    #													 	     #
				    # 		       The Variable Definition Of Environment        #
					#                       when Agent1 Learnt                   #
				    #                       In Propose Method		             #	
				    #                                                            #
			        ##############################################################*/
//---------------------------------------------------------------------------------------------------------//
float Envionment_Feedback_Agent2_Without_Ag1(int Slice,int Cell)
{
    float R=0;
	if(Slice==Cell) R=45;              //else R=-5;
	else R=0;
	
	return(R) ;
}
//---------------------------------------------------------------------------------------------------------//
float Envionment_Feedback_Agent3_Without_Ag1(int Slice,int Cell)
{
	float R=0;
	if(Slice==Cell) R=40;              //else R=-5;
	else R=0;
	
	return(R) ;
}
//---------------------------------------------------------------------------------------------------------//
                  /*##############################################################
                    #													 	     #
				    # 		       The Variable Definition Of Environment        #
					#                  when Agent1 and Agent2 Learnt             #
				    #                       In Propose Method		             #	
				    #                                                            #
			        ##############################################################*/
//---------------------------------------------------------------------------------------------------------//
float Envionment_Feedback_Agent3_Without_Ag1_Ag2(int Slice,int Cell)
{
	float R=0;
	if(Slice==Cell) R=65;              //else R=-5;
	else R=0;
	
	return(R) ;
}
//---------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------------------------//
                  /*##############################################################
                    #													 	     #
				    # 		       In This Section The User Select               #
					#                   The method from Menu                     #
					#                  Include:									 #
					#		         1- PTST  Method(Press 1)        	         # 					
					#				 2- T-MAS Priority(Press 2)       	         #
					#				 3- T-KAg Priority(Press 3)                  #
				    #                                                            #
			        ##############################################################*/
//---------------------------------------------------------------------------------------------------------//

int CriticMenu(void)
{
   int Item=0;	
   while((Item<1)||(Item>3))
   {
   
   Print_Text("Please Select Critic Type ")	;
   cout<<"\n\t\t-------------------------------------------------------------";
   cout<<"\n\t\t-                                                           -";
   cout<<"\n\t\t-            1- PTST  Method(Press 1)                       -";
   cout<<"\n\t\t-            2- T-MAS Method(Press 2)                       -";
   cout<<"\n\t\t-            3- T-KAg Method(Press 3)                       -";
   cout<<"\n\t\t-------------------------------------------------------------";
   cout<<" \n\n\t\t Please Select Your Option: ";
   cin>>Item;   
  }
  return(Item);
}
//*********************************************************************************************************//
                  /*##############################################################
                    #													 	     #
				    #        In The Below We define Parameter Should Be Used     #
					#                       in Different Method		             #	
				    #       include:                                             #
					#                 1-Learning Ratio                           #
					#                 2-Confidence                               #
					#                 3-Expertness                               #
					#                 4-Certainty                                #
					#                 5-Efficiency                               #
					#                 6-Correctness                              #
					#                 7-Density                                  #
			        ##############################################################*/
//*********************************************************************************************************//

//---------------------------------------------------------------------------------------------------------//
                  /*##############################################################
                    #													 	     #
				    # 		                1- Learning Ratio                    #	
				    #                                                            #
			        ##############################################################*/
//---------------------------------------------------------------------------------------------------------//
int LearningRate(float Q[][Col],int NoOfAgent)
{
	int i,j;
	int IndexI,IndexJ,LearnNum=0;
	float MaxItem;
	for(i=0;i<Row;i++)
	{
		MaxItem=Q[i][0];
		for(j=0;j<Col;j++)
		{
            if (MaxItem<=Q[i][j])  
			{ 
			 MaxItem=Q[i][j];
			 IndexI=i;
			 IndexJ=j;
			  }
			
		}
		if(IndexI==IndexJ)   LearnNum++;

	}
 
return(LearnNum);
}
//---------------------------------------------------------------------------------------------------------//
                  /*##############################################################
                    #													 	     #
				    # 		                2- Confidence	                     #	
				    #                                                            #
			        ##############################################################*/
//---------------------------------------------------------------------------------------------------------//
float Confidence(float Q[][Col],int NumOfAgent)
{
	float SumOfMax1=0,SumOfMax2=0;
	int i,j;
	float MaxItem1,MaxItem2;
	float Confidence=0;
	
	for(i=0;i<Row;i++)
	{
		if(ZeroCounter(Q,i)==Col)
		{
			MaxItem1=MaxItem2=0;
		}
		else
		{		
		      MaxItem1=-1000000;MaxItem2=MaxItem1;
		       for(j=0;j<Col;j++)
		            {
			          if(MaxItem1<Q[i][j]) MaxItem1=Q[i][j];			
		            }
		       for(j=0;j<Col;j++)
		            {
			         if((MaxItem2<Q[i][j])&&(Q[i][j]<MaxItem1)) MaxItem2=Q[i][j];
		            }
	    }
	    SumOfMax1+=MaxItem1;
	    SumOfMax2+=MaxItem2;
	/*	cout<<"\n----------------------------------------------------";
		cout<<"\nThe Fisrt Maximum In Slice "<<i<<" IS:    "<<MaxItem1;
		cout<<"\nThe Second Maximum In Slice "<<i<<" IS:    "<<MaxItem2;
		cout<<"\n---------------------------------------------------";*/
	}
	cout<<"\n The Avg Of Max1 IS: "<<double(SumOfMax1/9);
	cout<<"\n The Sum Of Max2 IS: "<<double(SumOfMax2/9);
	
	Confidence=(double(SumOfMax1/9)-double(SumOfMax2/9));
	cout<<"\n The Confidence Of Agent "<<NumOfAgent<<" IS: "<<Confidence;
	
	return(Confidence);
}
//---------------------------------------------------------------------------------------------------------//
                  /*##############################################################
                    #													 	     #
				    # 		                4-Certainty 	                     #	
				    #                                                            #
			        ##############################################################*/
//---------------------------------------------------------------------------------------------------------//
float Certainty(float Q[][Col],long int Episode_Number_Ag,int Slice,int Cell)
{
	int j;
	float Cert=0,T=0;
	float Sum_Of_Action=0;
	
	if (Tmin>=(T0/(1+log(Episode_Number_Ag)))) T=Tmin;
	else T=T0/(1+log(Episode_Number_Ag));
	
	for(j=0;j<Col;j++) Sum_Of_Action+=(exp(Q[Slice][j])/T);
	
	Cert=(exp(Q[Slice][Cell])/T)/Sum_Of_Action;
	return(Cert);
}
//*********************************************************************************************************//
//---------------------------------------------------------------------------------------------------------//
                  /*##############################################################
                    #													 	     #
				    # 		           End Of Function Definition		         #	
				    #                                                            #
			        ##############################################################*/
//---------------------------------------------------------------------------------------------------------//
//*********************************************************************************************************//


main()
{

    //long int ProgCounter=0;

	float Q1[Row][Col],NormQ1[Row][Col];
	float Q2[Row][Col],NormQ2[Row][Col];
	float Q3[Row][Col],NormQ3[Row][Col];	
	
	long int Episode_Counter;
	//********************The Below Definition Indicates The Num Of Slices That Agent Should be Completed************************/
	int List_Of_Slice_Ag1[Num_Of_Slice_Ag1];   //The Slices That Agent Should Be Solve
	int List_Of_Slice_Ag2[Num_Of_Slice_Ag2];
	int List_Of_Slice_Ag3[Num_Of_Slice_Ag3];
   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //********************The Below Definition Indicates The Num Of Slices and Cells  That Agent Can be Select*******************/
	int Possible_Slice_List[Num_Of_Slices];  //All of Slices That Assign To Agent's
	int Possible_Cell_List[Num_Of_Cells];    //All of Cells That Agents select  (Not Need I Think!!!)
   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //********************The Below Definition Indicates The Num Of Fault Of Agents**********************************************/	
	int Fault1[1]={0};  //The fault That Any Agent Done
	int Fault2[1]={0};
	int Fault3[1]={0};
   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //********************The Below Definition Indicates The Slices And Celss That Agents will be Select*************************/		
	int Slice1,Slice2,Slice3;
	int Cell1,Cell2,Cell3;
	
	int Next_Slice1,Next_Slice2,Next_Slice3;
	int Next_Cell1,Next_Cell2,Next_Cell3;
   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //*******The Below Definition Indicates The Reward Of Train And Episode: in Train Phase Any Agent select a Slice And a Cell***/		
	float MAS_Reward_Train=0;
	float MAS_Reward_Episode=0;
   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //*********************The Below Definition Indicates The Reward that any agent recieve from critic**************************/	
	float Reward_List[Num_Of_Agent];
	float R1=0,R2=0,R3=0;
	
	int i;
	long int Round=0;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //*********************The Below Definition Indicates The Variables that nedded for Analayzing********************************/	
    int LR1,LR2,LR3;
    long int Num_Of_Try=0;
    long int Num_Of_Try_Ag1=0,Num_Of_Try_Ag2=0,Num_Of_Try_Ag3=0;
	    
    float ALR1,ALR2,ALR3; //Agent Learning Rate
    float GLR=0;//Group Learning Rate
     
    float List_Of_ExpertNess[2][Num_Of_Agent]={1,2,3,0,0,0};
    
    int BoundList[Num_Of_Agent]={Bound1,Bound2,Bound3};
    int Bound_Index1,Bound_Index2,Bound_Index3;
    int B1,B2,B3;
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //*********************The Below Definition Indicates The Variables that nedded for Proposed Method**************************/	
    int Episode_Number_Ag1=0;
    int Episode_Number_Ag2=0;
    int Episode_Number_Ag3=0;  
	  
    float MAS_Reward_Test=0;
    
    int Critic_Type;
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //*********************The Below Definition Indicates The Variables that nedded Parameter************************************/	
    float Cnf1,Cnf2,Cnf3,MAS_Cnf;
    
    int NR_Ag1=0,NR_Ag2=0,NR_Ag3=0;
    int NP_Ag1=0,NP_Ag2=0,NP_Ag3=0;
    float Exp_Ag1=0,Exp_Ag2=0,Exp_Ag3=0;
    float MAS_Exp=0;
    int Sw_Ag1_Select=0;
    int Sw_Ag2_Select=0;
    int Sw_Ag3_Select=0;
    
    long int  Exp_Try1=0;
    long int  Exp_Try2=0;
    long int  Exp_Try3=0;
	    
    long int  NR_Ag1_Train=0 , NP_Ag1_Train=0;
    long int  NR_Ag2_Train=0 , NP_Ag2_Train=0;
    long int  NR_Ag3_Train=0 , NP_Ag3_Train=0;
    
    float Result[Res_Rec][Param+1];
    int R_Index_Res=0;
    int C_Index_res=0;
    
    long int Cert_Counter=0; //Need For Certainty
    float Sum_Of_Cert=0;
    float Cert_Episode=0;
    
    long int  Non_Zero_Reward=0;//Need For Effiecny
    long int  Non_Zero_R1=0;
    long int  Non_Zero_R2=0;
    long int  Non_Zero_R3=0;
    
    float MAS_Effic=0;
    
    float RealRewardAg1;
    float RealRewardAg2;
    float RealRewardAg3;
    
    float CorrAg1,CorrAg2,CorrAg3;
    float MASCorr=0;
    
    long int  NumOfRewarding=0;
    
   // for(ProgCounter=0;ProgCounter<100;ProgCounter++)
   // {

   // R_Index_Res=0;
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    
    //----------------------------------------------------The End Of Definitions-----------------------------------------------//
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	    
	srand (time(NULL));	
	Critic_Type=CriticMenu();
 //   Critic_Type=1;
	Reset_Matrix(Result);
   	   /*************************************************************************
		*		In This Section We Initialize The Q Tables    				    *
		*																		*
		*																		*
		*************************************************************************/
	Init_Q_Table(Q1);   Init_Q_Table(NormQ1);
	Init_Q_Table(Q2);   Init_Q_Table(NormQ2);
	Init_Q_Table(Q3);   Init_Q_Table(NormQ3);
	
	Print_Q_Table(Q1,1);
	Print_Q_Table(Q2,2);
	Print_Q_Table(Q3,3);
	
	
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	   /*************************************************************************
		*																		*
		*           In Below We Start To Train Machine		  				    *
		*																		*
		*																		*
		*************************************************************************/  	
	    
	for(Episode_Counter=0;Episode_Counter<Episode_Number_Train;Episode_Counter++)
	{
	   MAS_Reward_Episode=0;
	          	
	   while(MAS_Reward_Episode<Reward_Theroshold)
	   {
		 NR_Ag1=0;NP_Ag1=0;
		 NR_Ag2=0;NP_Ag2=0;
		 NR_Ag3=0;NP_Ag3=0;
		/************************************************************************
		*	In This Section Any List WIll be Reset These List Include:			*
		*	1.List Of  Slices That Agent should be completed					*
		*	2.Possible List of Slice that Similar to a Slice Box!               *
		*	3.Possible List of Cell that Similar to a Cell Box(Not Needed !!!   *
		*************************************************************************/
		ResetList(List_Of_Slice_Ag1,Num_Of_Slice_Ag1);
		ResetList(List_Of_Slice_Ag2,Num_Of_Slice_Ag2);
		ResetList(List_Of_Slice_Ag3,Num_Of_Slice_Ag3);
		
		Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		
		ResetList(Possible_Slice_List,Num_Of_Slices);
		Print_List(Possible_Slice_List,Num_Of_Slices,"List Of Possible Slices ");
		
		ResetList(Possible_Cell_List,Num_Of_Cells);
		Print_List(Possible_Cell_List,Num_Of_Cells,"List Of Possible Cells ");//Not Needed!!!
		
		/************************************************************************
		*	In This Section We Initialize The Slice List Of Any Agent		    *															
		*																		*
		*																		*
		*************************************************************************/
		
		Init_List_Of_Agent_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Possible_Slice_List);
		Init_List_Of_Agent_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Possible_Slice_List);
		Init_List_Of_Agent_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,Possible_Slice_List);
		
		Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		
		/************************************************************************
		*																		*
		*				Start Of Solving Multi Score Puzzle						*
		*																		*
		*************************************************************************/
		    MAS_Reward_Episode=0;//In Any Round include Three Try The MAS Reward Initialize With Zero
		    
		  	for(i=0;i<Num_Of_Select;i++) //**//***//
		  	{
		  		MAS_Reward_Train=0;// in any Try  the MAS be initialize with zero for divide into agents
		  	  	if (i==0)	
		  	       {
		  	         Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Bound1,Fault1);
		  	         Cell1=Select_Cell(Possible_Cell_List,Q1,Slice1);
		  	         if(Slice1==Cell1)  NR_Ag1++;   else NP_Ag1++;					   
				     Num_Of_Try_Ag1++;					   					    
						
		  	
		  	  		 Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Bound2,Fault2);
		  	  		 Cell2=Select_Cell(Possible_Cell_List,Q2,Slice2);
		  	  		 if(Slice2==Cell2) NR_Ag2++;   else NP_Ag2++;						 
					 Num_Of_Try_Ag2++;
    					    
    				    
		  	
		  	  		 Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,Bound3,Fault3);
		  	  		 Cell3=Select_Cell(Possible_Cell_List,Q3,Slice3);
		  	  		 if(Slice3==Cell3)	NR_Ag3++;   else NP_Ag3++;					 						 
					 Num_Of_Try_Ag3++;
						   						  
			       }
		  		   
		            cout<<"\n\n\nThe Selected Fisrt Slice By Agent1 IS: "<<Slice1;
		  	        cout<<"\nThe Selected Fisrt Cell By Agent1 IS: "<<Cell1;
			        cout<<"\n-------------------\n"	;
			        cout<<"\nThe Selected Fisrt Slice By Agent2 IS: "<<Slice2;
		  	        cout<<"\nThe Selected Fisrt Cell By Agent2 IS: "<<Cell2;  	
			        cout<<"\n-------------------\n"	;
                    cout<<"\nThe Selected Fisrt Slice By Agent3 IS: "<<Slice3;
		  	        cout<<"\nThe Selected Fisrt Cell By Agent3 IS: "<<Cell3;   	
		  	
		             /************************************************************************
		              *																		 *
		              *																		 *
		              *																		 *
		              ************************************************************************/  	
		  	
		  	        MAS_Reward_Train=Envionment_Feedback(Slice1,Cell1,Slice2,Cell2,Slice3,Cell3);
		  	        MAS_Reward_Episode+=MAS_Reward_Train;
		  	
		  	        cout<<"\n\n\n==========The Reward IS :   "<<MAS_Reward_Train<<"===========\n\n";
		  	        cout<<"\n\n==========The MAS Reward IS :=="<<MAS_Reward_Episode<<"===========\n";
		  	
		  	        if (i==Num_Of_Select-1) 
		               {
		                Next_Slice1=rand()%(Num_Of_Slices-0);  cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
			 			Next_Slice2=rand()%(Num_Of_Slices-0);  cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
			 			Next_Slice3=rand()%(Num_Of_Slices-0);  cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;	
						}
			        else
			           {
		                Next_Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Bound1,Fault1);
		  	            Next_Cell1=Select_Cell(Possible_Cell_List,Q1,Next_Slice1);
		  			  	
		  	            Next_Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Bound2,Fault2);
		  	            Next_Cell2=Select_Cell(Possible_Cell_List,Q2,Next_Slice2);
		  	
		  	            Next_Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,Bound3,Fault3);
		  	            Next_Cell3=Select_Cell(Possible_Cell_List,Q3,Next_Slice3); 
		    		    
		                cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
		  				cout<<"\nThe Selected Next Cell By Agent1 IS: "<<Next_Cell1;
			  			cout<<"\n-------------------\n"	;
			  
						cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
		  				cout<<"\nThe Selected Next Cell By Agent2 IS: "<<Next_Cell2;  	
			  			cout<<"\n-------------------\n"	;
			  
            			cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
		  				cout<<"\nThe Selected Next Cell By Agent3 IS: "<<Next_Cell3;	
						}
		 		 
		             /************************************************************************
		              *																		 *
		              *																		 *
		              *																		 *
		              ************************************************************************/  		 		 
	
		  	           Critic_Equal(MAS_Reward_Train,Reward_List);
		    
		               R1=Reward_List[0];
		               R2=Reward_List[1];
		               R3=Reward_List[2];
		               cout<<"\n\n\n R1=="<<R1<<"  R2=="<<R2<<"  R3=="<<R3;
		    
		               UpDate_QTable(Q1,Slice1,Cell1,R1,Next_Slice1);
	                   UpDate_QTable(Q2,Slice2,Cell2,R2,Next_Slice2);
	                   UpDate_QTable(Q3,Slice3,Cell3,R3,Next_Slice3);
	        	        		  	
		  	           Slice1=Next_Slice1;
		   	           Slice2=Next_Slice2;
		 	           Slice3=Next_Slice3; 
			      
			           Cell1=Next_Cell1;
			           Cell2=Next_Cell2;
			           Cell3=Next_Cell3;
			           
			           if(Slice1==Cell1)    NR_Ag1++;   else NP_Ag1++;					   
				       Num_Of_Try_Ag1++;					   
					    								  						   
			           if(Slice2==Cell2) 	NR_Ag2++;   else NP_Ag2++;		           			           	
				       Num_Of_Try_Ag2++;					   
					    								  						
			           if(Slice3==Cell3)    NR_Ag3++;   else NP_Ag3++;					   
				       Num_Of_Try_Ag3++;					   
					    
							  						   					    
                       cout<<"\n\n--------------------------------------------------------";
			           cout<<"\n-                                                      -";
			           cout<<"\n-                                                      -";
			           cout<<"\n           End   Of  Try   "<<i+1<<"          -";
			           cout<<"\n-                                                      -";
			           cout<<"\n--------------------------------------------------------";	
					   
					   Num_Of_Try++;					  													   
		   }//End For (a Try Is Done)
		               
	 	  	           cout<<"\n\n==========The MAS Reward IS :=="<<MAS_Reward_Episode<<"===========\n";
	                   
			           Print_Q_Table(Q1,1);
	                   Print_Q_Table(Q2,2);
	                   Print_Q_Table(Q3,3);
	                
	                   Round++;
	                   cout<<"\n\t\t===================The "<<Round<<"  th   Round========================";
					   Print_Text("The End Of A Try");
					   //getch();			  				  	
		}//End Train
		
			cout<<"\n\n--------------------------------------------------------";
			cout<<"\n-                                                      -";
			cout<<"\n-                                                      -";
			cout<<"\n           End   Of  Episode   "<<Episode_Counter+1<<"     -";
			cout<<"\n-                                                      -";
			cout<<"\n--------------------------------------------------------";
			
			Print_Text(" The Normalized Q Tables ");
			CopyMatrix(Q1,NormQ1);
			CopyMatrix(Q2,NormQ2);
			CopyMatrix(Q3,NormQ3);
			
	        PrintNormalizedQT(NormQ1,1);
	        PrintNormalizedQT(NormQ2,2);
	        PrintNormalizedQT(NormQ3,3);
	        
	        //getch();
	        	      		        				
	}//End Of Episode
	
		             /************************************************************************
		              *																		 *
		              *		In This Section We Analyze The Result For Next Phase             *
		              *																		 *
		              ************************************************************************/
            Print_Text("Analyze The Result");
			LR1=LearningRate(Q1,1);
	        LR2=LearningRate(Q2,2);
	        LR3=LearningRate(Q3,3);		              
					  
    	
    	cout<<" \n\n\t\t  The Fault Of Agent1 IS: "<<Fault1[0];
    	cout<<" \n\n\t\t  The Fault Of Agent2 IS: "<<Fault2[0];
    	cout<<" \n\n\t\t  The Fault Of Agent3 IS: "<<Fault3[0];	
				 
        Print_Text("The Learning Rate Of Agents");
		cout<<"\n\n\t\t=====Learning Rate Of Agent 1:  "<<double(LR1)/double(Fault1[0]);
		cout<<"\n\n\t\t=====Learning Rate Of Agent 2:  "<<double(LR2)/double(Fault2[0]);
		cout<<"\n\n\t\t=====Learning Rate Of Agent 3:  "<<double(LR3)/double(Fault3[0]);
		
		List_Of_ExpertNess[1][0]=(double(LR1)/double(Fault1[0]));
        List_Of_ExpertNess[1][1]=(double(LR2)/double(Fault2[0]));
        List_Of_ExpertNess[1][2]=(double(LR3)/double(Fault3[0]));
        
        
        Print_Matrix_float(List_Of_ExpertNess);
        
        SortMatrix(List_Of_ExpertNess);
        
        Bound_Index1=List_Of_ExpertNess[0][0];
        Bound_Index2=List_Of_ExpertNess[0][1];
        Bound_Index3=List_Of_ExpertNess[0][2];
        
        /*B1=BoundList[Bound_Index1-1];
        B2=BoundList[Bound_Index2-1];
        B3=BoundList[Bound_Index3-1];*/
        
        B1=Bound2;B2=Bound1;B3=Bound3;
		
		Print_Text("The Sorted Agents");
		cout<<"\n\t\tAgent "<<Bound_Index1<<"    >>\tAgent "<<Bound_Index2<<"   >>\tAgent "<<Bound_Index3;
		cout<<"\n\n\t\t"<<B1<<"\t  "<<B2<<"\t   "<<B3;
		Print_Text("The End Of First Phase");
		
		Print_Text("Sorted Bound");
		cout<<"\n B1= "<<B1<<"  B2= "<<B2<<" B3="<<B3;
				        
       //getch();
       
       
//**************************************************************************************************************************************//
//**************************************************************************************************************************************//
//**************************************************************************************************************************************//
//**************************************************************************************************************************************//

//**************************************************************************************************************************************//
//**************************************************************************************************************************************//
//**************************************************************************************************************************************//
//**************************************************************************************************************************************//	
                     /************************************************************************
		              *																		 *
		              *		In This Section We Start to Impelement The Proposed Method       *
		              *																		 *
		              ************************************************************************	              
	   cout<<"\n \tMAS_Reward_Episode: "<<MAS_Reward_Episode;
	 //  getch();
	   ResetListFloat(Reward_List,Num_Of_Agent);
	   Critic_Type_1(MAS_Reward_Episode,Reward_List);
	   cout<<"\n MAS Reward IS: "<<MAS_Reward_Episode;
	   R1=Reward_List[0];
	   R2=Reward_List[1];
	   R3=Reward_List[2];
	   cout<<"\n\n\n R1=="<<R1<<"  R2=="<<R2<<"  R3=="<<R3;
	  // getch();
	   cout<<"\n  Test   ";
	   //getch();
	   
	   while (LearningRate(Q1,1)<9)
	   	{
	   	  getch() ;
	      Episode_Number_Ag1++;		
	   	  MAS_Reward_Episode=0;
		/************************************************************************
		*	In This Section Any List WIll be Reset These List Include:			*
		*	1.List Of  Slices That Agent should be completed					*
		*	2.Possible List of Slice that Similar to a Slice Box!               *
		*	3.Possible List of Cell that Similar to a Cell Box    !			    *
		*************************************************************************
		while(MAS_Reward_Episode<Reward_Theroshold_Proposed_Phase)
		{
		
		ResetList(List_Of_Slice_Ag1,Num_Of_Slice_Ag1);
		ResetList(List_Of_Slice_Ag2,Num_Of_Slice_Ag2);
		ResetList(List_Of_Slice_Ag3,Num_Of_Slice_Ag3);
		
		Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		
		ResetList(Possible_Slice_List,Num_Of_Slices);
		Print_List(Possible_Slice_List,Num_Of_Slices,"List Of Possible Slices ");
		
		ResetList(Possible_Cell_List,Num_Of_Cells);
		Print_List(Possible_Cell_List,Num_Of_Cells,"List Of Possible Cells ");
		/************************************************************************
		*	In This Section We Initialize The Slice List Of Any Agent		    *															
		*																		*
		*																		*
		*************************************************************************			   	  
	     // while(MAS_Reward_Episode<Reward_Theroshold_Proposed_Phase)
		  //{
		  	
		  	cout<<"\n\n\n R1=="<<R1<<"  R2=="<<R2<<"  R3=="<<R3;
		  //	getch();
		  	for(i=0;i<Num_Of_Select;i++) //**//***
		  	{
		  		MAS_Reward_Train=0;
		  	  	if (i==0)	
		  	       {
		  	       	R1=Reward_List[0];
		  	        R2=Reward_List[1];
		  	        R3=Reward_List[2];
		  	             	if(R1>=TS1)
		  	       	             {
		  	                      Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,B1,Fault1);
		  	                      Cell1=Select_Cell(Possible_Cell_List,Q1,Slice1);
		                          cout<<"\n\n\nThe Selected Fisrt Slice By Agent1 IS: "<<Slice1;
		  	                      cout<<"\nThe Selected Fisrt Cell By Agent1 IS: "<<Cell1;
			                      cout<<"\n-------------------\n"	;											  	       		
					             }
				            if(R2>=TS2)
			                  	 {
				                  Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,B2,Fault2);
		  	  		              Cell2=Select_Cell(Possible_Cell_List,Q2,Slice2);
			                      cout<<"\nThe Selected Fisrt Slice By Agent2 IS: "<<Slice2;
		  	                      cout<<"\nThe Selected Fisrt Cell By Agent2 IS: "<<Cell2;  	
			                      cout<<"\n-------------------\n"	;									  	
				                 }  	
		  	  	            if(R3>=TS3)	 
		  	  	                 {
		  	  		              Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	  		              Cell3=Select_Cell(Possible_Cell_List,Q3,Slice3);
                                  cout<<"\nThe Selected Fisrt Slice By Agent3 IS: "<<Slice3;
		  	                      cout<<"\nThe Selected Fisrt Cell By Agent3 IS: "<<Cell3;  		  	  		              
					             }
		  			  	  		 
			       }//End If(I==0)
		  		   
		  		   if(R1>=TS1)  MAS_Reward_Train+=Envionment_Feedback_Agent1(Slice1,Cell1);
		  		   if(R2>=TS2)  MAS_Reward_Train+=Envionment_Feedback_Agent2(Slice2,Cell2);
		  		   if(R3>=TS3)  MAS_Reward_Train+=Envionment_Feedback_Agent3(Slice3,Cell3);					 	  		   
		  		   MAS_Reward_Episode+=MAS_Reward_Train;

					cout<<"\n\n\n==========The Reward IS :   "<<MAS_Reward_Train<<"===========\n\n";
		  	        cout<<"\n\n==========The MAS Reward IS :=="<<MAS_Reward_Episode<<"===========\n";
  	                
					  if (i==Num_Of_Select-1) 
		               {
		                if(R1>=TS1)  Next_Slice1=rand()%(Num_Of_Slices-0);
			 			if(R2>=TS2)  Next_Slice2=rand()%(Num_Of_Slices-0);
			 			if(R3>=TS3)  Next_Slice3=rand()%(Num_Of_Slices-0); 	
						}
						
						else
			           {
		                if(R1>=TS1) 
						  {
						  	Next_Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Bound1,Fault1);
		  	                Next_Cell1=Select_Cell(Possible_Cell_List,Q1,Next_Slice1);
		  	                cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
		  				    cout<<"\nThe Selected Next Cell By Agent1 IS: "<<Next_Cell1;
			  			    cout<<"\n-------------------\n"	;
						  }
		  			  	if(R2>TS2)
		  			  	{
		  			  		cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
		  				    cout<<"\nThe Selected Next Cell By Agent2 IS: "<<Next_Cell2;  	
			  			    cout<<"\n-------------------\n"	;
		  			  	    Next_Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Bound2,Fault2);
		  	                Next_Cell2=Select_Cell(Possible_Cell_List,Q2,Next_Slice2);	
							}
		  	            
		  				if(R3>=TS3)
		  				{
		  				    Next_Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,Bound3,Fault3);
		  	                Next_Cell3=Select_Cell(Possible_Cell_List,Q3,Next_Slice3); 
                            cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
		  				    cout<<"\nThe Selected Next Cell By Agent3 IS: "<<Next_Cell3;
							cout<<"\n-------------------\n"	;								  	
						  }
		  	           } 
		    		   ResetListFloat(Reward_List,Num_Of_Agent);
	                   Critic_Type_1(MAS_Reward_Episode,Reward_List); 
		               
					   R1=Reward_List[0];
		  	           R2=Reward_List[1];
		  	           R3=Reward_List[2];
		  	           cout<<"\n MAS Reward IS: "<<MAS_Reward_Episode;
		  	           cout<<"\n\n\n R1=="<<R1<<"  R2=="<<R2<<"  R3=="<<R3;
		  	           //getch();
		  	           
					   if(R1>=TS1)  UpDate_QTable(Q1,Slice1,Cell1,R1,Next_Slice1);
	                   if(R2>=TS2)  UpDate_QTable(Q2,Slice2,Cell2,R2,Next_Slice2);
	                   if(R3>=TS3)  UpDate_QTable(Q3,Slice3,Cell3,R3,Next_Slice3);
	                   
	                  if (R1>=TS1)
					   {
	                  	Slice1=Next_Slice1;
	                    Cell1=Next_Cell1;
					   }  
	                  if (R2>=TS2)
	                  {
	                  	Slice2=Next_Slice2;
		   	            Cell2=Next_Cell2;
					  }
		   	          if(R3>=TS3)
		   	          {
		   	          	Slice3=Next_Slice3; 	      			         			          
			            Cell3=Next_Cell3;
						 }
		   	          		 	        	                                   		  	           		  	           					    			  								  
      			}//END FOR
      }		  	
		             /************************************************************************
		              *																		 *
		              *																		 *
		              *																		 *
		              ************************************************************************  	
		//  }//END WHILE TRAIN
		            cout<<" \n The Episode Number :  "<<Episode_Number_Ag1;
		            
		            Print_Q_Table(Q1,1);
	                Print_Q_Table(Q2,2);
	                Print_Q_Table(Q3,3); 
	                
	                LR1=LearningRate(Q1,1);
	                LR2=LearningRate(Q2,2);
	                LR3=LearningRate(Q3,3);		 		
	               //getch();	                		  	        
		}//END WHILE EPISODE*/
	/*	Critic_Equal(MAS_Reward_Episode,Reward_List);
		cout<<"\n\n==========The MAS Reward IS :=="<<MAS_Reward_Episode<<"===========\n";
		R1=Reward_List[0];
		R2=Reward_List[1];
		R3=Reward_List[2];
		cout<<"\n\n\n R1=="<<R1<<"  R2=="<<R2<<"  R3=="<<R3;  
	               	 /************************************************************************
		              *																		 *
		              *																		 *
		              *																		 *
		              ************************************************************************/  		
		//Episode_Number_Ag1=0;		
//********************************************************************************************************************************************//		
	               	 /************************************************************************
		              *																		 *
		              *				        Start Of Proposed Method	     				 *
		              *																		 *
		              ************************************************************************/  
					  cout<<"\n Learning Rate Ag1 IS:"<<LearningRate(Q1,1);
					  cout<<"\n Learning Rate Ag2 IS:"<<LearningRate(Q2,1);
					  cout<<"\n Learning Rate Ag3 IS:"<<LearningRate(Q3,1);
					  
					  ALR1=(double(LearningRate(Q1,1)))/(double(9));
		              ALR2=(double(LearningRate(Q2,2)))/(double(9));
		              ALR3=(double(LearningRate(Q3,3)))/(double(9));		  
		              GLR=(double(ALR1+ALR2+ALR3))/double(3);
		              
		              
		              
		              		Cnf1=Confidence(Q1,1);
		  					Cnf2=Confidence(Q2,2);
		  					Cnf3=Confidence(Q3,3);
		  					MAS_Cnf=(Cnf1+Cnf2+Cnf3)/3;
		  					
		  					Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1));
		  					Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2));
		  					Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3));
		  					
                            Print_Text("Learnig Rate Of Agents And MAS IS:");
		                    cout<<"\nLearning Ratio Of Agent No 1  IS:"<<ALR1;
		                    cout<<"\nLearning Ratio Of Agent No 2  IS:"<<ALR2;
		                    cout<<"\nLearning Ratio Of Agent No 3  IS:"<<ALR3;		  
		                    cout<<"\n Learning Ratio of MAS IS: "<<GLR;		  					
		  
		  					Print_Text("The Confidence of Agents IS:");
		  					cout<<"\n The Confidence Of Agent 1  IS: "<<Cnf1;
		  					cout<<"\n The Confidence Of Agent 2  IS: "<<Cnf2;
		  					cout<<"\n The Confidence Of Agent 3  IS: "<<Cnf3;
		  					cout<<"\n The Confidence Of MAS  IS: "<<MAS_Cnf;
		  					
		  					cout<<"\n---------The Expertness Of Agents :-------------\n";
		  					cout<<"\n The Expertness of Agent 1  IS:"<<Exp_Ag1;
		  					cout<<"\n The Expertness of Agent 2  IS:"<<Exp_Ag2;
		  					cout<<"\n The Expertness of Agent 3  IS:"<<Exp_Ag3;
		  					
		  					
		  					
					  
					  //getch();
					  Num_Of_Try=0;
					  
		Round=0;
		
		Init_Q_Table(Q1);
		Init_Q_Table(Q2);
		Init_Q_Table(Q3);
//********************************************************************************************************************************************//				
	               	        /************************************************************************
		                     *																	    *
		                     *				        Start Of Agent 1 Learning	     			    *
		                     *																		*
		                     ************************************************************************/  		
//********************************************************************************************************************************************//				              
		       while ((LearningRate(Q1,1)<9)&& (Episode_Number_Ag1<=Iteration)) //Start Of Episode in Test Phase		         
	          //        for( i=1;i<=100;i++)
		             {
		              
					  Print_Q_Table(Q1,1);
		              Print_Q_Table(Q2,2);
		              Print_Q_Table(Q3,3);
		              
		              cout<<"\n Learning Rate Agent 1 IS: "	<<LearningRate(Q1,1);
		              cout<<"\n Learning Rate Agent 2 IS: "	<<LearningRate(Q2,1);
		              cout<<"\n Learning Rate Agent 3 IS: "	<<LearningRate(Q3,1);
		              
                      Episode_Number_Ag1++;
		    
		              MAS_Reward_Episode=0;
		              NR_Ag1=0;  NR_Ag2=0; NR_Ag3=0;
					  NP_Ag1=0;  NP_Ag2=0; NP_Ag3=0;
					  
					  Num_Of_Try_Ag1=0;
					  Num_Of_Try_Ag2=0;
					  Num_Of_Try_Ag3=0; 
					  
					  Sum_Of_Cert=0;
					  Cert_Counter=0;
					  
					  CorrAg1=0;
					  CorrAg2=0;
					  CorrAg3=0;//in any episode it reset to zero
					  
					  NumOfRewarding=0;
		                while(MAS_Reward_Episode<Reward_Theroshold_Proposed_Phase)
		                   {
						   		        
	               	        /************************************************************************
		                     *																	    *
		                     *			    In This Section MAS Start To Done It's Task             *
							 *               In This sction Start Round                             *
							 *               and any Round Includes 3 Try                           *
							 *             If Reward Of MAS Arrive To   Theroshold(40)              *
							 *                  Episode  Finished	     	                        *
		                     *																		*
		                     ************************************************************************/  						   		        
		  	                 ResetList(List_Of_Slice_Ag1,Num_Of_Slice_Ag1);
		                     ResetList(List_Of_Slice_Ag2,Num_Of_Slice_Ag2);
		                     ResetList(List_Of_Slice_Ag3,Num_Of_Slice_Ag3);
		      
		                     Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		                     Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		                     Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		      
		                     ResetList(Possible_Slice_List,Num_Of_Slices);
		                     Print_List(Possible_Slice_List,Num_Of_Slices,"List Of Possible Slices ");
		      
		                     ResetList(Possible_Cell_List,Num_Of_Cells);
		                     Print_List(Possible_Cell_List,Num_Of_Cells,"List Of Possible Cells ");
		      
		                     Init_List_Of_Agent_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Possible_Slice_List);
		                     Init_List_Of_Agent_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Possible_Slice_List);
		                     Init_List_Of_Agent_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,Possible_Slice_List);
		
		                     Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		                     Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		                     Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		      
		                     MAS_Reward_Episode=0;	// The Reward MAS Receive in any Round that inludes 3 Try
		                     MAS_Reward_Test=0;     // The Reward That MAS Receive in a Try
		      
		                     Print_Text("Start The Round (Try)");		                     
		                     cout<<"\n\n=============================================================================";
		                     cout<<"\n=============================================================================";
		      
		                     for(i=0;i<Num_Of_Select;i++)// Start of Any Round	(Any Round Includes 3 Tries)
		                        {
		                        	
		      	                 Num_Of_Try++;		      	                 
		      	                 MAS_Reward_Test=0;// In The Start Of Ant Try The MAS Reward Set to 0
		      	                 
		      	                 Sw_Ag1_Select=0;
		      	                 Sw_Ag2_Select=0;
		      	                 Sw_Ag3_Select=0;
		      	                 
		      	                 RealRewardAg1=0;
		      	                 RealRewardAg2=0;
		      	                 RealRewardAg3=0;
		      	                 
		      	                 if (i==0)
		      	                    {
	               	        /************************************************************************
		                     *																	    *
		                     *	        In The Try 1 Any Agent Try to Solving It's Task	            *    
							 *           and If MAS Receive Reward  Critic Distributes              *
							 *             It Between Agent's. If The Reward of Any                 *
							 *           Agent Arrive to it's TS It Works in next Trying            *
		                     *																		*
		                     ************************************************************************/  		      	                    			      	                    	
		      	                     //	if(R1>=TS1)		      	      
		      	                       {
		      	                        Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,B1,Fault1);
		  	                            Cell1=Select_Cell(Possible_Cell_List,Q1,Slice1);
		                                cout<<"\n\n\nThe Selected Fisrt Slice By Agent1 IS: "<<Slice1;
		  	                            cout<<"\nThe Selected Fisrt Cell By Agent1 IS: "<<Cell1;
			                            cout<<"\n-------------------------------------------------------------\n";		      
		                                //MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2);
				                        //MAS_Reward_Episode+=MAS_Reward_Test;	
				                        
				                        
				        
				                        //Expert Parameter
				                        //if(Slice1==Cell1) NR_Ag1++;else NP_Ag1++;
				                        Sw_Ag1_Select=1;
				                        Exp_Try1++;
				                        
				                        Num_Of_Try_Ag1++;  //Agent 1 Try To Solve
				                        //Expert Parameter
				                        
				                        //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q1,Episode_Number_Ag1,Slice1,Cell1);
				                        Cert_Counter++;
				                        //Certain Parameter
				                       }
	               	        /************************************************************************
		                     *																	    *
		                     *		       In The Below We Test if any Agent Learnt                 *
							 *        The Reward Of MAS Will Be Increase via The Algorithm          *
		                     *																		*
		                     ************************************************************************/  
		                                if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9))
									    {
									   	   if(Slice1==Cell1)  RealRewardAg1=20;
									   	   else RealRewardAg1=0;
										}   
										
										if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9)) 
										{
										   if(Slice1==Cell1)  RealRewardAg1=30;
									   	   else RealRewardAg1=0;	
									    }
									    
										if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9))	
										{
										   if(Slice1==Cell1)  RealRewardAg1=25;	
										   else  RealRewardAg1=0;
										}
																						   
										if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9))
										{
										if(Slice1==Cell1)  RealRewardAg1=35;	
										 else  RealRewardAg1=0;	
									    }												   												   
													                  
							           cout<<"\n Real Reward Of Agent1 IS: "<<RealRewardAg1;
							 				       
				                        if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9)) MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);
				                        if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9)) MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1)+10;
				                        if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9)) MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1)+5;
				                        if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9)) MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1)+15;
										
														       
           	                            //    if(R2>=TS2)
					                    if(LearningRate(Q2,2)<9)
		         	                       {
		      	                            Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,B2,Fault2);
		  	                                Cell2=Select_Cell(Possible_Cell_List,Q2,Slice2);
		                                    cout<<"\n\n\nThe Selected Fisrt Slice By Agent2 IS: "<<Slice2;
		  	                                cout<<"\nThe Selected Fisrt Cell By Agent2 IS: "<<Cell2;
			                                cout<<"\n---------------------------------------------------------------\n";
			                                
			                                if((LearningRate(Q3,3)<9))
			                                {
			                                	if(Slice2==Cell2)  RealRewardAg2=15;
			                                	else RealRewardAg2=0;
											}
											if((LearningRate(Q3,3)>=9))
											{
												if(Slice2==Cell2)  RealRewardAg2=20;
			                                	else RealRewardAg2=0;
											}
												
cout<<"\n Real Reward Of Agent 2 IS: "<<RealRewardAg2;													      
		                                    if((LearningRate(Q3,3)<9))  MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2);
		                                    else MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2)+5;
				                            //MAS_Reward_Episode+=MAS_Reward_Test;
					  
					                        //Expert Parameter
				                              //if(Slice2==Cell2) NR_Ag2++;else NP_Ag2++;
				                        Sw_Ag2_Select=1;
				                        Exp_Try2++;
														                              
				                              Num_Of_Try_Ag2++;
				                            //Expert Parameter	
				                            
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q2,Episode_Number_Ag1,Slice2,Cell2);
				                        Cert_Counter++;	
										   //Certain Parameter			                            
				                           }				      					     				     				     				 								  
				                       //    if(R3>=TS3)
				                        if(LearningRate(Q3,3)<9)
		      	                           {
		      	                            Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	                                Cell3=Select_Cell(Possible_Cell_List,Q3,Slice3);
		                                    cout<<"\n\n\nThe Selected Fisrt Slice By Agent3 IS: "<<Slice3;
		  	                                cout<<"\nThe Selected Fisrt Cell By Agent3 IS: "<<Cell3;
			                                cout<<"\n---------------------------------------------------------------\n";
			                                
			                                if((LearningRate(Q2,2)<9)) 
			                                {
			                                    if(Slice3==Cell3)  RealRewardAg3=10;
			                                	else RealRewardAg3=0;	
											}
											
											if((LearningRate(Q2,2)>=9)) 
			                                {
			                                    if(Slice3==Cell3)  RealRewardAg3=20;
			                                	else RealRewardAg3=0;	
											}
												
											cout<<"\n Real Reward Of Agent 3  IS: "<<RealRewardAg3;	
													      
		                                    if((LearningRate(Q2,2)<9))  MAS_Reward_Test+=Envionment_Feedback_Agent3(Slice3,Cell3);
		                                    else MAS_Reward_Test+=Envionment_Feedback_Agent3(Slice3,Cell3)+10;		              
		                               //Expertness Parameter		              
		                               //Expertness Parameter
				                       // MAS_Reward_Episode+=MAS_Reward_Test;					 
					                   //Expert Parameter
				                           // if(Slice3==Cell3) NR_Ag3++;else NP_Ag3++;
				                        Sw_Ag3_Select=1;
				                        Exp_Try3++;				                           
				                            Num_Of_Try_Ag3++;
				                       //Expert Parameter
									   
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q3,Episode_Number_Ag1,Slice3,Cell3);
				                        Cert_Counter++;	
										   //Certain Parameter										   	
				                            } 	
					                            Print_Text("The Next Selection");
					                            Next_Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,B1,Fault1);
		  	               						Next_Cell1=Select_Cell(Possible_Cell_List,Q1,Next_Slice1);	
                                                cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
		  				   						cout<<"\nThe Selected Next Cell By Agent1 IS: "<<Next_Cell1;
			  			   						cout<<"\n----------------------------------------------------------\n";		  	               
							  					
												if(LearningRate(Q2,2)<9)
												{
												      
												Next_Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,B2,Fault2);
		  	               						Next_Cell2=Select_Cell(Possible_Cell_List,Q2,Next_Slice2);	
                           						cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
		  				   						cout<<"\nThe Selected Next Cell By Agent2 IS: "<<Next_Cell2;  	
			  			   						cout<<"\n----------------------------------------------------------\n"; 
			  			   					    }
					 	      		
					 	      		            if(LearningRate(Q3,3<9))
					 	      		            {
					 	      		             Next_Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	                					 Next_Cell3=Select_Cell(Possible_Cell_List,Q3,Next_Slice3); 			  			  		
	                        					 cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
		  				    					 cout<<"\nThe Selected Next Cell By Agent3 IS: "<<Next_Cell3;
		  				    					 cout<<"\n----------------------------------------------------------\n";
					 	      		            	
												   }		      								
		  			
		  			                              /*MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);
				         							MAS_Reward_Episode+=MAS_Reward_Test;
				         
				    								MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2);
				         							MAS_Reward_Episode+=MAS_Reward_Test;
					
													MAS_Reward_Test+=Envionment_Feedback_Agent3(Slice3,Cell3);
				         							MAS_Reward_Episode+=MAS_Reward_Test;*/	     
					  						/************************************************************************
		              						 *																		*
		                                     *			      The Heart Of The Program								*
		                                     *																		*
		                                     ************************************************************************/    
					                        ResetListFloat(Reward_List,Num_Of_Agent);
					                        /************Neither Agent 2 and Agent 3 Learnt***********/
					    
					                        if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9))
					                        {
					                        switch(Critic_Type)
					                         {					    
					    	                  case 1:Critic_Type_1(MAS_Reward_Test,Reward_List);
					    	       					 cout<<"Slelect  1";
					    	                         break;
					        				  case 2:Critic_DB(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  2";
					                                 break;
					                          case 3:Critic_Type_TS_Agents_priority(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  3";
					                                 break;
					                          case 4:Critic_Type_TS_Expert_Agents_priority(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  4";
							                         break;                 
	                                         }	
											}
											/************ Agent 2 Learnt***********/
											if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9))
					                        {
					                        switch(Critic_Type)
					                         {					    
					    	                  case 1:Critic_Type_1_Without_Ag2(MAS_Reward_Test,Reward_List);
					    	       					 cout<<"Slelect  1";
					    	                         break;
					        				  case 2:Critic_DB_Without_Ag2(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  2";
					                                 break;
					                          case 3:Critic_Type_TS_Agents_priority_Without_Ag2(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  3";
					                                 break;
					                          case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag2(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  4";
							                         break;                 
	                                         }	
											}
											/************ Agent 3 Learnt***********/
											if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9))
					                        {
					                        switch(Critic_Type)
					                         {					    
					    	                  case 1:Critic_Type_1_Without_Ag3(MAS_Reward_Test,Reward_List);
					    	       					 cout<<"Slelect  1";
					    	                         break;
					        				  case 2:Critic_DB_Without_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  2";
					                                 break;
					                          case 3:Critic_Type_TS_Agents_priority_Without_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  3";
					                                 break;
					                          case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  4";
							                         break;                 
	                                         }	
											}
											/************Either Agent 2 and Agent 3 Learnt***********/
											if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9))
					                        {
					                        switch(Critic_Type)
					                         {					    
					    	                  case 1:Critic_Type_1_Without_Ag2_Ag3(MAS_Reward_Test,Reward_List);
					    	       					 cout<<"Slelect  1";
					    	                         break;
					        				  case 2:Critic_DB_Without_Ag2_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  2";
					                                 break;
					                          case 3:Critic_Type_TS_Agents_priority_Without_Ag2_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  3";
					                                 break;
					                          case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag2_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  4";
							                         break;                 
	                                         }	
											}
											/***********************************************************/
					                        
	                    
					                       cout<<"\n Step(TRY) Reward IS: "<<MAS_Reward_Test<<"\n";
					                       R1=Reward_List[0];
		  	                               R2=Reward_List[1];
		  	                               R3=Reward_List[2];
		  	                               
		  	                               NumOfRewarding++;
		  	            		  	          
						                   cout<<"\n R1 In First Step IS: "<<R1<<" \t TS1 IS:"<<TS1;
		  	                               cout<<"\n R2 In First Step IS: "<<R2<<" \t TS2 IS:"<<TS2;
		  	                               cout<<"\n R3 In First Step IS: "<<R3<<" \t TS3 IS:"<<TS3;
		  	                               // cout<<"\n --------------------End The Step: "<<i<<"---------------------\n";
										
										//****************ExpertNess****************//	
										   if (Sw_Ag1_Select==1) 
										   {
										   	if (R1>0)  NR_Ag1++; else NP_Ag1++;
										   }
										   
										   if (LearningRate(Q2,2)<9)	
										          {
										   	       if(Sw_Ag2_Select==1)
													  {
													  	if(R2>0)    NR_Ag2;else NP_Ag2++;
													  }
										          }
										          
										   if (LearningRate(Q3,3)<9)	
										          {
										   	       if (Sw_Ag3_Select==1) 
													  {
													  	if (R3>0)  NR_Ag3;else NP_Ag3++;
													  }
													  
										          }			  	            
										//****************ExpertNess****************//		  	
											
										// Efficeincy	 
										   if(MAS_Reward_Test>0)  Non_Zero_Reward++;
										   if(R1>0)	Non_Zero_R1++;
										   if(R2>0)	Non_Zero_R2++;										   				 
										   if(R3>0)	Non_Zero_R3++;
										// Efficiency  
										
										//*******************Correctness************//
										cout<<"\nR1 IS:"<<R1;
										cout<<"\nR2 IS:"<<R2;
										cout<<"\nR3 IS:"<<R3;
										
										cout<<"\n The Real Reward Of Agent1 IS: "<<RealRewardAg1;
										cout<<"\n The Real Reward Of Agent2 IS: "<<RealRewardAg2;
										cout<<"\n The Real Reward Of Agent3 IS: "<<RealRewardAg3;										
										
										if(abs(R1-RealRewardAg1)<=CorrThreshold )  CorrAg1++;else CorrAg1--;
										if(abs(R2-RealRewardAg2)<=CorrThreshold )  CorrAg2++;else CorrAg2--;
										if(abs(R3-RealRewardAg3)<=CorrThreshold )  CorrAg3++;else CorrAg3--;
										
										  cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
										  cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;	
										  
									/*	  if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9)	)								  		  									
		  					                                                     MASCorr=double(CorrAg1+CorrAg2+CorrAg3)/double(3);
		  					                                                     
		  							  	  if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9)	)
		  							  	  										 MASCorr=double(CorrAg1+CorrAg3)/double(2);
		  							  	  										 
										  if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9)	)
		  							  	  										 MASCorr=double(CorrAg1+CorrAg2)/double(2);	  
																					   
										  if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9)	)
		  							  	  										 MASCorr=double(CorrAg1)/double(1);	*/										   														
										//*******************Correctness************// 
					     //getch();
					  /************************************************************************
		               *																 	  *
		               *																	  *
		               *																	  *
		               ************************************************************************/ 
					                  if(R1>=TS1)			                 UpDate_QTable(Q1,Slice1,Cell1,R1,Next_Slice1);
	                                  if((LearningRate(Q2,2)<9)&&(R2>=TS2))  UpDate_QTable(Q2,Slice2,Cell2,R2,Next_Slice2);
	                    			  if((LearningRate(Q3,3)<9)&&(R3>=TS3))  UpDate_QTable(Q3,Slice3,Cell3,R3,Next_Slice3);
										    					    
                       				 cout<<"\n\n--------------------------------------------------------";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n           End   Of  Try   "<<i+1<<"          -";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n--------------------------------------------------------";									  	   						  						 	    
				                     }
	               	        /************************************************************************
		                     *																	    *
		                     *				        End Of First Try In Round	     			    *
		                     *																		*
		                     ************************************************************************/  				                     
				               else
				                    {
                            /*************************************************************************
		                     *																	     *
		                     *			      The Second and Third Try In Round 					 *
		                     *																		 *
		                     *************************************************************************/  
		                             if (R1>=TS1)
					                    {
	                  	                 Slice1=Next_Slice1;
	                                     Cell1=Next_Cell1;
		                                 cout<<"\n\n\nThe  "<<i+1<<" th Selected  Slice By Agent1 IS: "<<Slice1;
		  	                             cout<<"\nThe  "<<i+1<<"  th Selected  Cell By Agent1 IS: "<<Cell1;	 
										   
		                                if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9))
									    {
									   	   if(Slice1==Cell1)  RealRewardAg1=20;
									   	   else RealRewardAg1=0;
										}   
										
										if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9)) 
										{
										   if(Slice1==Cell1)  RealRewardAg1=30;
									   	   else RealRewardAg1=0;	
									    }
									    
										if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9))	
										{
										   if(Slice1==Cell1)  RealRewardAg1=25;	
										   else  RealRewardAg1=0;
										}
																						   
										if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9))
										{
										if(Slice1==Cell1)  RealRewardAg1=35;	
										 else  RealRewardAg1=0;	
									    }																			   
										    
cout<<"\n Real Reward Of Agent1 IS: "<<RealRewardAg1;											                                  
	                                     //Expert Parameter
				                         //if(Slice1==Cell1) NR_Ag1++;else NP_Ag1++;
				                        Sw_Ag1_Select=1;
				                        Exp_Try1++;
														                         
				                         Num_Of_Try_Ag1++;
				                         //Expert Parameter			                         				                         				                         									   
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q1,Episode_Number_Ag1,Slice1,Cell1);
				                        Cert_Counter++;	
										   //Certain Parameter
										   
										   					                         
					                    }  
					                  else cout<<"\n\t Not Selection With The Agent 1"  ;
					                  
	                                 if ((R2>=TS2)&&(LearningRate(Q2,2)<9))
	                                    {
	                  	                 Slice2=Next_Slice2;
		   	                             Cell2=Next_Cell2;
		                                 cout<<"\n\n\nThe  "<<i+1<<" th Selected  Slice By Agent2 IS: "<<Slice2;
		  	                             cout<<"\nThe  "<<i+1<<"  th Selected Cell By Agent2 IS: "<<Cell2;	
										   
			                                if((LearningRate(Q3,3)<9))
			                                {
			                                	if(Slice2==Cell2)  RealRewardAg2=15;
			                                	else RealRewardAg2=0;
											}
											if((LearningRate(Q3,3)>=9))
											{
												if(Slice2==Cell2)  RealRewardAg2=20;
			                                	else RealRewardAg2=0;
											}	
											
cout<<"\n Real Reward Of Agent 2 IS: "<<RealRewardAg2;																				   	   	                             
		   	                            //Expert Parameter
				                         //if(Slice2==Cell2) NR_Ag2++;else NP_Ag2++;
				                        Sw_Ag2_Select=1;
				                        Exp_Try2++;
														                         
				                         Num_Of_Try_Ag2++;
				                       //Expert Parameter
				                       
				                         				                         									   
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q2,Episode_Number_Ag1,Slice2,Cell2);
				                        Cert_Counter++;	
										   //Certain Parameter					                         
					                     }
					                  else cout<<"\n\t Not Selection With The Agent 2"  ; 
									    
		   	                         if((R3>=TS3)&&(LearningRate(Q3,3)<9))
		   	                            {
		   	          	                 Slice3=Next_Slice3; 	      			         			          
			                             Cell3=Next_Cell3;
		                                 cout<<"\n\n\nThe  "<<i+1<<" th Selected  Slice By Agent3 IS: "<<Slice3;
		  	                             cout<<"\nThe  "<<i+1<<"  th Selected Cell By Agent3 IS: "<<Cell3;
										   
			                                if((LearningRate(Q2,2)<9)) 
			                                {
			                                    if(Slice3==Cell3)  RealRewardAg3=10;
			                                	else RealRewardAg3=0;	
											}
											
											if((LearningRate(Q2,2)>=9)) 
			                                {
			                                    if(Slice3==Cell3)  RealRewardAg3=20;
			                                	else RealRewardAg3=0;	
											}	
											
cout<<"\n Real Reward Of Agent 3 IS: "<<RealRewardAg3;																				   			                             
			                             //Expert Parameter
				                        // if(Slice3==Cell3) NR_Ag3++;else NP_Ag3++;
				                        Sw_Ag3_Select=1;
				                        Exp_Try3++;
														                        
				                         Num_Of_Try_Ag3++;
				                         //Expert Parameter
				                         
				                         
				                         
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q3,Episode_Number_Ag1,Slice3,Cell3);
				                        Cert_Counter++;	
										   //Certain Parameter					                         
						                }
									  else cout<<"\n\t Not Selection With The Agent 3"  ;			
		              		              		              
					                 if(R1>=TS1)
						                {
						                 //MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);						 
					                     if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9)) MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);
				                         if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9)) MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1)+10;
				                         if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9)) MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1)+5;
				                         if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9)) MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1)+15;						 
				                         // MAS_Reward_Episode+=MAS_Reward_Test;	
						                } 
						             if(R2>=TS2)  
										{
						 				 //MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2);
						 				 if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9))  MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2);
						 				 if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9))  MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2)+5;		                 
				                         // MAS_Reward_Episode+=MAS_Reward_Test;	
						                }
				                     if(R3>=TS3)
				                        {				                         
				                         if((LearningRate(Q3,3)<9)&&(LearningRate(Q2,2)<9))  MAS_Reward_Test+=Envionment_Feedback_Agent3(Slice3,Cell3);
						                 if((LearningRate(Q3,3)<9)&&(LearningRate(Q2,2)>=9))  MAS_Reward_Test+=Envionment_Feedback_Agent3(Slice3,Cell3)+10;
				                         // MAS_Reward_Episode+=MAS_Reward_Test;	
						                } 
					    
					                 ResetListFloat(Reward_List,Num_Of_Agent);
					                 	                                 
					                        /************Neither Agent 2 and Agent 3 Learnt***********/
					    
					                        if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9))
					                        {
					                        switch(Critic_Type)
					                         {					    
					    	                  case 1:Critic_Type_1(MAS_Reward_Test,Reward_List);
					    	       					 cout<<"Slelect  1";
					    	                         break;
					        				  case 2:Critic_DB(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  2";
					                                 break;
					                          case 3:Critic_Type_TS_Agents_priority(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  3";
					                                 break;
					                          case 4:Critic_Type_TS_Expert_Agents_priority(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  4";
							                         break;                 
	                                         }	
											}
											/************ Agent 2 Learnt***********/
											if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9))
					                        {
					                        switch(Critic_Type)
					                         {					    
					    	                  case 1:Critic_Type_1_Without_Ag2(MAS_Reward_Test,Reward_List);
					    	       					 cout<<"Slelect  1";
					    	                         break;
					        				  case 2:Critic_DB_Without_Ag2(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  2";
					                                 break;
					                          case 3:Critic_Type_TS_Agents_priority_Without_Ag2(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  3";
					                                 break;
					                          case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag2(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  4";
							                         break;                 
	                                         }	
											}
											/************ Agent 3 Learnt***********/
											if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9))
					                        {
					                        switch(Critic_Type)
					                         {					    
					    	                  case 1:Critic_Type_1_Without_Ag3(MAS_Reward_Test,Reward_List);
					    	       					 cout<<"Slelect  1";
					    	                         break;
					        				  case 2:Critic_DB_Without_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  2";
					                                 break;
					                          case 3:Critic_Type_TS_Agents_priority_Without_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  3";
					                                 break;
					                          case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  4";
							                         break;                 
	                                         }	
											}
											/************Either Agent 2 and Agent 3 Learnt***********/
											if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9))
					                        {
					                        switch(Critic_Type)
					                         {					    
					    	                  case 1:Critic_Type_1_Without_Ag2_Ag3(MAS_Reward_Test,Reward_List);
					    	       					 cout<<"Slelect  1";
					    	                         break;
					        				  case 2:Critic_DB_Without_Ag2_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  2";
					                                 break;
					                          case 3:Critic_Type_TS_Agents_priority_Without_Ag2_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  3";
					                                 break;
					                          case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag2_Ag3(MAS_Reward_Test,Reward_List);
					                                 cout<<"Slelect  4";
							                         break;                 
	                                         }	
											}
											/***********************************************************/	                                 
		                    			cout<<"\n\t Learning Rate of Agent 1 IS:"<<LearningRate(Q1,1);
		                    			cout<<"\n\t Learning Rate of Agent 2 Is:"<<LearningRate(Q2,2);
		                    			cout<<"\n\t Learning Rate of Agent 3 IS:"<<LearningRate(Q3,3);					    			 		                      	                    					     					     					     
					                 cout<<"\n\n\t Reward In Step(Try) IS: "<<MAS_Reward_Test;
					    			 R1=Reward_List[0];
		  	            			 R2=Reward_List[1];
		  	            			 R3=Reward_List[2];
		  	            			 
		  	            			 NumOfRewarding++;
		  	            			 		  	          
						             cout<<"\n\n\t\t R1  IS: "<<R1<<" \t TS1 IS:"<<TS1;
		  	            			 cout<<"\n\t\t R2  IS: "<<R2<<" \t TS2 IS:"<<TS2;
		  	            			 cout<<"\n\t\t R3  IS: "<<R3<<" \t TS3 IS:"<<TS3;
		  	            			 
		  	            			 //****************ExpertNess****************//
										   if (Sw_Ag1_Select==1) 
										   {
										   	if (R1>0)  NR_Ag1++; else NP_Ag1++;
										   }
										   
										   if (LearningRate(Q2,2)<9)	
										          {
										   	       if(Sw_Ag2_Select==1)
													  {
													  	if(R2>0)    NR_Ag2;else NP_Ag2++;
													  }
										          }
										          
										   if (LearningRate(Q3,3)<9)	
										          {
										   	       if (Sw_Ag3_Select==1) 
													  {
													  	if (R3>0)  NR_Ag3;else NP_Ag3++;
													  }
													  
										          }			  	            			 
		  	            			 //****************ExpertNess****************//
		  	            			 
										// Efficeincy	 
										   if(MAS_Reward_Test>0)  Non_Zero_Reward++;
										   if(R1>0)	Non_Zero_R1++;
										   if(R2>0)	Non_Zero_R2++;										   				 
										   if(R3>0)	Non_Zero_R3++;
										// Efficiency 
										
										
										//*******************Correctness************//
										cout<<"\nR1 IS:"<<R1;
										cout<<"\nR2 IS:"<<R2;
										cout<<"\nR3 IS:"<<R3;
										
										cout<<"\n The Real Reward Of Agent1 IS: "<<RealRewardAg1;
										cout<<"\n The Real Reward Of Agent2 IS: "<<RealRewardAg2;
										cout<<"\n The Real Reward Of Agent3 IS: "<<RealRewardAg3;
										
										if(abs(R1-RealRewardAg1)<=CorrThreshold )  CorrAg1++;else CorrAg1--;
										if(abs(R2-RealRewardAg2)<=CorrThreshold  ) CorrAg2++;else CorrAg2--;
										if(abs(R3-RealRewardAg3)<=CorrThreshold  ) CorrAg3++;else CorrAg3--;
										
										  cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
										  cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;	
										  

		  							  															
										//*******************Correctness************// 										
										  		  	            			 
		  	            //getch();
		  	                         if(R1>=TS1)                            UpDate_QTable(Q1,Slice1,Cell1,R1,Next_Slice1);
	                                 if((R2>=TS2)&&(LearningRate(Q2,2)<9))  UpDate_QTable(Q2,Slice2,Cell2,R2,Next_Slice2);
	                                 if((R3>=TS3)&&(LearningRate(Q3,3)<9))  UpDate_QTable(Q3,Slice3,Cell3,R3,Next_Slice3);	
					 
					                 //getch();
					  /************************************************************************
		               *																 	  *
		               *																	  *
		               *																	  *
		               ************************************************************************/   				  					  	
				  					 if (i==Num_Of_Select-1) 
		               					{
		                				 Next_Slice1=rand()%(Num_Of_Slices-0);   cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
			 							 Next_Slice2=rand()%(Num_Of_Slices-0);   cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
			 							 Next_Slice3=rand()%(Num_Of_Slices-0); 	 cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
										}
									 else
			           					{
			           					  if(R1>=TS1)
			           					   {
			           	   				    Next_Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,B1,Fault1);
		  	               				    Next_Cell1=Select_Cell(Possible_Cell_List,Q1,Next_Slice1);	
                           				    cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
		  				   				    cout<<"\nThe Selected Next Cell By Agent1 IS: "<<Next_Cell1;
			  			   				    cout<<"\n-------------------\n"	;		  	               
						   				   }
		                                  if((R2>=TS2)&&(LearningRate(Q2,2)<9))
		                				   {
		                   					Next_Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,B2,Fault2);
		  	               					Next_Cell2=Select_Cell(Possible_Cell_List,Q2,Next_Slice2);	
                           					cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
		  				   					cout<<"\nThe Selected Next Cell By Agent2 IS: "<<Next_Cell2;  	
			  			   					cout<<"\n-------------------\n"	;		  	               
											}
		  			  					  if((R3>=TS3)&&(LearningRate(Q3,3)<9))
		  			  					   {
		  			  	    				Next_Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	                				Next_Cell3=Select_Cell(Possible_Cell_List,Q3,Next_Slice3); 			  			  		
	                        				cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
		  				    				cout<<"\nThe Selected Next Cell By Agent3 IS: "<<Next_Cell3;
											cout<<"\n-------------------\n"	;  	  	            
											}
											
											//getch();
		  	            		  		  	            		    		    		                	  									              				
				 	   					}	
					 	   					
				  
		      	                        // cout<<"\n --------------------End The Step: "<<i+1<<"---------------------\n";				        				        
		  	                         }
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		        End Of The Second and Third Try						  *
		                                  *																	  *
		                                  *********************************************************************/ 
										    					    
                       				 cout<<"\n\n--------------------------------------------------------";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n           End   Of  Try   "<<i+1<<"          -";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n--------------------------------------------------------";						    
					
					   				 MAS_Reward_Episode+=MAS_Reward_Test;// The Reward of Any Try Summed
					   				 cout<<"\n\t The MAS Reward In Try "<<i+1<<" Is:  "<<MAS_Reward_Test;
									 cout<<"\n\t\tThe MAS Reward In All IS: "<<MAS_Reward_Episode;   				       					   					  					    			    	        		  	        					  					    				  
		      					//	getch();
			  		            }//END FOR
			  		            
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      End Of A Round						  *
		                                  *																	  *
		                                  *********************************************************************/ 			  		            
			  		            
	          						 Print_Text("End Of The Round");
			  						 cout<<"\n----------The Round--:"<<Round++<<"  Of  "<<Episode_Number_Ag1<<" th Episode";
			  						 Print_Q_Table(Q1,1);
			  						 Print_Q_Table(Q2,2);
			  						 Print_Q_Table(Q3,3);
									 //getch();    
									 
									 cout<<"\n Learning Rate of Agent 1 IS:"<<LearningRate(Q1,1);
		                    		 cout<<"\n Learning Rate of Agent 2 Is:"<<LearningRate(Q2,2);
		                    		 cout<<"\n Learning Rate of Agent 3 IS:"<<LearningRate(Q3,3);    		 
		      		      		      		      		      		      		      
		  				   }
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		              End Of An Episode Means That                      *
										  *					The MAS Reward Arrive To The Threshold(40)        *
		                                  *																	  *
		                                  *********************************************************************/ 			  				   
		  			 	    cout<<"\nThe Sum Of MAS: "<<MAS_Reward_Episode;
		  					Print_Text("End Of Episode");
		  					cout<<"\n The Episode Number IS: "<<Episode_Number_Ag1;
		  					//getch();
		  					
		  					NR_Ag1_Train+=NR_Ag1;  NP_Ag1_Train+=NP_Ag1;
		  					NR_Ag2_Train+=NR_Ag2;  NP_Ag2_Train+=NP_Ag2;
		  					NR_Ag3_Train+=NR_Ag3;  NP_Ag3_Train+=NP_Ag3;
		  					
		  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      The Result Of Parameter			      *
		                                  *																	  *
		                                  *********************************************************************/ 
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Learning rate					  *
		                                  *																	  *
		                                  *********************************************************************/
                            Print_Text("The Learning Rate of Agents (LR)");										   										  		  
		                    cout<<"\nNum Of Try IS: "<<Num_Of_Try;
		                    cout<<"\n Learning Rate of Agent 1 IS:"<<LearningRate(Q1,1);
		                    cout<<"\n Learning Rate of Agent 2 Is:"<<LearningRate(Q2,2);
		                    cout<<"\n Learning Rate of Agent 3 IS:"<<LearningRate(Q3,3);
		                    		  
		                    ALR1=(double(LearningRate(Q1,1)))/(double(9));
		                    ALR2=(double(LearningRate(Q2,2)))/double(9);
		                    ALR3=(double(LearningRate(Q3,3)))/double(9);
		                    		  
		                    GLR=(double(ALR1+ALR2+ALR3))/double(3);
		                    
		  					Print_Text("Learnig Rate Of Agents And MAS IS:");
		  					cout<<"\nLearning Ratio Of Agent No 1  IS:"<<ALR1;
		  					cout<<"\nLearning Ratio Of Agent No 2  IS:"<<ALR2;
		  					cout<<"\nLearning Ratio Of Agent No 3  IS:"<<ALR3;
		  
		  					cout<<"\n Learning Ratio of MAS IS: "<<GLR;
		  
		  					Print_Q_Table(Q1,1);
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Confidence						  *
		                                  *																	  *
		                                  *********************************************************************/
			         							   		  		  
		  					Cnf1=Confidence(Q1,1);
		  					Cnf2=Confidence(Q2,2);
		  					Cnf3=Confidence(Q3,3);
		  					MAS_Cnf=(Cnf1+Cnf2+Cnf3)/3;
		  
		  					Print_Text("The Confidence of Agents IS:");
		  					cout<<"\n The Confidence Of Agent 1  IS: "<<Cnf1;
		  					cout<<"\n The Confidence Of Agent 2  IS: "<<Cnf2;
		  					cout<<"\n The Confidence Of Agent 3  IS: "<<Cnf3;
		  					cout<<"\n The Confidence Of MAS  IS: "<<MAS_Cnf;
		  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Expert						      *
		                                  *																	  *
		                                  *********************************************************************/
										   		  
		  							//if(LearningRate(Q1,1)<9) 
							       //Exp_Ag1=float(NR_Ag1-NP_Ag1);
								   //float(NR_Ag1+NP_Ag1);
							     if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9)) 
								 {
								 	Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); //else Exp_Ag1=0;
								 	Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); //else Exp_Ag2=0;
								 	Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); //else Exp_Ag3=0;
								 	MAS_Exp=(Exp_Ag1+Exp_Ag2+Exp_Ag3)/3;
								  } 
								  
							     if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9)) 
								 {
								 	Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); //else Exp_Ag1=0;
								 //	Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); //else Exp_Ag2=0;
								 	Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); //else Exp_Ag3=0;
								 	MAS_Exp=(Exp_Ag1+Exp_Ag3)/2;
								  } 								  
								  
							     if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9)) 
								 {
								 	Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); //else Exp_Ag1=0;
								 	Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); //else Exp_Ag2=0;
								 //	Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); //else Exp_Ag3=0;
								 	MAS_Exp=(Exp_Ag1+Exp_Ag2)/2;
								  } 	

							     if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9)) 
								 {
								 	Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); //else Exp_Ag1=0;
								 //	Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); //else Exp_Ag2=0;
								 //	Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); //else Exp_Ag3=0;
								 	MAS_Exp=(Exp_Ag1)/1;
								  } 								  							  
								  
						/*		   Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); //else Exp_Ag1=0;
		  					if(LearningRate(Q2,2)<9)  Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); else Exp_Ag2=0;
		  					if(LearningRate(Q3,3)<9)  Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); else Exp_Ag3=0;
		  					
		  					MAS_Exp=(Exp_Ag1+Exp_Ag2+Exp_Ag3)/3;*/
		  					
		  					Print_Text("Expert Of Agents;");
		  					cout<<"\n The Reward Number Time IS: "<<NR_Ag1;
		  					cout<<"\n The Punishment Number Time IS: "<<NP_Ag1;
		  					cout<<"\n The Number Time That Agent 1 Select IS:"<<Num_Of_Try_Ag1;
		  					cout<<"\n The ExpertNess Of Agent 1 In EPisode"<<Episode_Number_Ag1<<" IS: "<<Exp_Ag1;
		  					cout<<"\n The ExpertNess Of Agent 2 In EPisode"<<Episode_Number_Ag1<<" IS: "<<Exp_Ag2;
		  					cout<<"\n The ExpertNess Of Agent 3 In EPisode"<<Episode_Number_Ag1<<" IS: "<<Exp_Ag3;
		  					cout<<"\n------------------------------------------------------------------------\n";
		  					
		  					cout<<"\n The ExpertNess Of Agent 1 In Sum Of Episode IS:"<<NR_Ag1_Train-NP_Ag1_Train;
		  					cout<<"\n The ExpertNess Of Agent 2 In Sum Of Episode IS:"<<NR_Ag2_Train-NP_Ag2_Train;
		  					cout<<"\n The ExpertNess Of Agent 3 In Sum Of Episode IS:"<<NR_Ag3_Train-NP_Ag3_Train;
		  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Certainty					      *
		                                  *																	  *
		                                  *********************************************************************/		  					
		  					
		  					Cert_Episode=(Sum_Of_Cert/Cert_Counter);
		  					Print_Text("Certainty");
		  					cout<<"The Certainty Of MAS in Episode  "<<Episode_Number_Ag1<<" Is: "<<Cert_Episode;
		  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Efiiciency					      *
		                                  *																	  *
		                                  *********************************************************************/
							     			if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9)) 										  
							     			{
							     			 cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                     cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                     cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;	
		                                     MAS_Effic=double(Non_Zero_R1+Non_Zero_R2+Non_Zero_R3)/double(3);
											 }
											 
											if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9)) 										  
							     			{
							     			 cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                   //  cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                     cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;
											 	
		                                     MAS_Effic=double(Non_Zero_R1+Non_Zero_R3)/double(2);
											 } 
											 
											if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9)) 										  
							     			{
							     			 cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                     cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                    // cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;	
		                                     MAS_Effic=double(Non_Zero_R1+Non_Zero_R2)/double(2);
											 }	
											 
											if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9)) 										  
							     			{
							     			 cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                    // cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                    // cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;	
		                                     MAS_Effic=double(Non_Zero_R1)/double(1);
											 }											 										 
										  		  					
		                                 /*   cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                    cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                    cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;	                                    		                                    
		                                    
		  									MAS_Effic=double(Non_Zero_R1+Non_Zero_R2+Non_Zero_R3)/double(3);*/
		  									cout<<"\n The Effiency of MAS IS: "<<MAS_Effic;
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Correctness					      *
		                                  *																	  *
		                                  *********************************************************************/
										  cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
										  cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;	
										  
									    /*  if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)<9)	)								  		  									
		  					                                                     MASCorr=double(CorrAg1+CorrAg2+CorrAg3)/double(3);
		  					                                                     
		  							  	  if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)<9)	)
		  							  	  										 MASCorr=double(CorrAg1+CorrAg3)/double(2);
		  							  	  										 
										  if((LearningRate(Q2,2)<9)&&(LearningRate(Q3,3)>=9)	)
		  							  	  										 MASCorr=double(CorrAg1+CorrAg2)/double(2);	  
																					   
										  if((LearningRate(Q2,2)>=9)&&(LearningRate(Q3,3)>=9)	)
		  							  	  										 MASCorr=double(CorrAg1)/double(1);		*/									  
										  									  		  									
		  					               MASCorr=(double(CorrAg1+CorrAg2+CorrAg3)/double(3))/double(NumOfRewarding);
		  							  					
		  					              cout<<"\n The MAS  Correctness IS: "<<MASCorr;
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Save To Matrix					      *
		                                  *																	  *
		                                  *********************************************************************/		  					
							
							cout<<"\n+++++++++The End Of Episode:  "<<Episode_Number_Ag1; 
							
							 
							
							if(R_Index_Res<=Res_Rec)
							{
							Result[R_Index_Res][0]=R_Index_Res+1;
							Result[R_Index_Res][1]=GLR;
							//Result[R_Index_Res][1]=ALR1;
							Result[R_Index_Res][2]=MAS_Cnf;
							//Result[R_Index_Res][2]=Cnf1;
							Result[R_Index_Res][3]=MAS_Exp;
							//Result[R_Index_Res][3]=Exp_Ag1;
							Result[R_Index_Res][4]=Cert_Episode;
							Result[R_Index_Res][5]=MAS_Effic;
							//Result[R_Index_Res][5]=Non_Zero_R1;
							Result[R_Index_Res][6]=MASCorr;
							R_Index_Res++;	
							}
							else
							{
							     			
				                 switch(Critic_Type)
					                 {
					    	          case 1:Print_Text("PTST Method");
					    	                 break;
					                  //case 2:Print_Text("Dr Beigy Method");
					                    //     break;
					                  case 2:Print_Text("T-MAS Method");
					                         break;
					                  case 3:Print_Text("T-KAg Method");	                    
					                         break;
	                                 }
	                                 Print_Result(Result);
									 //SaveToFile(Result)	;
							}
							
						cout<<"\n The Number Of Try 1  IS:"<<Num_Of_Try_Ag1;
							       cout<<"\n The Number Of Rewarding IS: "<<NR_Ag1;
								   cout<<"\n The Number Of Punishment IS: "<<NP_Ag1; 			  		  		  
		  				//	getch();         					                  	                   	                  			   					  					   ;
					 }// End Of Any Episode
					 cout<<"\n\t\tThe Episode Number IS: "<<i;
					 
					
					 					 										 
//*******************************************************************************************************************//	
//*******************************************************************************************************************//						 
//*******************************************************************************************************************//						 
		
				        /*********************************************************************
                         *																 	 *
                         *		              End Of Agent 1 Learning						 *
                         *																	 *
                         *********************************************************************/
//*******************************************************************************************************************//	
//*******************************************************************************************************************//
//*******************************************************************************************************************//						 											   			        									
//*******************************************************************************************************************//		
//*******************************************************************************************************************//				
		                /************************************************************************
		                 *																		*
		                 *				        Start Of Agent2 Learning	     				* 
		                 *																		*
		                 ************************************************************************/  
//*******************************************************************************************************************//		
//*******************************************************************************************************************//				              
					         Print_Text("Learning Agent 2");
					  		 cout<<"\n The Learning Rate Of Agent (2) IS:"<<LearningRate(Q2,2);
					  		// getch();
					  		 Num_Of_Try=0;
					  
		
		              while ((LearningRate(Q2,2)<9)&&(Episode_Number_Ag2+Episode_Number_Ag1<=Iteration))//Start Of Episode in Test Phase
	                             //for(int Iteration=1;Iteration<=100;Iteration++)
								{
							     Print_Q_Table(Q1,1);		
		                         Print_Q_Table(Q2,2);
		                         Print_Q_Table(Q3,3);
		                         cout<<"\n Learning Rate Agent 2 IS: "	<<LearningRate(Q2,2);
                                 Episode_Number_Ag2++;		    
		                         MAS_Reward_Episode=0;
		                         
		              			 NR_Ag1=0;  NR_Ag2=0; NR_Ag3=0;
					  			 NP_Ag1=0;  NP_Ag2=0; NP_Ag3=0;
					  
					  			 Num_Of_Try_Ag1=0;
					  			 Num_Of_Try_Ag2=0;
					  			 Num_Of_Try_Ag3=0; 
								   								   	
								 Sum_Of_Cert=0;  
								 Cert_Counter=0;  	                         
		                         
		                         CorrAg1=0;
		                         CorrAg2=0;
		                         CorrAg3=0;
		                         
		                         NumOfRewarding=0;
		                         		                         
		                         while(MAS_Reward_Episode<Reward_Theroshold_Proposed_Phase)// Start of Any Round	
		                              {		        
		  	                           ResetList(List_Of_Slice_Ag1,Num_Of_Slice_Ag1);
		                               ResetList(List_Of_Slice_Ag2,Num_Of_Slice_Ag2);
		                               ResetList(List_Of_Slice_Ag3,Num_Of_Slice_Ag3);
		      
		                               Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		                               Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		                               Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		      
		                               ResetList(Possible_Slice_List,Num_Of_Slices);
		                               Print_List(Possible_Slice_List,Num_Of_Slices,"List Of Possible Slices ");
		      
		                               ResetList(Possible_Cell_List,Num_Of_Cells);
		                               Print_List(Possible_Cell_List,Num_Of_Cells,"List Of Possible Cells ");
		      
		                               Init_List_Of_Agent_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Possible_Slice_List);
		                               Init_List_Of_Agent_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Possible_Slice_List);
		                               Init_List_Of_Agent_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,Possible_Slice_List);
		
		                               Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		                               Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		                               Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		      
		                               MAS_Reward_Episode=0;	
		                               MAS_Reward_Test=0;
		      		                               
		                     		   Print_Text("Start The Round (Try)");		                     
		                               cout<<"\n\n=============================================================================";
		                               cout<<"\n=============================================================================";		                               
		      
		                               for(i=0;i<Num_Of_Select;i++)
		                                  {
		      	                           Num_Of_Try++;
		      	                           //if ((Num_Of_Try%1000)==0) getch();
		      							   MAS_Reward_Test=0;
		      							   
		      							   // Sw_Ag1_Select=0;
		      	                 			Sw_Ag2_Select=0;
		      	                 			Sw_Ag3_Select=0;
		      							   
		      	                           if (i==0)
		      	                               {
	               	        /************************************************************************
		                     *																	    *
		                     *	        In The Try 1 Any Agent Try to Solving It's Task	            *    
							 *           and If MAS Receive Reward  Critic Distributes              *
							 *             It Between Agent's. If The Reward of Any                 *
							 *           Agent Arrive to it's TS It Works in next Trying            *
		                     *																		*
		                     ************************************************************************/ 		      	                               	
		      	                                //	if(R1>=TS1)
		      	     							/* {
		      	        						Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,B1,Fault1);
		  	            						Cell1=Select_Cell(Possible_Cell_List,Q1,Slice1);
		                						cout<<"\n\n\nThe Selected Fisrt Slice By Agent1 IS: "<<Slice1;
		  	            						cout<<"\nThe Selected Fisrt Cell By Agent1 IS: "<<Cell1;
			            						cout<<"\n-------------------------------------------------------------\n";		      
		                						MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);
				        						//MAS_Reward_Episode+=MAS_Reward_Test;	
				       							}*/
				  
												//    if(R2>=TS2)
		         	 							{
		      	      							 Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,B2,Fault2);
		  	          							 Cell2=Select_Cell(Possible_Cell_List,Q2,Slice2);
		              							 cout<<"\n\n\nThe Selected Fisrt Slice By Agent2 IS: "<<Slice2;
		  	          							 cout<<"\nThe Selected Fisrt Cell By Agent2 IS: "<<Cell2;
			          							 cout<<"\n---------------------------------------------------------------\n";		      
		              							 //MAS_Reward_Test+=Envionment_Feedback_Agent2_Without_Ag1(Slice2,Cell2);
				      							 //MAS_Reward_Episode+=MAS_Reward_Test;
												 				        
				                        		 //Expert Parameter
				                        		 //if(Slice2==Cell2) NR_Ag2++;else NP_Ag2++;
				                        	
										if(LearningRate(Q3,3)<9)
										{
										   if(Slice2==Cell2)  RealRewardAg2=30;	
										   else  RealRewardAg2=0;
										}
																						   
										if(LearningRate(Q3,3)>=9)
										{
										   if(Slice2==Cell2)  RealRewardAg2=35;	
										 else  RealRewardAg2=0;	
									    }												   												   
													                  
							           cout<<"\n Real Reward Of Agent2 IS: "<<RealRewardAg2;				                        		 
				                        		 
				                        		 
				                        		Sw_Ag2_Select=1;
				                        		Exp_Try2++;				                        		 
				                        		 //Expert Parameter	
												 Num_Of_Try_Ag2++;
												 
									   
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q2,Episode_Number_Ag1+Episode_Number_Ag2,Slice2,Cell2);
				                        Cert_Counter++;	
										   //Certain Parameter													 											   	
				     							} 
				     
				                                 if(LearningRate(Q3,3)<9)  MAS_Reward_Test+=Envionment_Feedback_Agent2_Without_Ag1(Slice2,Cell2);
				                                 else 					   MAS_Reward_Test+=Envionment_Feedback_Agent2_Without_Ag1(Slice2,Cell2)+5;
						
				  
												    if(LearningRate(Q3,3<9))
		      	     							{
		      	      							 Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	          							 Cell3=Select_Cell(Possible_Cell_List,Q3,Slice3);
		              							 cout<<"\n\n\nThe Selected Fisrt Slice By Agent3 IS: "<<Slice3;
		  	          							 cout<<"\nThe Selected Fisrt Cell By Agent3 IS: "<<Cell3;
			          							 cout<<"\n---------------------------------------------------------------\n";		      
		              						     MAS_Reward_Test+=Envionment_Feedback_Agent3_Without_Ag1(Slice3,Cell3);
				     							 // MAS_Reward_Episode+=MAS_Reward_Test;
				     							 
																			
										        if(Slice3==Cell3)  RealRewardAg3=25;	
										        else  RealRewardAg3=0;
										          }				     							 
												  
				                        		//Expert Parameter
				                        		//if(Slice3==Cell3) NR_Ag3++;else NP_Ag3++;
				                        		//Expert Parameter
				                        		
				                        Sw_Ag3_Select=1;
				                        Exp_Try3++;	
													                        		
												Num_Of_Try_Ag3++;
												
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q3,Episode_Number_Ag1+Episode_Number_Ag2,Slice3,Cell3);
				                        Cert_Counter++;	
										   //Certain Parameter																											  	
				     								
					 
												/*	Next_Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Bound1,Fault1);
		  	               							Next_Cell1=Select_Cell(Possible_Cell_List,Q1,Next_Slice1);	
                           							cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
		  				   							cout<<"\nThe Selected Next Cell By Agent1 IS: "<<Next_Cell1;
			  			   							cout<<"\n----------------------------------------------------------\n";	*/	  	               
							  
					 								Print_Text("The Next Selection");
												Next_Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,B2,Fault2);
		  	               								Next_Cell2=Select_Cell(Possible_Cell_List,Q2,Next_Slice2);	
                           								cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
		  				   								cout<<"\nThe Selected Next Cell By Agent2 IS: "<<Next_Cell2;  	
			  			   								cout<<"\n----------------------------------------------------------\n"; 
					 	      		
					 	      					if(LearningRate(Q3,3)<9)
					 	      					{
					 	      					Next_Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	                							Next_Cell3=Select_Cell(Possible_Cell_List,Q3,Next_Slice3); 			  			  		
	                        							cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
		  				    							cout<<"\nThe Selected Next Cell By Agent3 IS: "<<Next_Cell3;
		  				    							cout<<"\n----------------------------------------------------------\n"; 	
					 	      						
												   }
					 	      					
		      									
		  				    
		  										/*MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);
				         						  MAS_Reward_Episode+=MAS_Reward_Test;
				         
				    							  MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2);
				         						  MAS_Reward_Episode+=MAS_Reward_Test;
					
												  MAS_Reward_Test+=Envionment_Feedback_Agent3(Slice3,Cell3);
				         						  MAS_Reward_Episode+=MAS_Reward_Test;*/	     
					  						/************************************************************************
		              						 *																	    *
		              						 *			      The Heart Of The Program	(Agent 2)					*
		              						 *																		*
		              						 ************************************************************************/    
					    						ResetListFloat(Reward_List,Num_Of_Agent);
					    						
					    						if(LearningRate(Q3,3)<9)
					    						{
					    						switch(Critic_Type)
					    							{
					    							 case 1:Critic_Type_1_Without_Ag1(MAS_Reward_Test,Reward_List);
					    	       							break;
					        						 case 2:Critic_DB_Without_Ag1(MAS_Reward_Test,Reward_List);
					        	   							break;
					        						 case 3:Critic_Type_TS_Agents_priority_Without_Ag1(MAS_Reward_Test,Reward_List);
					               							break;
					        						 case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag1(MAS_Reward_Test,Reward_List);	                    
					               							break;
	                    							}	
												}
					    					/**************************   Agent 3 Learnt   *************************/  
	                    					    if(LearningRate(Q3,3)>=9)
					    						{
					    						switch(Critic_Type)
					    							{
					    							 case 1:Critic_Type_1_Without_Ag1_Ag3(MAS_Reward_Test,Reward_List);
					    	       							break;
					        						 case 2:Critic_DB_Without_Ag1_Ag3(MAS_Reward_Test,Reward_List);
					        	   							break;
					        						 case 3:Critic_Type_TS_Agents_priority_Without_Ag1_Ag3(MAS_Reward_Test,Reward_List);
					               							break;
					        						 case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag1_Ag3(MAS_Reward_Test,Reward_List);	                    
					               							break;
	                    							}	
												}		
					                        /*****************************************************************************/                    							
					                                cout<<"\n Step(TRY) Reward IS: "<<MAS_Reward_Test<<"\n";					    	                         	                    	                   	                      					
					    						   
												    // R1=Reward_List[0];
		  	            							R2=Reward_List[1];
		  	            							R3=Reward_List[2];
		  	            							
		  	            							NumOfRewarding++;
		  	            		  	          
													//	cout<<"\n R1 In First Step IS: "<<R1<<" \t TS1 IS:"<<TS1;
		  	            							cout<<"\n R2 In First Step IS: "<<R2<<" \t TS2 IS:"<<TS2;
		  	            							cout<<"\n R3 In First Step IS: "<<R3<<" \t TS3 IS:"<<TS3;
		  	            							
		  	            			 //****************ExpertNess****************//
										 /*  if (Sw_Ag1_Select==1) 
										   {
										   	if (R1>0)  NR_Ag1++; else NP_Ag1++;
										   }*/
										   
										  // if (LearningRate(Q2,2)<9)	
										  //        {
										   	       if(Sw_Ag2_Select==1)
													  {
													  	if(R2>0)    NR_Ag2;else NP_Ag2++;
													  }
										   //       }
										          
										   if (LearningRate(Q3,3)<9)	
										          {
										   	       if (Sw_Ag3_Select==1) 
													  {
													  	if (R3>0)  NR_Ag3;else NP_Ag3++;
													  }
													  
										          }			  	            		 
		  	            			 //****************ExpertNess****************//		  	            							
													  
										// Efficeincy	 
										   if(MAS_Reward_Test>0)  Non_Zero_Reward++;
										   if(R1>0)	Non_Zero_R1++;
										   if(R2>0)	Non_Zero_R2++;										   				 
										   if(R3>0)	Non_Zero_R3++;
										// Efficiency   
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Correctness					      *
		                                  *																	  *
		                                  *********************************************************************/
											//*******************Correctness************//
									//cout<<"\nR1 IS:"<<R1;
										cout<<"\nR2 IS:"<<R2;
										cout<<"\nR3 IS:"<<R3;
										
									//	cout<<"\n The Real Reward Of Agent1 IS: "<<RealRewardAg1;
										cout<<"\n The Real Reward Of Agent2 IS: "<<RealRewardAg2;
										cout<<"\n The Real Reward Of Agent3 IS: "<<RealRewardAg3;										
										
									//	if(abs(R1-RealRewardAg1)<=CorrThreshold )  CorrAg1++;else CorrAg1--;
										if(abs(R2-RealRewardAg2)<=CorrThreshold )  CorrAg2++;else CorrAg2--;
										if(abs(R3-RealRewardAg3)<=CorrThreshold ) CorrAg3++;else CorrAg3--;
										
									//	  cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
										  cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;										  		  									
		  					              // MASCorr=double(CorrAg1+CorrAg2+CorrAg3)/double(3);
		  							  															
										//*******************Correctness************// 																				  		  	            							
					 
					  								// if (MAS_Reward_Test>0) getch();					  
					                                //   UpDate_QTable(Q1,Slice1,Cell1,R1,Next_Slice1);
	                                                if(R2>=TS2)				               UpDate_QTable(Q2,Slice2,Cell2,R2,Next_Slice2);
	                                                if((R3>=TS3)&&(LearningRate(Q3,3)<9))  UpDate_QTable(Q3,Slice3,Cell3,R3,Next_Slice3);
										    					    
                       				                 cout<<"\n\n--------------------------------------------------------";
			           				 				 cout<<"\n-                                                      -";
			           				 				 cout<<"\n-                                                      -";
			           				 				 cout<<"\n           End   Of  Try   "<<i+1<<"          -";
			           				 				 cout<<"\n-                                                      -";
			           				 				 cout<<"\n--------------------------------------------------------";
														 
	               	        				/************************************************************************
		                     				 *																	    *
		                     				 *				        End Of First Try In Round	     			    *
		                     				 *																		*
		                     				 ************************************************************************/  														 															   						  
						 	    
				  							   }
				  						else
				  								{
                     					   /*************************************************************************
		              						*																 	    *
		              						*			      The Second And Third Try In Round					    *
		              						*																	    *
		              						*************************************************************************/  
		             							/* if (R1>=TS1)
					   								{
	                  								Slice1=Next_Slice1;
	                    							Cell1=Next_Cell1;
					   								}  */
	                  							  if (R2>=TS2)
	                  								{
	                  								 Slice2=Next_Slice2;
		   	            							 Cell2=Next_Cell2;
		                                             cout<<"\n\n\nThe  "<<i+1<<" th Selected  Slice By Agent 2 IS: "<<Slice2;
		  	                                         cout<<"\nThe  "<<i+1<<"  th Selected  Cell By Agent 2 IS: "<<Cell2;
													   
										if(LearningRate(Q3,3)<9)
										{
										   if(Slice2==Cell2)  RealRewardAg2=30;	
										   else  RealRewardAg2=0;
										}
																						   
										if(LearningRate(Q3,3)>=9)
										{
										   if(Slice2==Cell2)  RealRewardAg2=35;	
										 else  RealRewardAg2=0;	
									    }												   												   
													                  
							           cout<<"\n Real Reward Of Agent2 IS: "<<RealRewardAg2;													   	  		   	            							 
		   	            							 
	                                     			 //Expert Parameter
				                         			 //if(Slice2==Cell2) NR_Ag2++;  else NP_Ag2++;
				                         			 //Expert Parameter	
													 Num_Of_Try_Ag2++; 	
													 
				                        		Sw_Ag2_Select=1;
				                        		Exp_Try2++;														  
													 
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q2,Episode_Number_Ag1+Episode_Number_Ag2,Slice2,Cell2);
				                        Cert_Counter++;	
										   //Certain Parameter																   	            							 
					  								}
					  							  else cout<<"\n\t Not Selection With The Agent 2";
														
		   	          							  if((R3>=TS3)&&(LearningRate(Q3,3)<9))
		   	          								{
		   	          								 Slice3=Next_Slice3; 	      			         			          
			            							 Cell3=Next_Cell3;
		                                             cout<<"\n\n\nThe  "<<i+1<<" th Selected  Slice By Agent 3 IS: "<<Slice3;
		  	                                         cout<<"\nThe  "<<i+1<<"  th Selected  Cell By Agent 3 IS: "<<Cell3;
		  	                                         
 													if(Slice3==Cell3)  RealRewardAg3=25;	
										        		else  RealRewardAg3=0;		  	                                         
			            							 
	                                     			 //Expert Parameter
				                         			// if(Slice3==Cell3) NR_Ag3++;  else NP_Ag3++;
				                         			 //Expert Parameter
													  Num_Of_Try_Ag3++;	
													  
				                        		Sw_Ag3_Select=1;
				                        		Exp_Try3++;														  
													  
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q3,Episode_Number_Ag1+Episode_Number_Ag2,Slice3,Cell3);
				                        Cert_Counter++;	
										   //Certain Parameter																  		            							 
						 							}
												  else cout<<"\n\t Not Selection With The Agent 3"  ;	 		
		              		              		              
					                              /* if(R1>=TS1)
						                            {
						                             MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);
				                                  // MAS_Reward_Episode+=MAS_Reward_Test;	
						                            } */
						                          if(R2>=TS2)  
													{
													 //MAS_Reward_Test+=Envionment_Feedback_Agent2_Without_Ag1(Slice2,Cell2);
						 							 if(LearningRate(Q3,3)<9)  MAS_Reward_Test+=Envionment_Feedback_Agent2_Without_Ag1(Slice2,Cell2);
				         							 else 					   MAS_Reward_Test+=Envionment_Feedback_Agent2_Without_Ag1(Slice2,Cell2)+5;
				        							 // MAS_Reward_Episode+=MAS_Reward_Test;	
													}
													
				        						  if(R3>=TS3) 
				        							{
				        							 if	(LearningRate(Q3,3)<9)
				         							 MAS_Reward_Test+=Envionment_Feedback_Agent3_Without_Ag1(Slice3,Cell3);
				        							 // MAS_Reward_Episode+=MAS_Reward_Test;	
													}
					    
					                              ResetListFloat(Reward_List,Num_Of_Agent);
					    					                              
													
					    						if(LearningRate(Q3,3)<9)
					    						{
					    						switch(Critic_Type)
					    							{
					    							 case 1:Critic_Type_1_Without_Ag1(MAS_Reward_Test,Reward_List);
					    	       							break;
					        						 case 2:Critic_DB_Without_Ag1(MAS_Reward_Test,Reward_List);
					        	   							break;
					        						 case 3:Critic_Type_TS_Agents_priority_Without_Ag1(MAS_Reward_Test,Reward_List);
					               							break;
					        						 case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag1(MAS_Reward_Test,Reward_List);	                    
					               							break;
	                    							}	
												}
					    					/**************************   Agent 3 Learnt   *************************/  
	                    					    if(LearningRate(Q3,3)>=9)
					    						{
					    						switch(Critic_Type)
					    							{
					    							 case 1:Critic_Type_1_Without_Ag1_Ag3(MAS_Reward_Test,Reward_List);
					    	       							break;
					        						 case 2:Critic_DB_Without_Ag1_Ag3(MAS_Reward_Test,Reward_List);
					        	   							break;
					        						 case 3:Critic_Type_TS_Agents_priority_Without_Ag1_Ag3(MAS_Reward_Test,Reward_List);
					               							break;
					        						 case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag1_Ag3(MAS_Reward_Test,Reward_List);	                    
					               							break;
	                    							}	
												}		
					                        /*****************************************************************************/  																	    
		                    						cout<<"\n Learning Rate of Agent 1 IS:"<<LearningRate(Q1,1);
		                    						cout<<"\n Learning Rate of Agent 2 Is:"<<LearningRate(Q2,2);
		                    						cout<<"\n Learning Rate of Agent 3 IS:"<<LearningRate(Q3,3);					    			 		                      	                    					     					     					     
					                 				cout<<"\n\t Reward In Step(Try) IS: "<<MAS_Reward_Test;	                    	                    	                    	                                        	                    	                    	                    	                   	                    	                       					     					     
					    						  
					   							  // R1=Reward_List[0];
		  	            						  R2=Reward_List[1];
		  	              						  R3=Reward_List[2];
		  	            		  	          
		  	            		  	          NumOfRewarding++;
		  	            		  	          
												  //	cout<<"\n R1  IS: "<<R1<<" \t TS1 IS:"<<TS1;
		  	            						  cout<<"\n R2  IS: "<<R2<<" \t TS2 IS:"<<TS2;
		  	            						  cout<<"\n R3  IS: "<<R3<<" \t TS3 IS:"<<TS3;
		  	            						  
		  	            			 //****************ExpertNess****************//
										 /*  if (Sw_Ag1_Select==1) 
										   {
										   	if (R1>0)  NR_Ag1++; else NP_Ag1++;
										   }*/
										   
										 //  if (LearningRate(Q2,2)<9)	
										  //        {
										   	       if(Sw_Ag2_Select==1)
													  {
													  	if(R2>0)    NR_Ag2;else NP_Ag2++;
													  }
										   //       }
										          
										   if (LearningRate(Q3,3)<9)	
										          {
										   	       if (Sw_Ag3_Select==1) 
													  {
													  	if (R3>0)  NR_Ag3;else NP_Ag3++;
													  }
													  
										          }			  	            		 
		  	            			 //****************ExpertNess****************//		  	            						  
		  	            						  
										// Efficeincy	 
										   if(MAS_Reward_Test>0)  Non_Zero_Reward++;
										   if(R1>0)	Non_Zero_R1++;
										   if(R2>0)	Non_Zero_R2++;										   				 
										   if(R3>0)	Non_Zero_R3++;
										// Efficiency   	
										
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Correctness					      *
		                                  *																	  *
		                                  *********************************************************************/
										//*******************Correctness************//
								//	cout<<"\nR1 IS:"<<R1;
										cout<<"\nR2 IS:"<<R2;
										cout<<"\nR3 IS:"<<R3;
										
								//		cout<<"\n The Real Reward Of Agent1 IS: "<<RealRewardAg1;
										cout<<"\n The Real Reward Of Agent2 IS: "<<RealRewardAg2;
										cout<<"\n The Real Reward Of Agent3 IS: "<<RealRewardAg3;										
										
								//		if(abs(R1-RealRewardAg1)<=CorrThreshold )  CorrAg1++;else CorrAg1--;
										if(abs(R2-RealRewardAg2)<=CorrThreshold )  CorrAg2++;else CorrAg2--;
										if(abs(R3-RealRewardAg3)<=CorrThreshold ) CorrAg3++;else CorrAg3--;
										
								//		  cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
										  cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;										  		  									
		  					      //         MASCorr=double(CorrAg1+CorrAg2+CorrAg3)/double(3);
		  							  															
										//*******************Correctness************// 									  	            						  
															  	            
		  	         							  //  if(R1>=TS1)  UpDate_QTable(Q1,Slice1,Cell1,R1,Next_Slice1);
	                   							  if(R2>=TS2)               			 UpDate_QTable(Q2,Slice2,Cell2,R2,Next_Slice2);
	                   							  if((R3>=TS3)&&(LearningRate(Q3,3)<9))  UpDate_QTable(Q3,Slice3,Cell3,R3,Next_Slice3);	
					 
					  							  //getch();
					  					   /***********************************************************************
		               						*																 	  *
		               						*		Select The Next States(Slice) and Actions(Cells)			  *
		               						*																	  *
		               						***********************************************************************/   				  	
				  	
				  								  if (i==Num_Of_Select-1) 
		               								{
		             								 //   Next_Slice1=rand()%(Num_Of_Slices-0);
			 										 Next_Slice2=rand()%(Num_Of_Slices-0); cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
			 										 Next_Slice3=rand()%(Num_Of_Slices-0); cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;	
													}
												  else
			           								{
			          								 /* 	if(R1>=TS1)
			           								    {
			           	   								 Next_Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Bound1,Fault1);
		  	               								 Next_Cell1=Select_Cell(Possible_Cell_List,Q1,Next_Slice1);	
                           								 cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
		  				   								 cout<<"\nThe Selected Next Cell By Agent1 IS: "<<Next_Cell1;
			  			   								 cout<<"\n-------------------\n"	;		  	               
						   								}*/
		                							  if(R2>=TS2)
		                								{
		                   								 Next_Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,B2,Fault2);
		  	               								 Next_Cell2=Select_Cell(Possible_Cell_List,Q2,Next_Slice2);	
                           								 cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
		  				   								 cout<<"\nThe Selected Next Cell By Agent2 IS: "<<Next_Cell2;  	
			  			   								 cout<<"\n-------------------\n"	;		  	               
														}
		  			  								  if((R3>=TS3)&&(LearningRate(Q3,3)<9))
		  			  									{
		  			  	    							 Next_Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	                							 Next_Cell3=Select_Cell(Possible_Cell_List,Q3,Next_Slice3); 			  			  		
	                        							 cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
		  				    							 cout<<"\nThe Selected Next Cell By Agent3 IS: "<<Next_Cell3;
														 cout<<"\n-------------------\n"	;  	  	            
														}
		  	            		  		  	            		    		    		                	  									              				
				 	   								}	
				  
		      	       								//cout<<"\n --------------------End The Step: "<<i+1<<"---------------------\n";
				        				        
		  	       								}
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		        End Of The Second and Third Try						  *
		                                  *																	  *
		                                  *********************************************************************/ 
										    					    
                       				 cout<<"\n\n--------------------------------------------------------";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n           End   Of  Try   "<<i+1<<"          -";
			           				 cout<<"\n-                                                      -";
			           				 cout<<"\n--------------------------------------------------------";					    					
					                			MAS_Reward_Episode+=MAS_Reward_Test; 
					                			cout<<"\n\t The MAS Reward In Try "<<i+1<<" IS: "<<MAS_Reward_Test;
					                			cout<<"\n\t\tThe MAS Reward In All IS: "<<MAS_Reward_Episode;   				       					   					  					    			    	        		  	        					  					    				  
		      					   				//getch();
			                			  }//END FOR
			  		            
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      End Of A Round						  *
		                                  *																	  *
		                                  *********************************************************************/ 			  		            
			  		            
	          						 Print_Text("End Of The Round");
			  						 cout<<"\n----------The Round--:"<<Round++<<"  Of"<<Episode_Number_Ag2<<" th Episode";
			  						 Print_Q_Table(Q1,1);
			  						 Print_Q_Table(Q2,2);
			  						 Print_Q_Table(Q3,3);
									 //getch();        		 			                			  	                		  		      		      		      		      		      		      		      
		  							  }//END WHILE(TRAIN)
		  							  
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		              End Of A 	Train Means That                      *
										  *					The MAS Reward Arrive To The Threshold(40)        *
		                                  *																	  *
		                                  *********************************************************************/ 			  				   
		  			 	    cout<<"\nThe Sum Of MAS: "<<MAS_Reward_Episode;
		  					Print_Text("End Of Episode");
		  					cout<<"\n The Episode Number Of Agent2 IS: "<<Episode_Number_Ag2;
                            cout<<"\n The Episode Number For MAS IS: "<<Episode_Number_Ag2+Episode_Number_Ag1;		  					
		  					//getch();
		  					
		  					//NR_Ag1_Train+=NR_Ag1;  NP_Ag1_Train+=NP_Ag1;
		  					NR_Ag2_Train+=NR_Ag2;  NP_Ag2_Train+=NP_Ag2;
		  					NR_Ag3_Train+=NR_Ag3;  NP_Ag3_Train+=NP_Ag3;		  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      The Result Of Parameter			      *
		                                  *																	  *
		                                  *********************************************************************/ 
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Learning rate					  *
		                                  *																	  *
		                                  *********************************************************************/
                           Print_Text("The Learning Rate of Agents (LR)");										   										  		  
		                    cout<<"\nNum Of Try IS: "<<Num_Of_Try;
		                    cout<<"\n Learning Rate of Agent 1 IS:"<<LearningRate(Q1,1);
		                    cout<<"\n Learning Rate of Agent 2 Is:"<<LearningRate(Q2,2);
		                    cout<<"\n Learning Rate of Agent 3 IS:"<<LearningRate(Q3,3);
		                    
		                    ALR1=(double(LearningRate(Q1,1)))/(double(9));
		                    ALR2=(double(LearningRate(Q2,2)))/(double(9));
		                    ALR3=(double(LearningRate(Q3,3)))/(double(9));
		                    		  
		                    GLR=(double(ALR1+ALR2+ALR3))/(double(3));
		                    
		  					Print_Text("Learnig Rate Of Agents And MAS IS:");
		  					cout<<"\nLearning Ratio Of Agent No 1  IS:"<<ALR1;
		  					cout<<"\nLearning Ratio Of Agent No 2  IS:"<<ALR2;
		  					cout<<"\nLearning Ratio Of Agent No 3  IS:"<<ALR3;
		  
		  					cout<<"\n Learning Ratio of MAS IS: "<<GLR;
		  					
		  					Print_Q_Table(Q1,1);
		  					Print_Q_Table(Q2,2);
		  					Print_Q_Table(Q3,3);
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Confidence						  *
		                                  *																	  *
		                                  *********************************************************************/
							Cnf1=Confidence(Q1,1);
		  					Cnf2=Confidence(Q2,2);
		  					Cnf3=Confidence(Q3,3);
		  					MAS_Cnf=(Cnf1+Cnf2+Cnf3)/3;
		  
		  					Print_Text("The Confidence of Agents IS:");
		  					cout<<"\n The Confidence Of Agent 1  IS: "<<Cnf1;
		  					cout<<"\n The Confidence Of Agent 2  IS: "<<Cnf2;
		  					cout<<"\n The Confidence Of Agent 3  IS: "<<Cnf3;
		  					cout<<"\n The Confidence Of MAS  IS: "<<MAS_Cnf;
		  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Expert						      *
		                                  *																	  *
		                                  *********************************************************************/
		                                  
							     if(LearningRate(Q3,3)<9)
								 {
								 //	Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); //else Exp_Ag1=0;
								 	Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); //else Exp_Ag2=0;
								 	Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); //else Exp_Ag3=0;
								 	MAS_Exp=(Exp_Ag1+Exp_Ag2+Exp_Ag3)/2;
								  } 
								  							  		  
								  
							     if(LearningRate(Q3,3)>=9)
								 {
								 //	Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); //else Exp_Ag1=0;
								 	Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); //else Exp_Ag2=0;
								 //	Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); //else Exp_Ag3=0;
								 	MAS_Exp=(Exp_Ag2)/1;
								  } 	
							   					                                  
		                                  
		  				/*	if(LearningRate(Q1,1)<9)  Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); else Exp_Ag1=0;
		  					if(LearningRate(Q2,2)<9)  Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); else Exp_Ag2=0;
		  					if(LearningRate(Q3,3)<9)  Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); else Exp_Ag3=0;
		  					MAS_Exp=(Exp_Ag1+Exp_Ag2+Exp_Ag3)/3;*/
							  		                                  
		                    //Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1));
		  					//Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2));
		  					//Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3));
		  					//MAS_Exp=(Exp_Ag2+Exp_Ag3)/2;
		  					
		  					Print_Text("Expert Of Agents;");
		  					//cout<<"\n The ExpertNess Of Agent 1 In EPisode"<<Episode_Number_Ag1<<" IS: "<<Exp_Ag1;
		  					cout<<"\n The ExpertNess Of Agent 2 In EPisode"<<Episode_Number_Ag2<<" IS: "<<Exp_Ag2;
		  					cout<<"\n The ExpertNess Of Agent 3 In EPisode"<<Episode_Number_Ag2<<" IS: "<<Exp_Ag3;
		  					cout<<"\n------------------------------------------------------------------------\n";
		  					
		  					//cout<<"\n The ExpertNess Of Agent 1 In Sum Of Episode IS:"<<NR_Ag1_Train-NP_Ag1_Train;
		  					cout<<"\n The ExpertNess Of Agent 2 In Sum Of Episode IS:"<<NR_Ag2_Train-NP_Ag2_Train;
		  					cout<<"\n The ExpertNess Of Agent 3 In Sum Of Episode IS:"<<NR_Ag3_Train-NP_Ag3_Train;		  					
		  					
		  					
		  					//cout<<"\n The ExpertNess Of Agent 1  IS: "<<Exp_Ag1;
		  					//cout<<"\n The ExpertNess Of Agent 2  IS: "<<Exp_Ag2;
		  					//cout<<"\n The ExpertNess Of Agent 3  IS: "<<Exp_Ag3;
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Certainty					      *
		                                  *																	  *
		                                  *********************************************************************/		  					
		  					
		  					Cert_Episode=(Sum_Of_Cert/Cert_Counter);
		  					Print_Text("Certainty");
		  					cout<<"The Certainty Of MAS in Episode  "<<Episode_Number_Ag1+Episode_Number_Ag2<<" Is: "<<Cert_Episode;
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Efiiciency					      *
		                                  *																	  *
		                                  *********************************************************************/	
											if(LearningRate(Q3,3)<9)										 
							     			{
							     			// cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                     cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                     cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;	
		                                     MAS_Effic=double(Non_Zero_R2+Non_Zero_R3)/double(2);
											 }	
											 
											if(LearningRate(Q3,3)>=9)
							     			{
							     			// cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                     cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                    // cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;	
		                                     MAS_Effic=double(Non_Zero_R2)/double(1);
											 } 									  
										  	  					
		                                  /*  cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                    cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                    cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;	                                    		                                    
		                                    
		  									MAS_Effic=double(Non_Zero_R1+Non_Zero_R2+Non_Zero_R3)/double(3);*/
		  									cout<<"\n The Effiency of MAS IS: "<<MAS_Effic;	
											  
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Correctness					      *
		                                  *																	  *
		                                  *********************************************************************/
									/*		if(LearningRate(Q3,3)<9)	MASCorr=double(CorrAg2+CorrAg3)/double(2);
											else MASCorr=double(CorrAg2)/double(1);		*/	                                  
		                                  
										  //cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
										  cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;										  		  									
		  					               MASCorr=(double(CorrAg1+CorrAg2+CorrAg3)/double(3))/double(NumOfRewarding);	
											
										cout<<"\n The MAS  Correctness IS: "<<MASCorr;	
																					  	  					
		  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Save To Matrix					      *
		                                  *																	  *
		                                  *********************************************************************/		  					
							
							cout<<"\n+++++++++The End Of Agent2  Episode :  "<<Episode_Number_Ag2;							
                            cout<<"\n The Episode Number IS: "<<Episode_Number_Ag2+Episode_Number_Ag1;
							
							if(R_Index_Res<=Res_Rec)
							{
							Result[R_Index_Res][0]=R_Index_Res+1;
							Result[R_Index_Res][1]=GLR;
							Result[R_Index_Res][2]=MAS_Cnf;
							Result[R_Index_Res][3]=MAS_Exp;
							Result[R_Index_Res][4]=Cert_Episode;
							Result[R_Index_Res][5]=MAS_Effic;
							Result[R_Index_Res][6]=MASCorr;							
							R_Index_Res++;	
							}
							else
							
							{
			                 switch(Critic_Type)
					                 {
					    	          case 1:Print_Text("PTST Method");
					    	                 break;
					                 // case 2:Print_Text("Dr Beigy Method");
					                 //        break;
					                  case 2:Print_Text("T-MAS Method");
					                         break;
					                  case 3:Print_Text("T-KAg Method");	                    
					                         break;
	                                 }
	                                 Print_Result(Result);
									 //SaveToFile(Result)	;	
							}
							
							
							
							
							 		  
		   //getch();         					                  	                   	                  			   					  					   ;
		}// End Of Any Episode
					        			
		cout<<"\n The Episode Number IS: "<<Episode_Number_Ag2+Episode_Number_Ag1;
		cout<<"\n Learning Rate:"<<LearningRate(Q1,1);
		cout<<"\n Learning Rate:"<<LearningRate(Q2,2);
		cout<<"\n Learning Rate:"<<LearningRate(Q3,3);
		//PrintNormalizedQT(Q2,2);
		
		
		
//*******************************************************************************************************************//	
//*******************************************************************************************************************//						 
//*******************************************************************************************************************//						 
		
				        /*********************************************************************
                         *																 	 *
                         *		              End Of Agent 2 Learning						 *
                         *																	 *
                         *********************************************************************/
//*******************************************************************************************************************//	
//*******************************************************************************************************************//
//*******************************************************************************************************************//						 											   			        									
//*******************************************************************************************************************//		
//*******************************************************************************************************************//				
		                /************************************************************************
		                 *																		*
		                 *				        Start Of Agent3 Learning	     				* 
		                 *																		*
		                 ************************************************************************/  
//*******************************************************************************************************************//		
//*******************************************************************************************************************//				              
					         Print_Text("Learning Agent 3");
					  		 cout<<"\n The Learning Rate Of Agent (3) IS:"<<LearningRate(Q3,3);
					  		// getch();
					  		 Num_Of_Try=0;		
//*******************************************************************************************************************//					  
		
							while ((LearningRate(Q3,3)<9)&&(Episode_Number_Ag3+Episode_Number_Ag2+Episode_Number_Ag1<=Iteration)) //Start Of Episode in Test Phase
	  						//for(int Iteration=1;Iteration<=100;Iteration++)
							   {
							   	
							     Print_Q_Table(Q1,1);		
		                         Print_Q_Table(Q2,2);
		                         Print_Q_Table(Q3,3);
		                         cout<<"\n Learning Rate Agent 3 IS: "	<<LearningRate(Q3,3);
                                 Episode_Number_Ag3++;		    
		                         MAS_Reward_Episode=0;
		                         
		              			 NR_Ag1=0;  NR_Ag2=0; NR_Ag3=0;
					  			 NP_Ag1=0;  NP_Ag2=0; NP_Ag3=0;
					  
					  			 Num_Of_Try_Ag1=0;
					  			 Num_Of_Try_Ag2=0;
					  			 Num_Of_Try_Ag3=0; 	
								   
								 Sum_Of_Cert=0;
								 Cert_Counter=0;	
								 
								 CorrAg3=0;	
								 
								 NumOfRewarding=0;						   	 							   	
		  								  						
		  						while(MAS_Reward_Episode<Reward_Theroshold_Proposed_Phase)// Start of Any Round	
		  							{
		        	 				 ResetList(List_Of_Slice_Ag1,Num_Of_Slice_Ag1);
		      						 ResetList(List_Of_Slice_Ag2,Num_Of_Slice_Ag2);
		      						   ResetList(List_Of_Slice_Ag3,Num_Of_Slice_Ag3);
		      
		      						 Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		      						 Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		      						   Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		      
		      						   ResetList(Possible_Slice_List,Num_Of_Slices);
		      						   Print_List(Possible_Slice_List,Num_Of_Slices,"List Of Possible Slices ");
		      
		       						   ResetList(Possible_Cell_List,Num_Of_Cells);
		       						   Print_List(Possible_Cell_List,Num_Of_Cells,"List Of Possible Cells ");
		      
		      						   Init_List_Of_Agent_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Possible_Slice_List);
		      						   Init_List_Of_Agent_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Possible_Slice_List);
		      						   Init_List_Of_Agent_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,Possible_Slice_List);
		
		      						   Print_List(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,"List Of Slice Agent1 ");
		      						   Print_List(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,"List Of Slice Agent2 ");
		      						   Print_List(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,"List Of Slice Agent3 ");
		      
		      							MAS_Reward_Episode=0;	
		      							MAS_Reward_Test=0;
		      		      									      									                               		      		                            
		                     		    Print_Text("Start The Round (Try)");		                     
		                                cout<<"\n\n=============================================================================";
		                                cout<<"\n=============================================================================";			      							
		      
		      								for(i=0;i<Num_Of_Select;i++)
		      									{
		      									 Num_Of_Try++;
		      									 //if ((Num_Of_Try%1000)==0) getch();
		      									 MAS_Reward_Test=0;
		      									 
		      									 Sw_Ag3_Select=0;
		      									 
		      									 if (i==0)
		      										{
	               	        /************************************************************************
		                     *																	    *
		                     *	        In The Try 1 Any Agent Try to Solving It's Task	            *    
							 *           and If MAS Receive Reward  Critic Distributes              *
							 *             It Between Agent's. If The Reward of Any                 *
							 *           Agent Arrive to it's TS It Works in next Trying            *
		                     *																		*
		                     ************************************************************************/ 
							 		      											
		      		 //	if(R1>=TS1)
		      	     /* {
		      	        Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,B1,Fault1);
		  	            Cell1=Select_Cell(Possible_Cell_List,Q1,Slice1);
		                cout<<"\n\n\nThe Selected Fisrt Slice By Agent1 IS: "<<Slice1;
		  	            cout<<"\nThe Selected Fisrt Cell By Agent1 IS: "<<Cell1;
			            cout<<"\n-------------------------------------------------------------\n";		      
		                MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);
				        //MAS_Reward_Episode+=MAS_Reward_Test;	
				       }*/
				  
				//    if(R2>=TS2)
		         	/* {
		      	      Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,B2,Fault2);
		  	          Cell2=Select_Cell(Possible_Cell_List,Q2,Slice2);
		              cout<<"\n\n\nThe Selected Fisrt Slice By Agent2 IS: "<<Slice2;
		  	          cout<<"\nThe Selected Fisrt Cell By Agent2 IS: "<<Cell2;
			          cout<<"\n---------------------------------------------------------------\n";		      
		              MAS_Reward_Test+=Envionment_Feedback_Agent2_Without_Ag1(Slice2,Cell2);
				      //MAS_Reward_Episode+=MAS_Reward_Test;	
				     } */
				  
				//    if(R3>=TS3)
		      	     						{
		      	      							Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	          							Cell3=Select_Cell(Possible_Cell_List,Q3,Slice3);
		              							cout<<"\n\n\nThe Selected Fisrt Slice By Agent3 IS: "<<Slice3;
		  	          							cout<<"\nThe Selected Fisrt Cell By Agent3 IS: "<<Cell3;
			          							cout<<"\n---------------------------------------------------------------\n";
			          							
			          							if(Slice3==Cell3)  RealRewardAg3=60;
			          							else RealRewardAg3=0;
												  		      
		              							MAS_Reward_Test+=Envionment_Feedback_Agent3_Without_Ag1_Ag2(Slice3,Cell3);
				     							// MAS_Reward_Episode+=MAS_Reward_Test;				     
				     							//Expert Parameter
				     							//if(Slice3==Cell3) NR_Ag3++;else NP_Ag3++;
				     							//Expert Parameter					 
					 							Num_Of_Try_Ag3++;
					 							
				                        		Sw_Ag3_Select=1;
				                        		Exp_Try3++;						 							
												 
									   
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q3,Episode_Number_Ag1+Episode_Number_Ag2+Episode_Number_Ag3,Slice3,Cell3);
				                        Cert_Counter++;	
										   //Certain Parameter													 	
				     						} 	
					 
				/*	Next_Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Bound1,Fault1);
		  	               Next_Cell1=Select_Cell(Possible_Cell_List,Q1,Next_Slice1);	
                           cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
		  				   cout<<"\nThe Selected Next Cell By Agent1 IS: "<<Next_Cell1;
			  			   cout<<"\n----------------------------------------------------------\n";	*/	  	               
							  
					 
				/*	Next_Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Bound2,Fault2);
		  	               Next_Cell2=Select_Cell(Possible_Cell_List,Q2,Next_Slice2);	
                           cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
		  				   cout<<"\nThe Selected Next Cell By Agent2 IS: "<<Next_Cell2;  	
			  			   cout<<"\n----------------------------------------------------------\n"; */
					 	      					Print_Text("The Next Selection");
		      								 	Next_Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	                				 	Next_Cell3=Select_Cell(Possible_Cell_List,Q3,Next_Slice3); 			  			  		
	                        				 	cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
		  				    				 	cout<<"\nThe Selected Next Cell By Agent3 IS: "<<Next_Cell3;
		  				    				 	cout<<"\n----------------------------------------------------------\n";
							  				            					   		  				    
		  			/*MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);
				         MAS_Reward_Episode+=MAS_Reward_Test;
				         
				    MAS_Reward_Test+=Envionment_Feedback_Agent2(Slice2,Cell2);
				         MAS_Reward_Episode+=MAS_Reward_Test;
					
					MAS_Reward_Test+=Envionment_Feedback_Agent3(Slice3,Cell3);
				         MAS_Reward_Episode+=MAS_Reward_Test;*/	     
					  /************************************************************************
		              *																		  *
		              *			      The Heart Of The Program								  *
		              *																		  *
		              *************************************************************************/    
					    						ResetListFloat(Reward_List,Num_Of_Agent);
					    
					    						switch(Critic_Type)
					    							{
					    							 case 1:Critic_Type_1_Without_Ag1_Ag2(MAS_Reward_Test,Reward_List);
					    	       							break; 
					        						 case 2:Critic_DB_Without_Ag1_Ag2(MAS_Reward_Test,Reward_List);
					               							break;
					        						 case 3:Critic_Type_TS_Agents_priority_Without_Ag1_Ag2(MAS_Reward_Test,Reward_List);
					               							break;
					        						 case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag1_Ag2(MAS_Reward_Test,Reward_List);	                    
					               							break;
	                    							}	
													cout<<"\n Step(TRY) Reward IS: "<<MAS_Reward_Test<<"\n";				    					    												    	                                     	                   	                   	                   					   					   					   					   					    
					   								// R1=Reward_List[0];
		  	           								// R2=Reward_List[1];
		  	            							R3=Reward_List[2];
		  	            							
		  	            							NumOfRewarding++;
		  	            		  	          
													//	cout<<"\n R1 In First Step IS: "<<R1<<" \t TS1 IS:"<<TS1;
		  	        								//  cout<<"\n R2 In First Step IS: "<<R2<<" \t TS2 IS:"<<TS2;
		  	            							cout<<"\n R3 In First Step IS: "<<R3<<" \t TS3 IS:"<<TS3;
		  	            							
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Correctness					      *
		                                  *																	  *
		                                  *********************************************************************/
										//*******************Correctness************//
								//	cout<<"\nR1 IS:"<<R1;
								//		cout<<"\nR2 IS:"<<R2;
										cout<<"\nR3 IS:"<<R3;
										
								//		cout<<"\n The Real Reward Of Agent1 IS: "<<RealRewardAg1;
								//		cout<<"\n The Real Reward Of Agent2 IS: "<<RealRewardAg2;
										cout<<"\n The Real Reward Of Agent3 IS: "<<RealRewardAg3;										
										
								//		if(abs(R1-RealRewardAg1)<=CorrThreshold )  CorrAg1++;else CorrAg1--;
								//		if(abs(R2-RealRewardAg2)<=CorrThreshold )  CorrAg2++;else CorrAg2--;
										if(abs(R3-RealRewardAg3)<=CorrThreshold ) CorrAg3++;else CorrAg3--;
										
								//		  cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
								//		  cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;										  		  									
		  					            //   MASCorr=double(CorrAg1+CorrAg2+CorrAg3)/double(3);
		  							  															
										//*******************Correctness************// 					  	            							
		  	            							
		  	            			 //****************ExpertNess****************//
										 /*  if (Sw_Ag1_Select==1) 
										   {
										   	if (R1>0)  NR_Ag1++; else NP_Ag1++;
										   }*/
										   
										 //  if (LearningRate(Q2,2)<9)	
										  //        {
										   /*	       if(Sw_Ag2_Select==1)
													  {
													  	if(R2>0)    NR_Ag2;else NP_Ag2++;
													  }
										   //       }*/
										          
										//   if (LearningRate(Q3,3)<9)	
										//          {
										   	       if (Sw_Ag3_Select==1) 
													  {
													  	if (R3>0)  NR_Ag3;else NP_Ag3++;
													  }
													  
										//          }			  	                   			 
		  	            			 //****************ExpertNess****************//			  	            							
		  	            							
										// Efficeincy	 
										   if(MAS_Reward_Test>0)  Non_Zero_Reward++;
										   if(R1>0)	Non_Zero_R1++;
										   if(R2>0)	Non_Zero_R2++;										   				 
										   if(R3>0)	Non_Zero_R3++;
										// Efficiency   		  	            							
		  	            									  	            							 	  	            												 					
					  								//   UpDate_QTable(Q1,Slice1,Cell1,R1,Next_Slice1);
	                  								//   UpDate_QTable(Q2,Slice2,Cell2,R2,Next_Slice2);
	                     							 if(R3>=TS3)   UpDate_QTable(Q3,Slice3,Cell3,R3,Next_Slice3);
													  
													  	
                    				                 cout<<"\n\n--------------------------------------------------------";
			           				 				 cout<<"\n-                                                      -";
			           				 				 cout<<"\n-                                                      -";
			           				 				 cout<<"\n           End   Of  Try   "<<i+1<<"          -";
			           				 				 cout<<"\n-                                                      -";
			           				 				 cout<<"\n--------------------------------------------------------";
														 
	               	        				/************************************************************************
		                     				 *																	    *
		                     				 *				        End Of First Try In Round	     			    *
		                     				 *																		*
		                     				 ************************************************************************/ 							    						  
						 	    
				  									}//End For
				  								 else
				  									{
                    					   /*************************************************************************
		              						*																 	    *
		              						*			      The Second And Third Try In Round					    *
		              						*																	    *
		              						*************************************************************************/  				  										                      
		             								 /* if (R1>=TS1)
					   									{
	                  									Slice1=Next_Slice1;
	                    								Cell1=Next_Cell1;
					   									}  */
	                 								/* if (R2>=TS2)
	                  									{
	                  									 Slice2=Next_Slice2;
		   	            								 Cell2=Next_Cell2;
					  									}*/
		   	          									if(R3>=TS3)
		   	          									{
		   	          										Slice3=Next_Slice3; 	      			         			          
			            									Cell3=Next_Cell3;
			            									cout<<"\n\n\nThe  "<<i+1<<" th Selected  Slice By Agent 3 IS: "<<Slice3;
		  	                                         		cout<<"\nThe  "<<i+1<<"  th Selected  Cell By Agent 3 IS: "<<Cell3;
			            							 
			            							 if(Slice3==Cell3)  RealRewardAg3=60;
			          							else RealRewardAg3=0;
			            							 
	                                     			 		//Expert Parameter
				                         			 		//if(Slice3==Cell3) NR_Ag3++;  else NP_Ag3++;
				                         			 		//Expert Parameter
													  		Num_Of_Try_Ag3++;
													  		
				                        		Sw_Ag3_Select=1;
				                        		Exp_Try3++;														  		
															  
									   
				                            //Certain Parameter
				                        Sum_Of_Cert+=Certainty(Q3,Episode_Number_Ag1+Episode_Number_Ag2+Episode_Number_Ag3,Slice3,Cell3);
				                        Cert_Counter++;	
										   //Certain Parameter																  			            							 
						 							    }
												  		 else cout<<"\n\t Not Selection With The Agent 3"  ;
			            												            												            												            												            				            		              		              		              
					   										/* if(R1>=TS1)
						 										{
						 										MAS_Reward_Test+=Envionment_Feedback_Agent1(Slice1,Cell1);				        										// MAS_Reward_Episode+=MAS_Reward_Test;	
						  										} */
															/*	if(R2>=TS2)  
																{
						 										MAS_Reward_Test+=Envionment_Feedback_Agent2_Without_Ag1(Slice2,Cell2);
				        										// MAS_Reward_Episode+=MAS_Reward_Test;	
																}*/						
				        								if(R3>=TS3)
				        								{
				         								 MAS_Reward_Test+=Envionment_Feedback_Agent3_Without_Ag1_Ag2(Slice3,Cell3);
				        								// MAS_Reward_Episode+=MAS_Reward_Test;	
														}
					    
					    								ResetListFloat(Reward_List,Num_Of_Agent);
					    								switch(Critic_Type)
					    									{
					    								 	 case 1:Critic_Type_1_Without_Ag1_Ag2(MAS_Reward_Test,Reward_List);
					    	       									break;
					        							  	 case 2:Critic_DB_Without_Ag1_Ag2(MAS_Reward_Test,Reward_List);
					               									break;
					        							 	 case 3:Critic_Type_TS_Agents_priority_Without_Ag1_Ag2(MAS_Reward_Test,Reward_List);
					               									break;
					        							 	 case 4:Critic_Type_TS_Expert_Agents_priority_Without_Ag1_Ag2(MAS_Reward_Test,Reward_List);	                    
						               								break;
	                    									}
		                    							cout<<"\n Learning Rate of Agent 1 IS:"<<LearningRate(Q1,1);
		                    							cout<<"\n Learning Rate of Agent 2 Is:"<<LearningRate(Q2,2);
		                    							cout<<"\n Learning Rate of Agent 3 IS:"<<LearningRate(Q3,3);					    			 		                      	                    					     					     					     
					                 					cout<<"\n\t Reward In Step(Try) IS: "<<MAS_Reward_Test;																	                    
	                    	                    	                                        	                    					     					     
					    								//cout<<"\n Reward IS: "<<MAS_Reward_Test;
					   									// R1=Reward_List[0];
		  	           									// R2=Reward_List[1];
		  	            								R3=Reward_List[2];
		  	            								
		  	            								NumOfRewarding++;
		  	            		  	          
														//	cout<<"\n R1  IS: "<<R1<<" \t TS1 IS:"<<TS1;
		  	         									//   cout<<"\n R2  IS: "<<R2<<" \t TS2 IS:"<<TS2;
		  	            								cout<<"\n R3  IS: "<<R3<<" \t TS3 IS:"<<TS3;
		  	            								
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Correctness					      *
		                                  *																	  *
		                                  *********************************************************************/
										//*******************Correctness************//
								//	cout<<"\nR1 IS:"<<R1;
								//		cout<<"\nR2 IS:"<<R2;
										cout<<"\nR3 IS:"<<R3;
										
								//		cout<<"\n The Real Reward Of Agent1 IS: "<<RealRewardAg1;
								//		cout<<"\n The Real Reward Of Agent2 IS: "<<RealRewardAg2;
										cout<<"\n The Real Reward Of Agent3 IS: "<<RealRewardAg3;										
										
								//		if(abs(R1-RealRewardAg1)<=CorrThreshold )  CorrAg1++;else CorrAg1--;
								//		if(abs(R2-RealRewardAg2)<=CorrThreshold )  CorrAg2++;else CorrAg2--;
										if(abs(R3-RealRewardAg3)<=CorrThreshold ) CorrAg3++;else CorrAg3--;
										
								//		  cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
								//		  cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;										  		  									
		  					           //    MASCorr=double(CorrAg1+CorrAg2+CorrAg3)/double(3);
		  							  															
										//*******************Correctness************// 		  	            								
		  	            								
		  	            			 //****************ExpertNess****************//
										 /*  if (Sw_Ag1_Select==1) 
										   {
										   	if (R1>0)  NR_Ag1++; else NP_Ag1++;
										   }*/
										   
										 //  if (LearningRate(Q2,2)<9)	
										  //        {
										   /*	       if(Sw_Ag2_Select==1)
													  {
													  	if(R2>0)    NR_Ag2;else NP_Ag2++;
													  }
										   //       }*/
										          
										//   if (LearningRate(Q3,3)<9)	
										//          {
										   	       if (Sw_Ag3_Select==1) 
													  {
													  	if (R3>0)  NR_Ag3;else NP_Ag3++;
													  }
													  
										//          }			  	     			 
		  	            			 //****************ExpertNess****************//			  	            								
		  	            								
										// Efficeincy	 
										   if(MAS_Reward_Test>0)  Non_Zero_Reward++;
										   if(R1>0)	Non_Zero_R1++;
										   if(R2>0)	Non_Zero_R2++;										   				 
										   if(R3>0)	Non_Zero_R3++;
										// Efficiency   		  	            								
		  	            
		  	         									//  if(R1>=TS1)  UpDate_QTable(Q1,Slice1,Cell1,R1,Next_Slice1);
	                 									//  if(R2>=TS2)  UpDate_QTable(Q2,Slice2,Cell2,R2,Next_Slice2);
	                   									if(R3>=TS3)  UpDate_QTable(Q3,Slice3,Cell3,R3,Next_Slice3);						 
					  									//getch();
					  					   /***********************************************************************
		               						*																 	  *
		               						*		Select The Next States(Slice) and Actions(Cells)			  *
		               						*																	  *
		               						***********************************************************************/   					  														  			  					  	
				  										if (i==Num_Of_Select-1) 
		               										{
		             										 //Next_Slice1=rand()%(Num_Of_Slices-0);
			 												 //Next_Slice2=rand()%(Num_Of_Slices-0);
			 												 Next_Slice3=rand()%(Num_Of_Slices-0); 	
															}
														else
			           										{
			          /* 	if(R1>=TS1)
			           	{
			           	   Next_Slice1=Select_Slice(List_Of_Slice_Ag1,Num_Of_Slice_Ag1,Bound1,Fault1);
		  	               Next_Cell1=Select_Cell(Possible_Cell_List,Q1,Next_Slice1);	
                           cout<<"\nThe Selected Next Slice By Agent1 IS: "<<Next_Slice1;
		  				   cout<<"\nThe Selected Next Cell By Agent1 IS: "<<Next_Cell1;
			  			   cout<<"\n-------------------\n"	;		  	               
						   }*/
		              /*  if(R2>=TS2)
		                {
		                   Next_Slice2=Select_Slice(List_Of_Slice_Ag2,Num_Of_Slice_Ag2,Bound2,Fault2);
		  	               Next_Cell2=Select_Cell(Possible_Cell_List,Q2,Next_Slice2);	
                           cout<<"\nThe Selected Next Slice By Agent2 IS: "<<Next_Slice2;
		  				   cout<<"\nThe Selected Next Cell By Agent2 IS: "<<Next_Cell2;  	
			  			   cout<<"\n-------------------\n"	;		  	               
						}*/
		  			  											if(R3>=TS3)
		  			  												{
		  			  	    										 Next_Slice3=Select_Slice(List_Of_Slice_Ag3,Num_Of_Slice_Ag3,B3,Fault3);
		  	                										 Next_Cell3=Select_Cell(Possible_Cell_List,Q3,Next_Slice3); 			  			  		
	                        										 cout<<"\nThe Selected Next Slice By Agent3 IS: "<<Next_Slice3;
		  				    										 cout<<"\nThe Selected Next Cell By Agent3 IS: "<<Next_Cell3;	  	            
																	}
		  	            		  		  	            		    		    		                	  									              				
				 	   										}	
				  
		      	       								//cout<<"\n --------------------End The Step: "<<i+1<<"---------------------\n";				        				        
		  	       									}
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		        End Of The Second and Third Try						  *
		                                  *																	  *
		                                  *********************************************************************/ 
										    					    
                       				 				cout<<"\n\n--------------------------------------------------------";
			           				 				cout<<"\n-                                                      -";
			           				 				cout<<"\n-                                                      -";
			           				 				cout<<"\n           End   Of  Try   "<<i+1<<"          -";
			           				 				cout<<"\n-                                                      -";
			           				 				cout<<"\n--------------------------------------------------------";					    
					
					  								MAS_Reward_Episode+=MAS_Reward_Test;
													cout<<"\n\t\tThe MAS Reward In All IS: "<<MAS_Reward_Episode;      				       					   					  					    			    	        		  	        					  					    				  
		      										//getch();
			  									}//END FOR
												  			  												  		            
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      End Of A Round						  *
		                                  *																	  *
		                                  *********************************************************************/ 			  		            
			  		            
	          						 	Print_Text("End Of The Round");
			  						 	cout<<"\n----------The Round--:"<<Round++<<"  Of"<<Episode_Number_Ag3<<" th Episode";
			  						 	Print_Q_Table(Q1,1);
			  						 	Print_Q_Table(Q2,2);
			  						 	Print_Q_Table(Q3,3);
										   			  										                		  		      		      		      		      		      		      		      
		  						}//END WHILE(TRAIN)
		  							  
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		              End Of A 	Train Means That                      *
										  *					The MAS Reward Arrive To The Threshold(40)        *
		                                  *																	  *
		                                  *********************************************************************/ 
		                                   			  				   
		  			 	    	cout<<"\nThe Sum Of MAS: "<<MAS_Reward_Episode;
		  						Print_Text("End Of Episode");
		  						cout<<"\n The Episode Number Of Agent 3 IS: "<<Episode_Number_Ag3;
		  						cout<<"\n The Episode Number For MAS IS: "<<Episode_Number_Ag3+Episode_Number_Ag2+Episode_Number_Ag1;                            	  					
		  						//getch();
		  						
		  						//NR_Ag1_Train+=NR_Ag1;  NP_Ag1_Train+=NP_Ag1;
		  						//NR_Ag2_Train+=NR_Ag2;  NP_Ag2_Train+=NP_Ag2;
		  						NR_Ag3_Train+=NR_Ag3;  NP_Ag3_Train+=NP_Ag3;		  						
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      The Result Of Parameter			      *
		                                  *																	  *
		                                  *********************************************************************/ 
		                                  
										 /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Learning rate					  *
		                                  *																	  *
		                                  *********************************************************************/
                            Print_Text("The Learning Rate of Agents (LR)");										   										  		  
		                    cout<<"\nNum Of Try IS: "<<Num_Of_Try;
		                    cout<<"\n Learning Rate of Agent 1 IS:"<<LearningRate(Q1,1);
		                    cout<<"\n Learning Rate of Agent 2 Is:"<<LearningRate(Q2,2);
		                    cout<<"\n Learning Rate of Agent 3 IS:"<<LearningRate(Q3,3);
		                    
		                    ALR1=(double(LearningRate(Q1,1)))/(double(9));
		                    ALR2=(double(LearningRate(Q2,2)))/(double(9));
		                    ALR3=(double(LearningRate(Q3,3)))/(double(9));		                                  
							GLR=(double(ALR1+ALR2+ALR3))/(double(3));
		                    
		  					Print_Text("Learnig Rate Of Agents And MAS IS:");
		  					cout<<"\nLearning Ratio Of Agent No 1  IS:"<<ALR1;
		  					cout<<"\nLearning Ratio Of Agent No 2  IS:"<<ALR2;
		  					cout<<"\nLearning Ratio Of Agent No 3  IS:"<<ALR3;
		  
		  					cout<<"\n Learning Ratio of MAS IS: "<<GLR;
		  					
							Print_Q_Table(Q1,1);  	
		  					Print_Q_Table(Q2,2);
		  					Print_Q_Table(Q3,3);
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Confidence						  *
		                                  *																	  *
		                                  *********************************************************************/			  										  		  								  								  		  		  		  		  		  		  		  		  		  		  		  		  
		  					Cnf1=Confidence(Q1,1);
		  					Cnf2=Confidence(Q2,2);
		  					Cnf3=Confidence(Q3,3);
		  					MAS_Cnf=(Cnf1+Cnf2+Cnf3)/3;
		  
		  					Print_Text("The Confidence of Agents IS:");
		  					cout<<"\n The Confidence Of Agent 1  IS: "<<Cnf1;
		  					cout<<"\n The Confidence Of Agent 2  IS: "<<Cnf2;
		  					cout<<"\n The Confidence Of Agent 3  IS: "<<Cnf3;
		  					cout<<"\n The Confidence Of MAS  IS: "<<MAS_Cnf;
		  							  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Expert						      *
		                                  *																	  *
		                                  *********************************************************************/
		                                  
		                                  
		  				//	if(LearningRate(Q1,1)<9)  Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1)); else Exp_Ag1=0;
		  				//	if(LearningRate(Q2,2)<9)  Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2)); else Exp_Ag2=0;
		  				//	if(LearningRate(Q3,3)<9)  
						                 Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3)); //else Exp_Ag3=0;
		  					MAS_Exp=(Exp_Ag3)/1;		                                  
		                                  
		                    			//Exp_Ag1=(float(NR_Ag1-NP_Ag1)/float(Num_Of_Try_Ag1));
		  								//Exp_Ag2=(float(NR_Ag2-NP_Ag2)/float(Num_Of_Try_Ag2));
		  								//Exp_Ag3=(float(NR_Ag3-NP_Ag3)/float(Num_Of_Try_Ag3));
		  								
		  								//MAS_Exp=(Exp_Ag3)/1;
		  					
		  								Print_Text("Expert Of Agents;");
		  					//cout<<"\n The ExpertNess Of Agent 1 In EPisode"<<Episode_Number_Ag1<<" IS: "<<Exp_Ag1;
		  					//cout<<"\n The ExpertNess Of Agent 2 In EPisode"<<Episode_Number_Ag1<<" IS: "<<Exp_Ag2;
		  								cout<<"\n The ExpertNess Of Agent 3 In EPisode"<<Episode_Number_Ag3<<" IS: "<<Exp_Ag3;
		  								cout<<"\n------------------------------------------------------------------------\n";
		  					
		  					//cout<<"\n The ExpertNess Of Agent 1 In Sum Of Episode IS:"<<NR_Ag1_Train-NP_Ag1_Train;
		  					//cout<<"\n The ExpertNess Of Agent 2 In Sum Of Episode IS:"<<NR_Ag2_Train-NP_Ag2_Train;
		  								cout<<"\n The ExpertNess Of Agent 3 In Sum Of Episode IS:"<<NR_Ag3_Train-NP_Ag3_Train;		  								
		  								
		  								
		  								//cout<<"\n The ExpertNess Of Agent 1  IS: "<<Exp_Ag1;
		  								//cout<<"\n The ExpertNess Of Agent 2  IS: "<<Exp_Ag2;
		  								//cout<<"\n The ExpertNess Of Agent 3  IS: "<<Exp_Ag3;
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Certainty					      *
		                                  *																	  *
		                                  *********************************************************************/		  					
		  					
		  					Cert_Episode=(Sum_Of_Cert/Cert_Counter);
		  					Print_Text("Certainty");
		  					cout<<"The Certainty Of MAS in Episode  "<<Episode_Number_Ag1+Episode_Number_Ag2+Episode_Number_Ag3<<" Is: "<<Cert_Episode;
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Efiiciency					      *
		                                  *																	  *
		                                  *********************************************************************/		  					
		                                    //cout<<"\n The Efficieny of Agent1 IS: "<<Non_Zero_R1;
		                                    //cout<<"\n The Efficieny of Agent2 IS: "<<Non_Zero_R2;
		                                    cout<<"\n The Efficieny of Agent3 IS: "<<Non_Zero_R3;	                                    		                                    
		                                    
		  									MAS_Effic=double(Non_Zero_R3)/double(1);
		  									cout<<"\n The Effiency of MAS IS: "<<MAS_Effic;	
											  
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Agent Correctness					      *
		                                  *																	  *
		                                  *********************************************************************/
										 // cout<<"\nThe Correctness Of Agent 1  IS:  "<<CorrAg1;
										 // cout<<"\nThe Correctness Of Agent 2  IS:  "<<CorrAg2;
										  cout<<"\nThe Correctness Of Agent 3  IS:  "<<CorrAg3;										  		  									
		  					               MASCorr=(double(CorrAg3)/double(1))/NumOfRewarding;	
											 
										 cout<<"\n The MAS  Correctness IS: "<<MASCorr;	 										  	  					
		  					
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                      Save To Matrix					      *
		                                  *																	  *
		                                  *********************************************************************/			  								
							
										cout<<"\n+++++++++The End Of Agent3  Episode :  "<<Episode_Number_Ag3;							
                            			cout<<"\n The Episode Number IS: "<<Episode_Number_Ag3+Episode_Number_Ag2+Episode_Number_Ag1;	
                            			
							if(R_Index_Res<=Res_Rec)
							{
							Result[R_Index_Res][0]=R_Index_Res+1;
							Result[R_Index_Res][1]=GLR;
							Result[R_Index_Res][2]=MAS_Cnf;
							Result[R_Index_Res][3]=MAS_Exp;
							Result[R_Index_Res][4]=Cert_Episode;
							Result[R_Index_Res][5]=MAS_Effic;
							Result[R_Index_Res][6]=MASCorr;														
							R_Index_Res++;	
							}
							else
							{
			                 switch(Critic_Type)
					                 {
					    	          case 1:Print_Text("TS Only Method");
					    	                 break;
					                 // case 2:Print_Text("Dr Beigy Method");
					                 //        break;
					                  case 2:Print_Text("TS+MAS Priority  Method");
					                         break;
					                  case 3:Print_Text("TS+Expert Agent  Priority  Method");	                    
					                         break;
	                                 }
	                                 Print_Result(Result);
									 SaveToFile(Result)	;	
							}
							
								  
		   //getch();         					         	  							  										   		  	 		  		  
		   								//getch();         	
		           					                  	                   	                  			   					  					   
							   }// End Of Any Episode
					        			
		cout<<"\n The Episode Number IS: "<<Episode_Number_Ag3+Episode_Number_Ag2+Episode_Number_Ag1;
		cout<<"\n Learning Rate:"<<LearningRate(Q1,1);
		cout<<"\n Learning Rate:"<<LearningRate(Q2,2);
		cout<<"\n Learning Rate:"<<LearningRate(Q3,3);
		//PrintNormalizedQT(Q3,3);	
		
						                 switch(Critic_Type)
					                 {
					    	          case 1:Print_Text("PTST Method");
					    	                 break;
					                 // case 2:Print_Text("Dr Beigy Method");
					                 //        break;
					                  case 2:Print_Text("T-MAS Method");
					                         break;
					                  case 3:Print_Text("T-KAg Method");	                    
					                         break;
	                                 }
	                                 Print_Result(Result);
									 SaveToFile(Result)	;	
		
		getch();
//}
					                     /*********************************************************************
		                                  *																 	  *
		                                  *		                          The End 						  	  *
		                                  *																	  *
		                                  *********************************************************************/
		  	        
			        
}
		  	
		  	
		  	
			
		              
						   	
  


