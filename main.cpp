#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <utility>
#include <map>
#include <string.h>

// CHANGE THE BRANCH PREDICTION TYPE HERE
#define BRANCH_PREDICTION_TYPE 1

#define ADDER_UNIT 3
#define MULTIPLIER_UNIT 2
#define CONSTANT_UNIT 1
#define NUM_REGISTERS 16
#define MEMORY_SIZE 100000
#define ISSUE_NUMER 1
using namespace std;

// The Memory structure which does the loading from memory and stroing to memory
struct Memory{
	vector<float> MEMORY;
	Memory(){
		MEMORY.resize(MEMORY_SIZE);
		for (int i = 0; i < MEMORY.size(); ++i)
		{
			MEMORY[i]=10;
		}
	}

	// Save in memory
	void save(int f,float val){
		int i = (int)f;
		if(i>=MEMORY_SIZE)
			return;
		else
			MEMORY[i]=val;
	}

	// Load from memory
	float load(int i){
		if(i>=MEMORY_SIZE)
			return 0.0;
		else
			return MEMORY[i];
	}

} MEMORY;

// The ROB structure 
struct ROB{
	int Register;	// This stores the register value
	float val;		// value of ROB
	int hasVal;		// if it has a value or not
	ROB(){
		hasVal = 0;
		val = 0;
	}
};

// Array of ROB and total Number of defined ROB's
vector<ROB> rob;
int numROB;

// Register Structure
struct Register{
	string name;		// Name of register
	float val;			// value of register
	int rob_no;			// Rob no from which it has to fetch value
	int hasVal;			// if it has a vlue of not

	Register(){
		val = 0.0;
		name = "";
		hasVal = 1;
	}

	Register(const Register &a){
		name = a.name;
		val = a.val;
		rob_no = a.rob_no;
		hasVal = a.hasVal;
	}
};


// Array of registers 
vector<Register> Reg(NUM_REGISTERS+1);
// This is used to restore the previous values of register
vector<Register> PrevReg(NUM_REGISTERS+1);

// To break the string by delimeter
vector<string> explode(string const & s, char delim)
{
    vector<string> result;
    istringstream iss(s);

    for (string token; getline(iss, token, delim); )
    {
        result.push_back(token);
    }

    return result;
}

// The Instruction Structure
struct INSTR
{
	string ins,reg;		// ins is the instruction and reg is the parameter
	string RegDest , RegSrc1, RegSrc2;		// The three registers
	int addr  ;			// Address
	float FVal;			// Value of Src2
	int UnitType; // 1 = FPADDER , 2 = FPMULTIPLIER , 3 = INTEGERUNITS , 4 = none (Branch or halt)
	int RegSrc2Type; // 1 = Reg , 2 = float , 3 = addr

	int cycleDone;		// no of cycles done by this instruction

	int r1,r2,r3;		// 3 ROB's for this instr

	int FalseNode;		// A false insturction has to be put into the regs
	int Instr_no;		// Instrucion no -> used while flushing

	// COnstuctor
	INSTR(int f = 0){
		addr = -1;
		FVal = -1;
		RegSrc2 = "R16";
		RegSrc1 = "R16";
		RegDest = "R16";
		cycleDone = 0;
		FalseNode = f;
	}

	// Sets value of Src2
	void setRegSrc2(string s){
		stringstream sstr;

		if(s[0] == 'R'){
			RegSrc2 = s; RegSrc2Type = 1;
		}
		else if(s[0] != '#'){
			RegSrc2Type = 3;
			sstr<<s;
			sstr>>addr;
			FVal = addr;
		}
		else{
			RegSrc2Type = 2;
			sstr<<s.substr(1);
			sstr>>FVal;
			addr = FVal;
		}
	}

