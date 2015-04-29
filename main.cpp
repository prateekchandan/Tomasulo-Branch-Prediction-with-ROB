#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <utility>
#include <map>

#define ADDER_UNIT 3
#define MULTIPLIER_UNIT 2
#define CONSTANT_UNIT 1
#define NUM_REGISTERS 16
#define MEMORY_SIZE 100000
using namespace std;

struct Memory{
	vector<float> MEMORY;
	Memory(){
		MEMORY.resize(MEMORY_SIZE);
	}

	void save(int f,float val){
		int i = (int)f;
		if(i>=MEMORY_SIZE)
			return;
		else
			MEMORY[i]=val;
	}

	float load(int i){
		if(i>=MEMORY_SIZE)
			return 0.0;
		else
			return MEMORY[i];
	}

} MEMORY;

struct ROB{
	int Register;
	float val;
	int hasVal;
	ROB(){
		hasVal = 0;
		val = 0;
	}
};

vector<ROB> rob;
int numROB;

struct Register{
	string name;
	float val;
	int rob_no;
	int hasVal;

	Register(){
		val = 0.0;
		name = "";
		hasVal = 1;
	}
};

vector<Register> Reg(NUM_REGISTERS);

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

struct INSTR
{
	string ins,reg;
	string RegDest , RegSrc1, RegSrc2;
	int addr  ;
	float FVal;
	int UnitType; // 1 = FPADDER , 2 = FPMULTIPLIER , 3 = INTEGERUNITS , 4 = none (Branch or halt)
	int RegSrc2Type; // 1 = Reg , 2 = float , 3 = addr

	int cycleDone;

	int r1,r2,r3;

	INSTR(){
		addr = -1;
		FVal = -1;
		RegSrc2 = "R16";
		RegSrc1 = "R16";
		RegDest = "R16";
		cycleDone = 0;
	}

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

vector<int> FPAdder(ADDER_UNIT,-1) , FPMultiplier(MULTIPLIER_UNIT,-1) , IntegerUnit(CONSTANT_UNIT,-1);
int num_FPAdder = 0 , num_FPMultiplier = 0 , Num_IU = 0;


int numlines , Curline = 0 ,Prevline = -1; 
vector<INSTR> ins;

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

int Cost(string S){
	if(S=="FPADD") return 3;
	if(S=="FPSUB") return 3;
	if(S=="FPMULT") return 5;
	if(S=="FPDIV") return 8;
	if(S=="STR") return 3;
	return 1;
}

void CreateROBs(){
	//ins[Curline].Print();
	int r_no = NUmFromReg(ins[Curline].RegDest);
	int r_no1 = NUmFromReg(ins[Curline].RegSrc1);
	int r_no2 = NUmFromReg(ins[Curline].RegSrc2);

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
		cout<<b<<" "<<MEMORY.load(b)<<endl;
		return MEMORY.load(b);
	}
}

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

bool is_branch=false;

