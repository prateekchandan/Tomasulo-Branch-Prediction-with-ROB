#include <string.h>
// The structure for Branch Tager Buffer
struct BranchTagetBuffer{
	char command[100][100];
	int PCC[100],TKN[100];
	int num;

} BTF;



	// Returns the branch prediction
void TakenNotTaken(char* s,int* PC , int* tkn,BranchTagetBuffer *b){
	int i;
	for (i = 0; i < b->num; ++i)
	{
		if(!strcmp(s,command[i])){
			*tkn = b->TKN[i];
			*PCC = b->PCC[i];
			return;
		}
	}
}

	// Update the buffer
void UpdateTakenNotTaken(char* s , int* PC , int* tkn,BranchTagetBuffer *b){
	int i;
	for (i = 0; i < b->num; ++i)
	{
		if(!strcmp(s,command[i])){
			b->TKN[i] = *tkn;
			b->PCC[i] = *PC;
			return;
		}
	}

	strcpy(command[b->num],s);
	b->TKN[b->num] = b->tkn;
	b->PCC[b->num] = b->PC;
	b->num++;
}

// Used only when the Prediction type is 2
int prediction_state = 0;
int prediction_count = 0;

// The main branch prediction fucntion
// stage = 0 means IF , stage = 1 means EXEC
// Returning alues by call by reference
void BrancPrediction(int type,char* btype,int stage,int actual_taken,int *PC,int *taken){
	// type = 1 means using branch target buffer
	if(type==1){
		if(stage == 0){
			TakenNotTaken(btype,PC,taken,&BTF);
		}
		else{
			UpdateTakenNotTaken(btype,PC,actual_taken,&BTF);
		}
	}
	// If its 2 bit branch predictor
	else if(type==2){
		if(stage==0){
			*taken = prediction_state;
			*PC = 0;
		}
		else{
			if (actual_taken= prediction_state) 
		  	{
		    	if(prediction_count==0 && prediction_state==0)
		       	{
		       		prediction_count=0 ;prediction_state=0;
		       	}
		 		else if(prediction_count==1 && prediction_state==0)
		   		{
		   			prediction_count=prediction_count-1; prediction_state=0;
		   		}
				else if(prediction_count==2 && prediction_state==1)
		   		{
		   			prediction_count=2; prediction_state=1;
		   		}
		  		else if(prediction_count==1 && prediction_state==1)
		      	{
		      		prediction_count= prediction_count+1; prediction_state=1;
		      	}
		    }
			else if (actual_taken!= prediction_state)
		  	{
		     	if(prediction_count==0 && prediction_state==0)
		       	{
		       		prediction_count=prediction_count+1 ;prediction_state=0;
		       	}
		     	else if(prediction_count==1 && prediction_state==0)
		       	{
		       		prediction_count=prediction_count+1; prediction_state=1;
		       	}
		     	else if(prediction_count==2 && prediction_state==0)
		       	{
		       		prediction_count=prediction_count-1; prediction_state=1;
		       	}
		    	else if(prediction_count==1 && prediction_state==1)
		       	{
		       		prediction_count= prediction_count-1; prediction_state=0;
		       	}
		  	}
		}
	}
	// static predictor
	else if(type==3){
		if(stage==0){
			*taken = 1;
			*PC = 0;
		}
	}
}