	// Parse the input to fill all the entries
	void ParseReg(){
		vector<string> reglist = explode(reg,',');
		if(ins == "HALT");
		else if(ins == "FPADD" || ins == "FPSUB" || ins=="FPMULT" || ins=="FPDIV" 
			|| ins=="ADD" || ins == "SUB"){
			if(reglist.size()!=3){
				cout<<"Error in Input File\n";
				exit(0);
			}
			else
			{
				RegDest = reglist[0];
				RegSrc1 = reglist[1];
				setRegSrc2(reglist[2]);
			}
		}
		else if(ins=="LOAD" || ins=="MOV"){
			if(reglist.size()!=2){
				cout<<"Error in Input File\n";
				exit(0);
			}
			else
			{
				RegDest = reglist[0];
				if(reglist[1][0]=='#')
					ins = "MOV";
				setRegSrc2(reglist[1]);
			}
		}
		else if(ins=="BR"){
			if(reglist.size()!=1){
				cout<<"Error in Input File\n";
				exit(0);
			}
			else
				setRegSrc2(reglist[0]);
		}
		else if(ins=="STR" || ins=="BGT" || ins=="BLT" || ins=="BGE" || ins=="BLE" || ins=="BZ" || ins=="BNEZ"){
			if(reglist.size()!=2){
				cout<<"Error in Input File\n";
				exit(0);
			}
			else{
				RegSrc1 = reglist[0];
				setRegSrc2(reglist[1]);
			}
		}
		else{
			cout<<"Invalid Command :" << ins<< endl;
			exit(0);
		}

		if(ins == "FPADD" || ins == "FPSUB" || ins=="ADD" || ins == "SUB"){
			UnitType = 1;
		}
		else if(ins=="FPMULT" || ins=="FPDIV" ){
			UnitType = 2;
		}
		else if(ins=="LOAD" || ins=="MOV"){
			UnitType = 3;
		}
		else{
			UnitType = 4;
		}
	}

	void Print(){
		cout<<ins<<" : "<<RegDest<<" : "<<RegSrc1<<" : "<<RegSrc2<< " : "<<addr<<" : "<<FVal<<" : "<<UnitType<<endl;
	}
};
// A false instruction
INSTR FalseINSTR(-1);

bool operator==(INSTR a , INSTR b){
	return a.FalseNode == b.FalseNode;
}

bool operator!=(INSTR a , INSTR b){
	return a.FalseNode != b.FalseNode;
}

// The structure for Branch Tager Buffer
struct BranchTagetBuffer{
	map<string,pair<int,int> > btf;

	// Returns the branch prediction
	pair<int,int> TakenNotTaken(string s){
		map<string,pair<int,int> >::iterator it;
		it = btf.find(s);
		if(it == btf.end())
			return pair<int,int>(0,0);

		return btf[s];
	}

	// Update the buffer
	void UpdateTakenNotTaken(string s , int PC , int tkn){
		pair<int,int> a(tkn,PC);
		btf[s] = a;	
	}

	// Loads from file
	void loadFromFile(char* filename){
		string s(filename),s1;
		int n1,n2;
		s+=".initial_history";
		fstream infile(s.c_str());
		if(infile.fail()){
			return;
		}
		while(infile>>s1)
		{
			infile>>n1>>n2;
			UpdateTakenNotTaken(s1,n2,n1);
		}
	}

	// Saves buffer to file
	void saveToFile(char* filename){
		string s(filename);
		s+=".initial_history";
		fstream outf;
		outf.open(s.c_str(),ofstream::out);
		map<string,pair<int,int> >::iterator it = btf.begin();
		for (;it!=btf.end(); ++it)
		{
			outf<<it->first<<" "<<it->second.first<<" "<<it->second.second<<endl;
		}
	}

} BTF;

// Used only when the Prediction type is 2
int prediction_state = 0;
int prediction_count = 0;

// The main branch prediction fucntion
// stage = 0 means IF , stage = 1 means EXEC
pair<int,int> BrancPrediction(int type,string btype,int stage,int actual_taken,int PC=0){
	// type = 1 means using branch target buffer
	if(type==1){
		if(stage == 0){
			return BTF.TakenNotTaken(btype);
		}
		else{
			BTF.UpdateTakenNotTaken(btype,PC,actual_taken);
		}
	}
	// If its 2 bit branch predictor
	else if(type==2){
		if(stage==0){
			return pair<int,int>(prediction_state,0);
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
			pair<int,int> a(1,0);
			return a;
		}
	}
}

// GLOBALS MODULES OF THE MACHINE
vector<INSTR> FPAdder(ADDER_UNIT,FalseINSTR) , FPMultiplier(MULTIPLIER_UNIT,FalseINSTR) , IntegerUnit(CONSTANT_UNIT,FalseINSTR);
int num_FPAdder = 0 , num_FPMultiplier = 0 , Num_IU = 0;