bool ProcessQueue(){
	cout<<Curline<<" "<<Reg[0].val<<" , "<<Reg[1].val<<endl;	

	for (int i = 0; i < ADDER_UNIT; ++i)
	{
		if(FPAdder[i]!=-1){
			int C = Cost(ins[FPAdder[i]].ins);
			if(C > ins[FPAdder[i]].cycleDone) ins[FPAdder[i]].cycleDone++;
			else{
				if(rob[ins[FPAdder[i]].r2].hasVal && rob[ins[FPAdder[i]].r3].hasVal){
					
					rob[ins[FPAdder[i]].r1].val = process(ins[FPAdder[i]].ins,rob[ins[FPAdder[i]].r2].val , rob[ins[FPAdder[i]].r3].val);
					rob[ins[FPAdder[i]].r1].hasVal = 1;
					int R = rob[ins[FPAdder[i]].r1].Register;
					
					if( Reg[R].rob_no == ins[FPAdder[i]].r1 && R!=16){
						Reg[R].val = rob[ins[FPAdder[i]].r1].val;
						Reg[R].hasVal = 1;
					}
					ins[FPAdder[i]].cycleDone = 0;
					FPAdder[i] = -1;	
				}
			}
		}
	}

	for (int i = 0; i < MULTIPLIER_UNIT; ++i)
	{
		if(FPMultiplier[i]!=-1){
			int C = Cost(ins[FPMultiplier[i]].ins);
			if(C > ins[FPMultiplier[i]].cycleDone) ins[FPMultiplier[i]].cycleDone++;
			else{
				if(rob[ins[FPMultiplier[i]].r2].hasVal && rob[ins[FPMultiplier[i]].r3].hasVal){
					
					rob[ins[FPMultiplier[i]].r1].val = process(ins[FPMultiplier[i]].ins,rob[ins[FPMultiplier[i]].r2].val , rob[ins[FPMultiplier[i]].r3].val);
					rob[ins[FPMultiplier[i]].r1].hasVal = 1;
					int R = rob[ins[FPMultiplier[i]].r1].Register;
					
					if( Reg[R].rob_no == ins[FPMultiplier[i]].r1 && R!=16){
						Reg[R].val = rob[ins[FPMultiplier[i]].r1].val;
						Reg[R].hasVal = 1;
					}
					ins[FPMultiplier[i]].cycleDone = 0;
					FPMultiplier[i] = -1;	
				}
			}
		}
	}
	for (int i = 0; i < CONSTANT_UNIT; ++i)
	{
		if(IntegerUnit[i]!=-1){
			int C = Cost(ins[IntegerUnit[i]].ins);
			if(C > ins[IntegerUnit[i]].cycleDone) ins[IntegerUnit[i]].cycleDone++;
			else{
				if(ins[IntegerUnit[i]].ins=="HALT"){
					IntegerUnit[i] = -1;
				}
				else if(ins[IntegerUnit[i]].ins=="MOV" || ins[IntegerUnit[i]].ins=="LOAD"){
					if(rob[ins[IntegerUnit[i]].r3].hasVal){
						rob[ins[IntegerUnit[i]].r1].val = process(ins[IntegerUnit[i]].ins,0.0,rob[ins[IntegerUnit[i]].r3].val);
						rob[ins[IntegerUnit[i]].r1].hasVal = 1;
						int R = rob[ins[IntegerUnit[i]].r1].Register;
						if( Reg[R].rob_no == ins[IntegerUnit[i]].r1 && R!=16){
							Reg[R].val = rob[ins[IntegerUnit[i]].r1].val;
							Reg[R].hasVal = 1;
						}

						ins[IntegerUnit[i]].cycleDone = 0;
						IntegerUnit[i] = -1;
					}
				}
				else if(ins[IntegerUnit[i]].ins=="STR"){
					if(rob[ins[IntegerUnit[i]].r3].hasVal && rob[ins[IntegerUnit[i]].r2].hasVal){
						MEMORY.save(rob[ins[IntegerUnit[i]].r3].val,rob[ins[IntegerUnit[i]].r2].val);
						cout<<"Saved "<<rob[ins[IntegerUnit[i]].r2].val<<" at "<<rob[ins[IntegerUnit[i]].r3].val<<endl;

						ins[IntegerUnit[i]].cycleDone = 0;
						IntegerUnit[i] = -1;
					}
				}
				else if(ins[IntegerUnit[i]].ins=="BR"){
					if(rob[ins[IntegerUnit[i]].r3].hasVal){
						Curline+= rob[ins[IntegerUnit[i]].r3].val;
						ins[IntegerUnit[i]].cycleDone = 0;
						IntegerUnit[i] = -1;
						is_branch = false;
					}
				}
				else{
					if(rob[ins[IntegerUnit[i]].r3].hasVal && rob[ins[IntegerUnit[i]].r2].hasVal){
						bool b = ProcessBranch(ins[IntegerUnit[i]].ins,rob[ins[IntegerUnit[i]].r2].val);
						if(b){
							Curline+=rob[ins[IntegerUnit[i]].r3].val;
						}
						ins[IntegerUnit[i]].cycleDone = 0;
						IntegerUnit[i] = -1;
						is_branch = false;
					}
				}

				
			}
		}
	}
	if(is_branch)
		return true;
	/*
	for (int i = 1; i < 3; ++i)
	{
		cout<<Reg[i].val<<" & "<<Reg[i].hasVal<<" & "<<Reg[i].rob_no<<endl;
	}
	for (int i = 0; i < rob.size(); ++i)
	{
		cout<<rob[i].Register<<" : "<<rob[i].val<<" : "<<rob[i].hasVal<<endl;
	}*/
	
	if(Curline >= numlines){
		bool test = false;
		for (int i = 0; i < ADDER_UNIT; ++i)
			if(FPAdder[i]!=-1) {test = true; }

		for (int i = 0; i < MULTIPLIER_UNIT; ++i)
			if(FPMultiplier[i]!=-1) {test = true;}

		for (int i = 0; i < CONSTANT_UNIT; ++i)
			if(IntegerUnit[i]!=-1) {test = true;}

		return test;
	}
	
	if(Prevline == Curline){
		return true;
	}

	INSTR next = ins[Curline];
	bool flag = false;

	if(next.UnitType == 1){
		for (int i = 0; i < ADDER_UNIT; ++i)
		{
			if(FPAdder[i]==-1){
				CreateROBs();
				FPAdder[i] = Curline;
				flag = true;

				break;
			}
		}
	}

	else if(next.UnitType == 2){
		for (int i = 0; i < MULTIPLIER_UNIT; ++i)
		{
			if(FPMultiplier[i]==-1){
				CreateROBs();
				FPMultiplier[i] = Curline;
				flag = true;
				break;
			}
		}
	}

	else //if(next.UnitType == 3)
	{
		for (int i = 0; i < CONSTANT_UNIT; ++i)
		{
			if(IntegerUnit[i]==-1){
				if(ins[Curline].ins[0]=='B') is_branch = true;
				CreateROBs();
				IntegerUnit[i] = Curline;
				flag = true;
				break;
			}
		}
	}

	if(flag){
		Prevline = Curline;
		Curline++;
	}

	
	
	return true;
};

int main(int argc, char const *argv[])
{
	if (argc < 2)
	{
		cerr<<"Input File required \n";
		exit(0);
	}

	fstream inputStream(argv[1]);

	// Initialize Registers

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

	for(int i=0;inputStream>>ins[i].ins&&i<numlines;i++){
		if(ins[i].ins.size()>1 && ins[i].ins[0]=='-' && ins[i].ins[1]=='-'){
			getline(inputStream,garbage);
			i--;
		}
		else{
			inputStream>>ins[i].reg;
			ins[i].ParseReg();
		}
	}
	
	int numCycles = 0;
	while(ProcessQueue()){
		numCycles ++ ;
	}

	cout<<"Register Status \n";
	for (int i = 0; i < Reg.size()-1; ++i)
	{
		cout<<"R"<<i<<" : "<<Reg[i].val<<endl;
	}
	cout<<"Total No of Cycles = "<<numCycles<<endl;
	
	return 0;
}