// Numlines = num lines in code , Curline is the PC , Prevline is previously issued line
int numlines , Curline = 0 ,Prevline = -1; 
// list of instructions
vector<INSTR> ins;

// Extract number from register
int NUmFromReg(string s){
	if(s.size()<2 || s[0]!='R')
	{
		cout<<"Invalid Register :"<<s<<endl;
		exit(0);
	}
	s=s.substr(1);
	stringstream ss;
	ss<<s;
	int n;
	ss>>n;
	return n;
};

// Find cost of an instruction
int Cost(string S){
	if(S=="FPADD") return 3;
	if(S=="FPSUB") return 3;
	if(S=="FPMULT") return 5;
	if(S=="FPDIV") return 8;
	if(S=="STR") return 3;
	return 1;
}

// Create ROB's for the instructions
void CreateROBs(){
	//ins[Curline].Print();
	int r_no = NUmFromReg(ins[Curline].RegDest);
	int r_no1 = NUmFromReg(ins[Curline].RegSrc1);
	int r_no2 = NUmFromReg(ins[Curline].RegSrc2);

	// Store the Src1 and Src into ROB's
	ROB temp,temp1,temp2;
	temp1.Register = r_no1;
	if(r_no1==16){}
	else if(Reg[r_no1].hasVal){
		temp1.val = Reg[r_no1].val;
		temp1.hasVal = 1;
		ins[Curline].r2 = numROB;
		rob.push_back(temp1);
		numROB++;
	}
	else{
		ins[Curline].r2 = Reg[r_no1].rob_no;
	}
	

	temp2.Register = r_no2;
	if(ins[Curline].RegSrc2Type>=2){
		temp2.hasVal = 1;
		temp2.val = ins[Curline].FVal;
		ins[Curline].r3 = numROB;
		rob.push_back(temp2);
		numROB++;
	}
	else if(Reg[r_no2].hasVal){
		temp2.hasVal = 1;
		temp2.val = Reg[r_no2].val;
		ins[Curline].r3 = numROB;
		rob.push_back(temp2);
		numROB++;
	}
	else{
		ins[Curline].r3 = Reg[r_no2].rob_no;
	}
	
	// If insturction is not store and branch  store Dest in Rob
	if(ins[Curline].UnitType==4){
		return;
	}

	ins[Curline].r1 = numROB;
	Reg[r_no].rob_no = numROB;
	Reg[r_no].hasVal = 0;
	temp.Register = r_no;
	rob.push_back(temp);
	numROB++;
}

// Process Other operators to return proper values
double process(string s,float a , float b){
	if(s=="FPADD" || s=="ADD"){
		return a+b;
	}
	else if(s=="FPSUB" || s=="SUB")
	{
		return a-b;
	}
	else if(s=="FPMULT" || s=="MULT")
	{
		return a*b;
	}
	else if(s=="FPDIV" || s=="DIV"){
		
		return a/b;
	}
	else if(s=="MOV"){
		return b;
	}
	else if(s=="LOAD"){
		return MEMORY.load(b);
	}
}

//Process a branch instruction
bool ProcessBranch(string s,int b){
	if(s=="BGT"){
		return b>0;
	}
	else if(s=="BLT"){
		return b<0;
	}
	else if(s=="BGE"){
		return b>=0;
	}
	else if(s=="BLE"){
		return b>=0;
	}
	else if(s=="BZ"){
		return b==0;
	}
	else if(s=="BNEZ"){
		return b!=0;
	}
}

bool penalTy = 0;			// Penalty is used in branch misprediction , added to total number of cycles
bool is_branch=false;		// is a branch instruction is seen
bool is_predicted = false;	// if a branch prediction is done 
bool is_taken=true;			// If a branch is taken or not in prediction
int numROB_atb = -1; 		// ROB count at branch prediction_count
int totalBranch=0;			// Total number of branch instructions
int BranchPre=0;			// Total no of correct branch prediction
int PrevCurline = 0;		// The PC value at time of branch instruction

// Main Function to process the Queue
bool ProcessQueue(){
	// Process instructions in the adder unit
	for (int i = 0; i < ADDER_UNIT; ++i)
	{
		if(FPAdder[i]!=FalseINSTR){
			int C = Cost(FPAdder[i].ins);
			// If no of cycles is less increase cycle count
			if(C > FPAdder[i].cycleDone) FPAdder[i].cycleDone++;
			// Else if the dependent ROB's are available , store in new ROB
			else{
				if(rob[FPAdder[i].r2].hasVal && rob[FPAdder[i].r3].hasVal){
					
					rob[FPAdder[i].r1].val = process(FPAdder[i].ins,rob[FPAdder[i].r2].val , rob[FPAdder[i].r3].val);
					rob[FPAdder[i].r1].hasVal = 1;
					int R = rob[FPAdder[i].r1].Register;
					
					if( Reg[R].rob_no == FPAdder[i].r1 && R!=16 &&( numROB_atb<0 ||  IntegerUnit[i].Instr_no<=numROB_atb)){
						Reg[R].val = rob[FPAdder[i].r1].val;
						Reg[R].hasVal = 1;
					}
					FPAdder[i].cycleDone = 0;
					FPAdder[i] = FalseINSTR;	
				}
			}
		}
	}

	// Process instruction in Multiplier Unit
	for (int i = 0; i < MULTIPLIER_UNIT; ++i)
	{
		if(FPMultiplier[i]!=FalseINSTR){
			int C = Cost(FPMultiplier[i].ins);
			if(C > FPMultiplier[i].cycleDone) FPMultiplier[i].cycleDone++;
			else{
				if(rob[FPMultiplier[i].r2].hasVal && rob[FPMultiplier[i].r3].hasVal){
					
					rob[FPMultiplier[i].r1].val = process(FPMultiplier[i].ins,rob[FPMultiplier[i].r2].val , rob[FPMultiplier[i].r3].val);
					rob[FPMultiplier[i].r1].hasVal = 1;
					int R = rob[FPMultiplier[i].r1].Register;
					
					if( Reg[R].rob_no == FPMultiplier[i].r1 && R!=16 &&( numROB_atb<0 ||  IntegerUnit[i].Instr_no<=numROB_atb)){
						Reg[R].val = rob[FPMultiplier[i].r1].val;
						Reg[R].hasVal = 1;
					}
					FPMultiplier[i].cycleDone = 0;
					FPMultiplier[i] = FalseINSTR;	
				}
			}
		}
	}
	// Process instruction in the Integer Unit
	for (int i = 0; i < CONSTANT_UNIT; ++i)
	{
		if(IntegerUnit[i]!=FalseINSTR)
		{
			int C = Cost(IntegerUnit[i].ins);
			if(C > IntegerUnit[i].cycleDone){
				IntegerUnit[i].cycleDone++;
				if(is_branch && !is_predicted && IntegerUnit[i].ins!="BR"){
					if(!is_predicted && ((BRANCH_PREDICTION_TYPE!=1 && rob[IntegerUnit[i].r3].hasVal)||(BRANCH_PREDICTION_TYPE==1))){
						is_predicted = true;
						for (int j = 0; j < Reg.size(); ++j)
						{
							PrevReg[j] = Reg[j];
						}
						numROB_atb = numROB;
						pair<int,int> predict;
						predict = BrancPrediction(BRANCH_PREDICTION_TYPE,IntegerUnit[i].ins,0,0,0);
						int newPC;
						if(BRANCH_PREDICTION_TYPE==1){
							newPC = predict.second;
						}
						else{
							newPC = rob[IntegerUnit[i].r3].val;
						}
						if(predict.first != 0){
							Curline += newPC;
							is_taken = true;
						}
						else{
							is_taken = false;
						}
					}
				}
			}
			else{
				// Remove the halt instruction
				if(IntegerUnit[i].ins=="HALT"){
					IntegerUnit[i] = FalseINSTR;
				}
				// Process move and load instruction
				else if(IntegerUnit[i].ins=="MOV" || IntegerUnit[i].ins=="LOAD"){
					if(rob[IntegerUnit[i].r3].hasVal){
						rob[IntegerUnit[i].r1].val = process(IntegerUnit[i].ins,0.0,rob[IntegerUnit[i].r3].val);
						rob[IntegerUnit[i].r1].hasVal = 1;
						int R = rob[IntegerUnit[i].r1].Register;
						if( Reg[R].rob_no == IntegerUnit[i].r1 && R!=16&&( numROB_atb<0 ||  IntegerUnit[i].Instr_no<=numROB_atb)){
							Reg[R].val = rob[IntegerUnit[i].r1].val;
							Reg[R].hasVal = 1;
						}

						IntegerUnit[i].cycleDone = 0;
						IntegerUnit[i] = FalseINSTR;
					}
				}
				// Process a store
				else if(IntegerUnit[i].ins=="STR"){
					if(rob[IntegerUnit[i].r3].hasVal && rob[IntegerUnit[i].r2].hasVal){
						MEMORY.save(rob[IntegerUnit[i].r3].val,rob[IntegerUnit[i].r2].val);
						IntegerUnit[i].cycleDone = 0;
						IntegerUnit[i] = FalseINSTR;
					}
				}
				// Simple branch
				else if(IntegerUnit[i].ins=="BR"){
					if(rob[IntegerUnit[i].r3].hasVal){
						Curline+= rob[IntegerUnit[i].r3].val;
						IntegerUnit[i].cycleDone = 0;
						IntegerUnit[i] = FalseINSTR;
						is_branch = false;
					}
				}
				// All other branches
				else{
					// If ready to execute
					if(rob[IntegerUnit[i].r3].hasVal && rob[IntegerUnit[i].r2].hasVal){
						Prevline = PrevCurline;
						bool actual_taken = false;
						bool b = ProcessBranch(IntegerUnit[i].ins,rob[IntegerUnit[i].r2].val);
						int PC = rob[IntegerUnit[i].r3].val;
						if(is_predicted)
							totalBranch++;
						// The actual branch prediction
						if(b){
							actual_taken = true;
							PC = rob[IntegerUnit[i].r3].val;
							Curline = PrevCurline + rob[IntegerUnit[i].r3].val ;

						}
						else{
							Curline = PrevCurline +1;
						}
						// here is a miss prediction
						if(is_taken!=actual_taken && is_predicted){
							penalTy += 2;
							// FLUSH ALL INSTRUCTION IN CURRET UNITS
							for (int j = 0; j < ADDER_UNIT; ++j)
							{
								if(FPAdder[j]==FalseINSTR){
									if(FPAdder[j].Instr_no>=numROB_atb)
										FPAdder[j] = FalseINSTR;
								}
							}
							for (int j = 0; j < MULTIPLIER_UNIT; ++j)
							{
								if(FPMultiplier[j]==FalseINSTR){
									if(FPMultiplier[j].Instr_no>=numROB_atb)
										FPMultiplier[j] = FalseINSTR;
								}
							}
							for (int j = 0; j < CONSTANT_UNIT; ++j)
							{
								if(IntegerUnit[j]==FalseINSTR){
									if(IntegerUnit[j].Instr_no>=numROB_atb)
										IntegerUnit[j] = FalseINSTR;
								}
							}
							// RESTORE ROB's
							int len = rob.size();
							for (int j = numROB_atb; j < numROB; ++j)
							{
								//rob.pop_back();
							}
							//numROB = numROB_atb;
							for (int j = 0; j < Reg.size(); ++j)
							{
								Reg[j].name = PrevReg[j].name;
							}
							is_predicted = false;

						}
						else if(is_predicted){
							// Correct branch prediction
							BranchPre++;
							//fill all ROBs into register
							for (int j = numROB_atb; j < numROB; ++j)
							{
								Reg[rob[j].Register].val = rob[j].val;
								Reg[rob[j].Register].hasVal = rob[j].hasVal;
							}
						}
						// update buffer
						BrancPrediction(BRANCH_PREDICTION_TYPE,IntegerUnit[i].ins,1,actual_taken,PC);

						IntegerUnit[i].cycleDone = 0;
						IntegerUnit[i] = FalseINSTR;
						is_branch = false;
					}
					// else do the prediction
					
				}

				
			}
		}
	}
	
	
	// The clear all instruction before exiting
	if(Curline >= numlines){
		bool test = false;
		for (int i = 0; i < ADDER_UNIT; ++i)
			if(FPAdder[i]!=FalseINSTR) {test = true; }

		for (int i = 0; i < MULTIPLIER_UNIT; ++i)
			if(FPMultiplier[i]!=FalseINSTR) {test = true;}

		for (int i = 0; i < CONSTANT_UNIT; ++i)
			if(IntegerUnit[i]!=FalseINSTR) {test = true;}

		return test;
	}
	
	if(Prevline == Curline){
		return true;
	}


	// Issie stage
	INSTR next;
	bool flag = false;


	// Issue multiple instruciton
	for (int it = 0; it < ISSUE_NUMER; ++it)
	{
		next = ins[Curline];
		if(next.UnitType == 1){
			for (int i = 0; i < ADDER_UNIT; ++i)
			{
				if(FPAdder[i]==FalseINSTR){
					CreateROBs();
					FPAdder[i] = ins[Curline];
					FPAdder[i].Instr_no = numROB;
					flag = true;

					break;
				}
			}
		}

		else if(next.UnitType == 2){
			for (int i = 0; i < MULTIPLIER_UNIT; ++i)
			{
				if(FPMultiplier[i]==FalseINSTR){
					CreateROBs();


					FPMultiplier[i] = ins[Curline];
					FPMultiplier[i].Instr_no = numROB;
					flag = true;
					break;
				}
			}
		}

		else //if(next.UnitType == 3)
		{
			for (int i = 0; i < CONSTANT_UNIT; ++i)
			{
				if(IntegerUnit[i]==FalseINSTR){
					if(ins[Curline].ins[0]=='B') is_branch = true;
					CreateROBs();


					if(ins[Curline].ins[0]=='B'){
						PrevCurline = Curline+1;
					}

					IntegerUnit[i].Instr_no = numROB;
					IntegerUnit[i] = ins[Curline];
					flag = true;
					break;
				}
			}
		}

		if(flag){
			Prevline = Curline;
			Curline++;
		}
	}

	
	return true;
};

int main(int argc, char  *argv[])
{
	if (argc < 2)
	{
		cerr<<"Input File required \n";
		exit(0);
	}

	fstream inputStream(argv[1]);

	// Initialize Registers

	BTF.loadFromFile(argv[1]);
	for (int i = 0; i < NUM_REGISTERS; ++i)
	{
		stringstream ss;
		ss<<"R"<<i;
		Reg[i].name=ss.str();
	}
	//////////////////////

	if(inputStream.fail()){
		cerr<<"Invalid Input File\n";
		exit(0);
	}

	string garbage,s;

	// Parse input and get the inputStream
	while(inputStream>>s){
		if(s.size()>1 && s[0]=='-' && s[1]=='-'){
			getline(inputStream,garbage);
			continue;
		}
		stringstream ss;
		ss<<s;
		ss>>numlines;
		break;
	}

	ins.resize(numlines+2);

	// get all teh insturctions
	for(int i=0;i<numlines && inputStream>>ins[i].ins;i++){
		if(ins[i].ins.size()>1 && ins[i].ins[0]=='-' && ins[i].ins[1]=='-'){
			getline(inputStream,garbage);
			i--;
		}
		else{
			inputStream>>ins[i].reg;
			ins[i].ParseReg();
		}
	}
	
	int memsize=0;;
	// Get the memory size
	while(inputStream>>s){
		if(s.size()>1 && s[0]=='-' && s[1]=='-'){
			getline(inputStream,garbage);
			continue;
		}
		stringstream ss;
		ss<<s;
		ss>>memsize;
		break;
	}
	string s1;
	// Get all memory contents
	for (int i = 0; i < memsize && inputStream>>s ; ++i)
	{
		if(s.size()>1 && s[0]=='-' && s[1]=='-'){
			getline(inputStream,garbage);
			--i;
		}
		else{
			int mem;
			float val;
			inputStream>>s1;
			s = s.substr(1);
			s = s.substr(0,s.size()-1);

			s1 = s1.substr(1);
			s1 = s1.substr(0,s1.size()-1);

			stringstream ss,ss1;
			ss<<s;
			ss>>mem;
			ss1<<s1;
			ss1>>val;
			MEMORY.save(mem,val);
		}
	}

	int numCycles = 0;
	// Process all the queues
	while(ProcessQueue()){
		numCycles ++ ;
	}
	// And branch miss penalty
	numCycles += penalTy;
	/*
	cout<<"Register Status \n";
	for (int i = 0; i < Reg.size()-1; ++i)
	{
		cout<<"R"<<i<<" : "<<Reg[i].val<<endl;
	}*/
	cout<<"Total No of Cycles = "<<numCycles<<endl;
	if(totalBranch==0)
		cout<<"Misprediction Rate = 0"<<endl;
	else
		cout<<"Misprediction Rate = "<<((double)(totalBranch-BranchPre)*100.0)/(double)totalBranch<<endl;
	
	
	BTF.saveToFile(argv[1]);
	return 0;
}