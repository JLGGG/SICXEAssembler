//---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

//Code Author : Jun Su. Lim
//Student Num : 2010111661
//Created     : 2015-11-05
//Description : SICXE Assembler (floating point instruction 지원)

//---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

/***************************** DECLERATE VARIABLE ****************************/
//SICXE format
typedef enum { formatOne=1, formatTwo, formatThree, formatFour } FormatType;

//Register structure
typedef struct XeRegister 
{
	char regName;//register name
	int  regNum; //register number
}Register;

//operation structure
typedef struct OperationCodeTable
{
	char Mnemonic[8];//Mnemonic
	char Format;     //SICXE format
	unsigned short int  ManchineCode;//Machine code 
}SIC_OPTAB;

//Symbol structure
typedef struct SymbolTable
{
	char Label[10]; //label
	int Address;    //address
}SIC_SYMTAB;

//Intermediate file structure
typedef struct IntermediateRecord{
	unsigned short int LineIndex;//Line number
	unsigned short int Loc;      //relative address
	unsigned long int ObjectCode;//object code
	float FloatNum;              //float save
	int FloatFlag;               //float flag
	char LabelField[32];         //label
	char OperatorField[32];      //operator
	char OperandField[32];       //operand
	FormatType formType;         //SICXE format
}IntermediateRec;

//IEEE 754 규격에 따른 실수 사용
union {
	float f;            //실수 저장
	unsigned char c[8]; //실수 표현
}_float;


int Counter;
int LOCCTR[100];
int LocctrCounter = 0;
int ProgramLen;
int Index;
int j;
int ManchineCode;
int SymtabCounter = 0;
int start_address;
int program_length;
int ArrayIndex = 0;

unsigned short int FoundOnSymtab_flag = 0;
unsigned short int FoundOnOptab_flag = 0;

char Buffer[256];
char Label[32];
char Mnemonic[32];
char Operand[32];

SIC_SYMTAB SYMTAB[100];//symbol table
IntermediateRec* IMRArray[100];//intermediate file

//레지스터 저장
static Register REGTAB[]=
{
	{ 'A', 0},
	{ 'X', 1},
	{ 'L', 2},
	{ 'B', 3},
	{ 'S', 4},
	{ 'T', 5},
	{ 'F', 6},
};

//operation 저장
static SIC_OPTAB OPTAB[]=
{
    {   "ADD",  '3',  0x18},
    {   "AND",  '3',  0x40},
    {  "COMP",  '3',  0x28},
    {   "DIV",  '3',  0x24},
    {     "J",  '3',  0x3C},
    {   "JEQ",  '3',  0x30},
    {   "JGT",  '3',  0x34},
    {   "JLT",  '3',  0x38},
    {  "JSUB",  '3',  0x48},
    {   "LDA",  '3',  0x00},
    {  "LDCH",  '3',  0x50},
    {   "LDL",  '3',  0x08},
    {   "LDX",  '3',  0x04},
    {   "MUL",  '3',  0x20},
    {    "OR",  '3',  0x44},
    {    "RD",  '3',  0xD8},
    {  "RSUB",  '3',  0x4C},
    {   "STA",  '3',  0x0C},
    {  "STCH",  '3',  0x54},
    {   "STL",  '3',  0x14},
    {  "STSW",  '3',  0xE8},
    {   "STX",  '3',  0x10},
    {   "SUB",  '3',  0x1C},
    {    "TD",  '3',  0xE0},
    {   "TIX",  '3',  0x2C},
    {    "WD",  '3',  0xDC},
	{  "ADDR",  '2',  0x90}, //SICXE instruction
	{ "CLEAR",  '2',  0xB4},
	{ "COMPR",  '2',  0xA0},
	{  "DIVR",  '2',  0x9C},
	{   "LDB",  '3',  0x68},
	{   "LDS",  '3',  0x6C},
	{   "LDT",  '3',  0x74},
	{   "LPS",  '3',  0xD0},
	{  "MULR",  '2',  0x98},
	{   "RMO",  '2',  0xAC},
	{ "SHIFTL", '2',  0xA4},
	{ "SHIFTR", '2',  0xA8},
	{  "SIO",   '1',  0xF0},
	{  "SSK",   '3',  0xEC},
	{  "STB",   '3',  0x78},
	{  "STI",   '3',  0xD4},
	{  "STS",   '3',  0x7C},
	{  "STT",   '3',  0x84},
	{ "SUBR",   '2',  0x94},
	{  "SVC",   '2',  0xB0},
	{  "TIO",   '1',  0xF8},
	{ "TIXR",   '2',  0xB8},
	{  "HIO",   '1',  0xF4},
	{ "ADDF",   '3',  0x58}, //floating point instruction
	{ "COMPF",  '3',  0x88},
	{ "DIVF",   '3',  0x64},
	{  "FIX",   '1',  0xC4},
	{ "FLOAT",  '1',  0xC0},
	{  "LDF",   '3',  0x70},
	{ "MULF",   '3',  0x60},
	{ "NORM",   '1',  0xC8},
	{  "STF",   '3',  0x80},
	{ "SUBF",   '3',  0x5c},
};

/****************************** DFINATE FUNCTION *****************************/

//Parameter    : void
//Return       : char*, 레이블 반환
//Description  : 버퍼에서 레이블 읽어온 다음에 반환
char* ReadLabel(){
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Label[j++] = Buffer[Index++];
	Label[j] = '\0';
	return(Label);
}

//Parameter    : void
//Return       : void
//Description  : 공백 skip
void SkipSpace(){
	while (Buffer[Index] == ' ' || Buffer[Index] =='\t')
		Index++;
}

//Parameter    : void
//Return       : char*, operation return
//Description  : 버퍼에서 연산자 읽어온 다음에 반환
char* ReadOprator(){
	j = 0;//zeroing
	while(Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Mnemonic[j++] = Buffer[Index++];
	Mnemonic[j] = '\0';
	return(Mnemonic);
}

//Parameter    : void
//Return       : char*, operand return
//Description  : 버퍼에서 피연산자 읽어온 다음에 반환
char* ReadOperand(){
	j = 0;//zeroing
	while(Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Operand[j++] = Buffer[Index++];
	Operand[j] = '\0';
	return(Operand);
}

//Parameter    : char* label, 레이블 저장
//Return       : void
//Description  : 인자로 전달된 레이블 symbol table에 저장 및 주소 배정
void RecordSymtab(char* label){
	strcpy(SYMTAB[SymtabCounter].Label,label);
	SYMTAB[SymtabCounter].Address = LOCCTR[LocctrCounter-1];
	SymtabCounter++;	
}

//Parameter    : char* label, 레이블 저장
//Return       : int, flag return
//Description  : 레이블을 symbol table에서 검색, 레이블이 존재하면 flag 1 없으면 flag 0
int SearchSymtab(char* label){
	int k;
	FoundOnSymtab_flag = 0;

	for (k= 0; k<=SymtabCounter; k++)	{
		if (!strcmp(SYMTAB[k].Label,label)){
			FoundOnSymtab_flag = 1;
			return (FoundOnSymtab_flag);
		}
	}
	return (FoundOnSymtab_flag);
}

//Parameter    : char* Mnemonic, 니모닉 저장
//Return       : int, flag return
//Description  : 니모닉을 op table에서 검색, 니모닉이 존재하면 flag 1 없으면 flag 0
//             : OPTAB에서 니모닉 위치 Counter에 저장
int SearchOptab(char * Mnemonic){
	int size = sizeof(OPTAB)/sizeof(SIC_OPTAB);
	int i;
	FoundOnOptab_flag = 0;
	for(i=0;i<size;i++){
		if(!strcmp(Mnemonic,OPTAB[i].Mnemonic)){
			Counter = i;
			FoundOnOptab_flag = 1;
			break;
		}
	}
	return (FoundOnOptab_flag);
}

//Parameter    : char* c, 문자열 저장
//Return       : int, decimal return
//Description  : 매개변수에 저장된 문자열을 십진수로 변환 후 반환
int StrToDec(char* c){
	int dec_num = 0;
	int k,l;
	char temp[10];
	int len = strlen(c);
	strcpy(temp,c);

	for (k = len-1, l = 1; k>=0; k--)
	{
		dec_num = dec_num+(int)(temp[k]-'0')*l;
		l = l*10;//십진수로 변환
	}
	return (dec_num);
}

//Parameter    : char* c, 문자열 저장
//Return       : int, hex return
//Description  : 매개변수에 저장된 문자열을 16진수로 변환 후 반환
int StrToHex(char* c)
{
	int hex_num = 0;
	int k,l;
	char temp[10];
	int len = strlen(c);
	strcpy(temp, c);

	for (k = len-1, l = 1; k >=0; k--)
	{
		if (temp[k]>='0' && temp[k]<='9')
			hex_num = hex_num + (int)(temp[k]-'0')*l;
		else if (temp[k]>='A' && temp[k]<='F')
            hex_num = hex_num + (int)(temp[k]-'A'+10)*l;
		else if (temp[k]>='a' && temp[k]>='f')
            hex_num = hex_num + (int)(temp[k]-'a'+10)*l;
		else ;
		l = l*16;//16진수 변환
	}
	return (hex_num);
}

//Parameter    : char* c, 문자열 저장
//Return       : 길이 반환
//Description  : 매개변수에 저장된 C' , X' 문자열 길이 반환
int ComputeLen(char* c){
	unsigned int b;
	char len[32];
	
	strcpy(len,c);
	if (len[0] =='C' || len[0] =='c' && len[1] =='\''){
		for (b = 2; b<=strlen(len); b++){
			if (len[b] == '\''){
				b -=2;
				break;
			}
		}
	}
	if (len[0] =='X' || len[0] =='x' && len[1] =='\'')
		b = 1;
	return (b);
}

//Parameter    : void
//Return       : void
//Description  : List파일 생성
void CreateProgramList(){
	int loop;
	FILE *fptr_list; 

	//파일 생성(쓰기 모드)
	fptr_list = fopen("sicxe.list" , "w");
	
	//예외처리
    if (fptr_list == NULL)
	{
		printf("ERROE: Unable to open the sic.list.\n");
		exit(1);
	}

	//list파일 기록
	fprintf(fptr_list, "%-4s\t%-10s%-10s%-10s\t%s\n", "LOC", "LABEL", "OPERATOR", "OPERAND", "OBJECT CODE");
	for (loop = 0; loop<ArrayIndex; loop++)
	{
		//연산자가 START, RESW, RESB, END일 때 실행
		//목적코드가 존재하지 않는다.
		if (!strcmp(IMRArray[loop]->OperatorField,"START") || !strcmp(IMRArray[loop]->OperatorField,"RESW") || 
			!strcmp(IMRArray[loop]->OperatorField,"RESB") || !strcmp(IMRArray[loop]->OperatorField,"END"))
		    fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\n", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField,
			IMRArray[loop]->OperandField);
		//실수형이 아닐 때 출력
        else if(IMRArray[loop]->FloatFlag==0)
		    fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\t%06x\n", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField,
			IMRArray[loop]->OperandField, IMRArray[loop]->ObjectCode);
		//실수형일 때 출력
		else if(IMRArray[loop]->FloatFlag==1)
		{
			int i;
			 fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\t", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField, IMRArray[loop]->OperandField);

			 //공용체 4byte float멤버변수에 실수를 저장한다.
			 //float변수를 char형 1byte단위로 쪼개서 hex형으로 출력한다.
			 //실수 출력 반복문
			 _float.f = IMRArray[loop]->FloatNum;
			 for(i=0; i<4; i++)
				 fprintf(fptr_list, "%02x", _float.c[i]);

			 fprintf(fptr_list, "\n");
		}
	}
	fclose(fptr_list);
}

//Parameter    : void
//Return       : void
//Description  : object 파일 생성
void CreateObjectCode(){
	int first_address;
	int last_address;
	int temp_address;
	int first_index;
	int last_index;
	int x, xx, i;
	int loop;
	//임시변수
	int temp_objectcode[50];
	int temp_format[50]; 
	int floatFlag[50];
	float temp_float[50];
	char temp_operator[50][10];
	char temp_operand[50][10];

	FILE *fptr_obj;    
	fptr_obj = fopen("sicxe.obj","w");
    if (fptr_obj == NULL)
	{
		printf("ERROE: Unable to open the sic.obj.\n");
		exit(1);
	}

	printf("Creating Object Code...\n\n");

	//연산자가 START이면 H레코드를 출력한다.
	loop = 0;
	if (!strcmp(IMRArray[loop]->OperatorField, "START"))
	{
		printf("H%-6s%06x%06x\n",IMRArray[loop]->LabelField, start_address, program_length);
		fprintf(fptr_obj,"H^%-6s^%06x^%06x\n",IMRArray[loop]->LabelField, start_address, program_length);
		loop++;
	}

	//T레코드 출력 구문
	while(1)
	{

		first_address = IMRArray[loop]->Loc;
		last_address = IMRArray[loop]->Loc + 27;
		first_index = loop;

		//intermediate file로부터 데이터를 읽어온 다음 임시변수에 저장한다.
		//END를 만날때까지 데이터를 읽어온다.
		//RESB, RESW를 만나면 address만 저장한다.
		for (x = 0, temp_address = first_address ; temp_address<=last_address; loop++)
		{
			if (!strcmp(IMRArray[loop]->OperatorField, "END"))
				break;
			else if (strcmp(IMRArray[loop]->OperatorField, "RESB") && strcmp(IMRArray[loop]->OperatorField, "RESW"))
			{
				temp_format[x]=IMRArray[loop]->formType;
				temp_objectcode[x] = IMRArray[loop]->ObjectCode;
				floatFlag[x]=IMRArray[loop]->FloatFlag;

				//실수 읽어오는 구문
				//floatFlag가 1이면 intermediate file에 실수를 임시변수에 저장
				//floatFlag가 0이면 임시변수에 0.0 저장
				if(floatFlag[x]==1)
				{
					temp_float[x] = IMRArray[loop]->FloatNum;
				}
				else
					temp_float[x] = 0.0;

				strcpy(temp_operator[x], IMRArray[loop]->OperatorField);
				strcpy(temp_operand[x], IMRArray[loop]->OperandField);
				last_index = loop+1;
			    x++;
			}
			else ;
			temp_address = IMRArray[loop+1]->Loc;
		}

		//T레코드에 시작주소와 T레코드 한 라인의 길이 출력
		printf("T%06x%02x", first_address, (IMRArray[last_index]->Loc - IMRArray[first_index]->Loc));
		fprintf(fptr_obj, "T^%06x^%02x", first_address, (IMRArray[last_index]->Loc - IMRArray[first_index]->Loc));

		//임시변수에 저장된 데이터를 파일에 저장하는 구문
		//BYTE x'--' 명령어는 16진수 2자리로 파일에 저장
		//format3, 연산자가 WORD, BYTE C'---'중에 하나가 참이면서 실수 flag가 0일 때 16진수 6자리로 출력
		//실수 flag가 1이면 실수를 16진수 8자리로 파일에 출력
		//format4이면 16진수 8자리로 파일에 출력
		//format2이면 16진수 4자리로 출력
		//format1이면 16진수 2자리로 출력
		for (xx = 0; xx<x; xx++)
		{
			if ((strcmp(temp_operator[xx], "BYTE")==0) && (temp_operand[xx][0]=='X' || temp_operand[xx][0]=='x')){
			printf("%02x", temp_objectcode[xx]);
			fprintf(fptr_obj, "^%02x", temp_objectcode[xx]);
			}
			else if((temp_format[xx]==formatThree || !strcmp(temp_operator[xx], "WORD") || 
				(!strcmp(temp_operator[xx], "BYTE") && temp_operand[xx][0]=='C' || temp_operand[xx][0]=='c')) && floatFlag[xx]==0){
				printf("%06x", temp_objectcode[xx]);
			    fprintf(fptr_obj, "^%06x", temp_objectcode[xx]);
			}
			else if(floatFlag[xx]==1){
				_float.f = temp_float[xx];

				 //공용체 4byte float멤버변수에 실수를 저장한다.
				 //float변수를 char형 1byte단위로 쪼개서 hex형으로 출력한다.
				 //실수 출력 반복문
				fprintf(fptr_obj, "^");
				for(i=0; i<4; i++){
				printf("%02x", _float.c[i]);
			    fprintf(fptr_obj, "%02x", _float.c[i]);
				}
			}
			else if(temp_format[xx]==formatFour){
				printf("%08x", temp_objectcode[xx]);
			    fprintf(fptr_obj, "^%08x", temp_objectcode[xx]);
			}
			else if(temp_format[xx]==formatTwo){
				printf("%04x", temp_objectcode[xx]);
			    fprintf(fptr_obj, "^%04x", temp_objectcode[xx]);
			}
			else if(temp_format[xx]==formatOne){
				printf("%02x", temp_objectcode[xx]);
			    fprintf(fptr_obj, "^%02x", temp_objectcode[xx]);
			}
		}

		printf("\n");
		fprintf(fptr_obj,"\n");

		if (!strcmp(IMRArray[loop]->OperatorField, "END"))
			break;
	}

	//M레코드 출력 구문
	//프로그램의 시작주소가 0일 때 M레코드 출력
	//프로그램의 시작주소가 0이 아니면 program relocation 되었으므로 M레코드 출력 안함.
	//format4는 absolute-addressing이므로 프로그램 재배치를 위한 M레코드 출력해야 됨.
	if(LOCCTR[0]==0)
	{
		for(loop=1; loop<ArrayIndex; loop++)
		{
			if(IMRArray[loop]->formType==formatFour)
			{
				printf("M%06x%02x+%s", LOCCTR[loop-1]+1 ,5 ,IMRArray[0]->LabelField);      
				fprintf(fptr_obj,"M%06x^%02x+%s", LOCCTR[loop-1]+1, 5 ,IMRArray[0]->LabelField);
				printf("\n");
				fprintf(fptr_obj,"\n");
			}
		}
	}

	//E레코드 출력, 프로그램 실행주소 지정
	printf("E%06x\n\n", start_address);
	fprintf(fptr_obj, "E^%06x\n\n", start_address);
	fclose(fptr_obj);//파일 종료
}

/******************************* MAIN FUNCTION *******************************/
void main (void)
{
	//변수 선언
	FILE* fptr;

	char filename[15];
	char label[32];
	char opcode[32];
	char operand[32];

	int loc = 0;
	int line = 0;
	int loop;
	int is_empty_line;
	int is_comment;
	int loader_flag = 0;
	int search_symtab;
	int search_regtab;
	int i, x;

	unsigned long inst_fmt;
	unsigned long inst_fmt_opcode;
	unsigned long inst_fmt_index;
	unsigned long inst_fmt_address;

	printf(" ******************************************************************************\n");
	printf(" * Program: SICXE ASSEMBRER                                                   *\n");
	printf(" *                                                                            *\n");
	printf(" * Procedure:                                                                 *\n");
	printf(" *   - Enter file name of source code.                                        *\n");
	printf(" *   - Do pass 1 process.                                                     *\n");
	printf(" *   - Do pass 2 process.                                                     *\n");
	printf(" *   - Create \"program list\" data on sic.list.(Use Notepad to read this file) *\n");
	printf(" *   - Create \"object code\" data on sic.obj.(Use Notepad to read this file)   *\n");
	printf(" *   - Also output object code to standard output device.                     *\n");
	printf(" ******************************************************************************\n");


	printf("\nEnter the file name you want to assembly (sic.asm):");
	scanf("%s",filename);
	fptr = fopen(filename,"r");//파일 열기

	//예외처리
	if (fptr == NULL)
	{
		printf("ERROE: Unable to open the %s file.\n",filename);
		exit(1);
	}

/********************************** PASS 1 ***********************************/
	printf("Pass 1 Processing...\n\n");
	while (fgets(Buffer,256,fptr) != NULL)
	{
		is_empty_line = strlen(Buffer);

		Index = 0;
		j = 0;
		//버퍼로부터 레이블을 읽어와 변수에 저장
		strcpy(label,ReadLabel());

		//명령어인지 주석인지 판단하는 구문
        if (Label[0] == '.')
			is_comment = 1;
		else
			is_comment = 0;

		//읽어온 명령어의 길이가 1이상 이면서 주석이 아닐 때 실행되는 구문
		if (is_empty_line>1 && is_comment!=1)
		{
			Index = 0;
			j = 0;
		    
			//intermediate structure 힙에 동적 할당
			IMRArray[ArrayIndex] = (IntermediateRec*)malloc(sizeof(IntermediateRec));
			
			//버퍼로부터 레이블을 읽어와 중간파일에 저장
			IMRArray[ArrayIndex]->LineIndex = ArrayIndex;
			strcpy(label,ReadLabel());
			strcpy(IMRArray[ArrayIndex]->LabelField,label);
			SkipSpace();

			//버퍼로부터 연산자를 읽어온 다음 중간파일에 저장
			//연산자가 START이면 피연산자를 읽어온 다음 중간파일에 저장
			//피연산자를 16진수로 변환한 다음 location counter에 저장
			//16진수로 변환된 수는 프로그램의 시작주소가 됨.
			if (line == 0)
			{
				strcpy(opcode,ReadOprator());
				strcpy(IMRArray[ArrayIndex]->OperatorField,opcode);
				if (!strcmp(opcode,"START"))
				{
					SkipSpace();//연산자와 피연산자 사이의 공백 제거
					strcpy(operand,ReadOperand());
					strcpy(IMRArray[ArrayIndex]->OperandField, operand);/* [A] */
					LOCCTR[LocctrCounter] = StrToHex(operand);
					start_address = LOCCTR[LocctrCounter];
				}
				//START연산자가 없으면 프로그램의 시작주소는 0
				else
				{
					LOCCTR[LocctrCounter] = 0;
					start_address = LOCCTR[LocctrCounter];
				}
			}
			//START연산자가 포함된 명령어를 제외한 모든 명령어 처리 구문
			//연산자와 피연산자를 버퍼로부터 읽어온 다음 중간파일에 저장
			else
			{
				strcpy(opcode,ReadOprator());
				strcpy(IMRArray[ArrayIndex]->OperatorField,opcode);
				SkipSpace();
				strcpy(operand,ReadOperand());
				strcpy(IMRArray[ArrayIndex]->OperandField,operand);

				//연산자가 END가 아닐 때 실행
				if (strcmp(opcode,"END"))
				{
					if (label[0] != '\0')
					{
						//레이블이 symbol table에 존재하는지 확인
						//레이블이 존재하면 중복 심볼이므로 에러 출력 후 파일 종료
						if (SearchSymtab(label))
						{
							fclose(fptr);
							printf("ERROE: Duplicate Symbol\n");
							FoundOnSymtab_flag = 0;
							exit(1);
						}
						//symbol table에 레이블 저장
						RecordSymtab(label);
					}

					//format4일 때 실행되는 구문
					if (SearchOptab (opcode) || opcode[0]=='+')
					{
						if(opcode[0]=='+')
						{
							//연산자 앞에 붙은 +를 제거한 형태로 만드는 반복문
							//배열의 0번지에 저장된 +를 뒤에 저장된 데이터를 앞으로 한칸씩 이동해서 +삭제
							for(i=0; i<strlen(opcode)-1; i++)
								opcode[i]=opcode[i+1];
							opcode[strlen(opcode)-1]='\0';

							//연산자가 OPTAB에 존재하는지 확인
							//format4이므로 길이는 4byte 증가
							//중간파일 formType에 format4인 것 저장
							SearchOptab(opcode);
							LOCCTR[LocctrCounter] = loc + 4;
							IMRArray[ArrayIndex]->formType=formatFour;
						}
						//연산자 마지막 철자에 R이 붙어있으면서 OR이 아닐 때 실행
						//OR은 format3이므로 해당 구문에서 실행되면 안됨.
						//연산자 마지막 철자에 R이 붙어있으면 레지스터 연산.
						//레지스터 연산은 format2이므로 길이 2만큼 증가
						//중간파일 formType에 format2인 것 저장
						else if(opcode[strlen(opcode)-1]=='R' && strcmp(opcode, "OR"))
						{
							LOCCTR[LocctrCounter] = loc + 2;
							IMRArray[ArrayIndex]->formType=formatTwo;
						}
						//연산자 마지막 철자에 R이 안 붙은 format2 명령어 처리 구문
						//중간파일 formType에 format2인 것 저장
						else if(!strcmp(opcode, "SHIFTL") || !strcmp(opcode, "RMO") || !strcmp(opcode, "SVC"))
						{
							LOCCTR[LocctrCounter] = loc + 2;
							IMRArray[ArrayIndex]->formType=formatTwo;
						}
						//format1 연산자 처리 구문
						//연산자 마지막 철자가 O이거나 FIX, FLOAT, NORM일 때 실행
						//중간파일 formType에 format1인 것 저장
						//format1이므로 길이 1만큼 증가
						else if(opcode[strlen(opcode)-1]=='O' || !strcmp(opcode, "FIX") || !strcmp(opcode, "FLOAT") || !strcmp(opcode, "NORM"))
						{
							LOCCTR[LocctrCounter] = loc + 1;
							IMRArray[ArrayIndex]->formType=formatOne;
						}
						//format3 처리 구문
						//OPTAB에 저장되어있는 format을 읽어와 정수로 변환한 다음 location counter에 저장
						//중간파일 formType에 format3인 것 저장
						else
						{
							LOCCTR[LocctrCounter] = loc + (int)(OPTAB[Counter].Format-'0') ;
							IMRArray[ArrayIndex]->formType=formatThree;
						}
					}
					//실행 연산자가 아닌 메모리 연산자인 경우 실행
					//WORD는 SICXE에서 3byte로 표현되므로 길이 3 증가
					//RESW는 피연산자에 저장된 수*3만큼 길이 증가
					//RESB는 피연산자에 저장된 수*1만큼 길이 증가
					//BYTE 연산자의 피연산자는 C' 또는 X'와 같이 쓰이므로 C'---', X'--' 사이의 길이 측정 후 location counter에 저장 
					else if (!strcmp(opcode,"WORD"))
						LOCCTR[LocctrCounter] = loc + 3;
					else if (!strcmp(opcode,"RESW"))
						LOCCTR[LocctrCounter] = loc + 3 * StrToDec(operand);
					else if (!strcmp(opcode,"RESB"))
						LOCCTR[LocctrCounter] = loc + StrToDec(operand);
					else if (!strcmp(opcode,"BYTE"))
						LOCCTR[LocctrCounter] = loc + ComputeLen(operand);
					else{//예외처리
						fclose(fptr);
						printf("ERROE: Invalid Operation Code\n");
						exit(1);
					}
				}
			}
			//START를 제외한 명령어 길이를 중간파일에 저장하기 위해서 LocctrCounter-1을 함.
			loc = LOCCTR[LocctrCounter];
			IMRArray[ArrayIndex]->Loc = LOCCTR[LocctrCounter-1];
			LocctrCounter++;
			ArrayIndex++;
		}
		FoundOnOptab_flag = 0;
		line += 1;
	}
	//프로그램 전체 길이 계산
	//반복문이 종료될 때 LocctrCounter가 1증가하면서 종료되었으므로 마지막 명령어의 위치는 LocctrCounter-2를 해야 됨.
	program_length = LOCCTR[LocctrCounter-2]- LOCCTR[0];

	//symbol table 출력
	//레이블과 주소를 출력한다.
	printf("SYMBOL TABLE\n");
	printf("------------------\n");
	printf("LABEL\tADDRESS\n");
	printf("------------------\n");
	for(loop=0; loop<ArrayIndex; loop++)   //symbol table print!!
	{
		if(isalpha(SYMTAB[loop].Label[0]))
			printf("%s\t%x\n",SYMTAB[loop].Label,SYMTAB[loop].Address);
	}

/********************************** PASS 2 ***********************************/
	printf("\nPass 2 Processing...\n");

	//intermediate stucture array에 길이만큼 반복
	for (loop = 1; loop<ArrayIndex; loop++){
		inst_fmt_opcode = 0;
		inst_fmt_index = 0;
		inst_fmt_address = 0;
		IMRArray[loop]->FloatFlag = 0;

		strcpy(opcode,IMRArray[loop]->OperatorField);

		//연산자가 OPTAB에 존재하면 실행
		if (SearchOptab(opcode) || opcode[0]=='+'){
			//format4인 경우 실행
			if(opcode[0]=='+' && IMRArray[loop]->formType==formatFour)
			{
				//연산자 앞에 붙은 +를 제거한 형태로 만드는 반복문
				//배열의 0번지에 저장된 +를 뒤에 저장된 데이터를 앞으로 한칸씩 이동해서 +삭제
				for(i=0; i<strlen(opcode)-1; i++)
						opcode[i]=opcode[i+1];
				opcode[strlen(opcode)-1]='\0';
				
				//연산자를 OPTAB에서 검색한 후 연산자의 머신코드와 3h를 더한 다음 임시변수에 저장
				//3h를 더하는 이유는 SICXE명령어의 opcode 8bit에서 n , i flag가 항상 1이므로 3을 더한다.
				//임시변수에 저장된 opcode를 좌측으로 24bit만큼 이동한다.
				//임시변수에 저장된 opcode를 중간파일에 저장한다.
				//피연산자를 중간파일에서 읽어온 다음 operand변수에 저장
				//피연산자가 operand,X 형태이면 indexed addressing이므로 플래그의 값은 9가 됨
				//operand,X 형태가 아니면 플래그의 값은 1이 됨
				//피연산자를 SYMTAB에서 검색한 다음 SYMTAB에 저장된 주소를 inst_fmt_address에 저장
				//inst_fmt_opcode + inst_fmt_index + inst_fmt_address를 더하면 목적코드가 된다.
				SearchOptab(opcode);
				inst_fmt_opcode = OPTAB[Counter].ManchineCode+0x03;
				inst_fmt_opcode <<=24;
				IMRArray[loop]->ObjectCode = inst_fmt_opcode;
				strcpy(operand,IMRArray[loop]->OperandField);

				if (operand[strlen(operand)-2] == ',' && (operand[strlen(operand)-1] == 'X' || operand[strlen(operand)-1] == 'x')){
				inst_fmt_index = 0x00900000;
				operand[strlen(operand)-2] = '\0';
				}
				else
					inst_fmt_index = 0x00100000;

				for(search_symtab = 0; search_symtab<SymtabCounter; search_symtab++){
					if(!strcmp(operand, SYMTAB[search_symtab].Label))
					inst_fmt_address = (long)SYMTAB[search_symtab].Address ;
				}
				inst_fmt =  inst_fmt_opcode + inst_fmt_index + inst_fmt_address;
				IMRArray[loop]->ObjectCode = inst_fmt;
			}
			//format2인 경우 실행
			//format2는 16bit이므로 opcode를 8bit 좌측으로 이동 
			//레지스터 연산은 피연산자가 2개이거나 1개인 경우가 존재한다.
			//피연산자로 쓰인 레지스터를 REGTAB에서 검색한다.
			//피연산자 레지스터가 2개인 경우 첫 번째 레지스터는 레지스터의 값에다가 16을 곱한다.
			//두 번째 레지스터는 레지스터의 값에다가 1을 곱한다. 2개의 값을 더한 후 address에 저장
			//16과 1을 곱하는 이유는 operand field가 1byte인데 첫 번째 레지스터는 앞쪽 4bit, 두 번째 레지스터는 뒤쪽 4bit에 저장되기 때문이다.
			//피연산자가 1개인 경우 REGTAB에서 레지스터를 검색한 후 레지스터의 값과 16을 곱한 다음 address에 저장
			//16을 곱하는 이유는 레지스터가 1개일 때 1byte에서 상위 4bit에 값이 저장되고 하위 4bit는 0으로 채워지기 때문이다.
			//inst_fmt_opcode+inst_fmt_address의 값이 목적코드가 된다. format2는 플래그가 없으므로 inst_fmt_index는 없다.
			else if(opcode[strlen(opcode)-1]=='R'&& IMRArray[loop]->formType==formatTwo)
			{
				inst_fmt_opcode = OPTAB[Counter].ManchineCode;
				inst_fmt_opcode <<=8;
				IMRArray[loop]->ObjectCode = inst_fmt_opcode;
			    strcpy(operand,IMRArray[loop]->OperandField);

				if(isalpha(operand[0]) && operand[1]==',' && isalpha(operand[2]))
				{
					for(search_regtab=0; search_regtab<7; search_regtab++)
					{
						if(operand[0]==REGTAB[search_regtab].regName)
							inst_fmt_address += REGTAB[search_regtab].regNum*16;
						if(operand[2]==REGTAB[search_regtab].regName)
							inst_fmt_address += REGTAB[search_regtab].regNum*1;
					}
				}
				else if(isalpha(operand[0]) && operand[1]=='\0')
				{
					for(search_regtab=0; search_regtab<7; search_regtab++)
					{
						if(operand[0]==REGTAB[search_regtab].regName)
							inst_fmt_address += REGTAB[search_regtab].regNum*16;
					}

				}
				inst_fmt = inst_fmt_opcode+inst_fmt_address;
				IMRArray[loop]->ObjectCode = inst_fmt;
			}
			//format2인 경우 실행
			//format2는 16bit이므로 opcode를 8bit 좌측으로 이동 
			//SHIFTL, RMO, SVC 연산은 피연산자가 2개이거나 1개인 경우가 존재한다.
			//피연산자로 쓰인 레지스터를 REGTAB에서 검색한다.
			//피연산자 레지스터가 2개인 경우 첫 번째 레지스터는 레지스터의 값에다가 16을 곱한다.
			//두 번째 레지스터는 레지스터의 값에다가 1을 곱한다. 2개의 값을 더한 후 address에 저장
			//16과 1을 곱하는 이유는 operand field가 1byte인데 첫 번째 레지스터는 앞쪽 4bit, 두 번째 레지스터는 뒤쪽 4bit에 저장되기 때문이다.
			//피연산자가 1개인 경우 REGTAB에서 레지스터를 검색한 후 레지스터의 값과 16을 곱한 다음 address에 저장
			//16을 곱하는 이유는 레지스터가 1개일 때 1byte에서 상위 4bit에 값이 저장되고 하위 4bit는 0으로 채워지기 때문이다.
			//inst_fmt_opcode+inst_fmt_address의 값이 목적코드가 된다. format2는 플래그가 없으므로 inst_fmt_index는 없다.
			else if((!strcmp(opcode, "SHIFTL") || !strcmp(opcode, "RMO") || !strcmp(opcode, "SVC")) && IMRArray[loop]->formType==formatTwo)
			{
				inst_fmt_opcode = OPTAB[Counter].ManchineCode;
				inst_fmt_opcode <<=8;
				IMRArray[loop]->ObjectCode = inst_fmt_opcode;
			    strcpy(operand,IMRArray[loop]->OperandField);		

				if(isalpha(operand[0]) && operand[1]==',' && isalpha(operand[2]))
				{
					for(search_regtab=0; search_regtab<7; search_regtab++)
					{
						if(operand[0]==REGTAB[search_regtab].regName)
							inst_fmt_address += REGTAB[search_regtab].regNum*16;
						if(operand[2]==REGTAB[search_regtab].regName)
							inst_fmt_address += REGTAB[search_regtab].regNum*1;
					}
				}
				else if(isalpha(operand[0]) && operand[1]=='\0')
				{
					for(search_regtab=0; search_regtab<7; search_regtab++)
					{
						if(operand[0]==REGTAB[search_regtab].regName)
							inst_fmt_address += REGTAB[search_regtab].regNum*16;
					}

				}
				inst_fmt = inst_fmt_opcode+inst_fmt_address;
				IMRArray[loop]->ObjectCode = inst_fmt;
			}
			//format1인 경우 실행
			//format1은 1byte이므로 OPTAB에 machine code가 목적코드가 된다.
			else if((opcode[strlen(opcode)-1]=='O' || !strcmp(opcode, "FIX") || !strcmp(opcode, "FLOAT") || !strcmp(opcode, "NORM"))
				&& IMRArray[loop]->formType==formatOne)
			{
				inst_fmt_opcode = OPTAB[Counter].ManchineCode;
				IMRArray[loop]->ObjectCode = inst_fmt_opcode;	

				inst_fmt=inst_fmt_opcode;
				IMRArray[loop]->ObjectCode=inst_fmt;
			}
			//format3인 경우 실행
			//opcode는 SICXE이므로 연산코드+3h를 해서 구한다.
			//foramt3는 24bit를 사용해서 명령어를 표현하므로 opcode는 16bit 좌측으로 이동한다.
			//opcode가 RSUB인 경우에는 플래그 비트와 TA가 필요없다. 연산코드가 목적코드가 된다.
			//format3는 relative-addressing으로 구동되므로 해당 프로그램에서는 pc-relative addressing을 사용했다.
			//TA와 pc가 가리키는 주소를 빼서 relative-address를 구했다.
			//operand,X인 경우 indexed addressing이므로 플래그는 8+2=A가 된다.
			//operand,X가 아닌 경우 플래그는 2가 된다.
			//inst_fmt_opcode+inst_fmt_index+inst_fmt_address의 값이 목적코드가 된다.
			else if(IMRArray[loop]->formType==formatThree)
			{
				inst_fmt_opcode = OPTAB[Counter].ManchineCode+0x03;
				inst_fmt_opcode <<=16;
				IMRArray[loop]->ObjectCode = inst_fmt_opcode;
				strcpy(operand,IMRArray[loop]->OperandField);

				if(strcmp(opcode, "RSUB"))
				{
					if (operand[strlen(operand)-2] == ',' && (operand[strlen(operand)-1] == 'X' || operand[strlen(operand)-1] == 'x')){
						inst_fmt_index = 0x00A000;
						operand[strlen(operand)-2] = '\0';
					}
					else
						inst_fmt_index = 0x002000;	


					for(search_symtab = 0; search_symtab<SymtabCounter; search_symtab++){
						if(!strcmp(operand, SYMTAB[search_symtab].Label))
							//PC-relative addressing.
						inst_fmt_address = (long)SYMTAB[search_symtab].Address - LOCCTR[loop];
					}
				}
				inst_fmt =  inst_fmt_opcode + inst_fmt_index + inst_fmt_address;
				IMRArray[loop]->ObjectCode = inst_fmt;
			}
		
		}
		//연산자가 WORD인 경우 실행
		//피연산자를 중간파일에서 가져온 다음 피연산자 내부에 '.'이 있는지 확인
		//'.'이 존재하면 피연산자의 값은 실수이다.
		//strchr로 실수를 판별한 후 실수이면 피연산자를 atof로 실수로 변환한 다음 중간파일 FloatNum에 저장
		//실수이면 중간파일 FloatFlag는 1이 됨
		//실수가 아니면 피연산자를 정수로 변환한 다음 중간파일 ObjectCode에 저장
		else if (!strcmp(opcode, "WORD")){
			strcpy(operand,IMRArray[loop]->OperandField);
			if(strchr(operand,'.'))
			{
				IMRArray[loop]->FloatNum=atof(operand);
				IMRArray[loop]->FloatFlag=1;
			}
			else
			{
				IMRArray[loop]->ObjectCode = StrToDec(operand);//float 수정할 곳
			}
		}
		//연산자가 BYTE인 경우 실행
		//BYTE C'---' 형태인 경우 for문을 사용해서 C'---' 내부의 값을 int형으로 변환한 다음 ObjectCode에 저장
		//ObjectCode에 저장된 값을 좌측으로 8bit 이동한다. 이동하는 이유는 C'를 없애기 위해서
		//BYTE X'--' 형태인 경우 X'--' 내부의 값을 hex로 변환한 다음 ObjectCode에 저장한다.
		//ObjectCode에 저장된 값을 좌측으로 8bit 이동한다. 이동하는 이유는 X'를 없애기 위해서
		else if (!strcmp(opcode,"BYTE")){
			strcpy(operand,IMRArray[loop]->OperandField);
            IMRArray[loop]->ObjectCode = 0;

			if(operand[0]=='C' || operand[0]=='c' && operand[1]=='\''){
				for (x = 2; x<=(int)(strlen(operand)-2); x++){
					IMRArray[loop]->ObjectCode=IMRArray[loop]->ObjectCode + (int)operand[x];
					IMRArray[loop]->ObjectCode<<=8;
				}
			}

            if(operand[0]=='X' || operand[0]=='x' && operand[1]=='\''){
				char *operand_ptr;
				operand_ptr = &operand[2];//X' 다음에 값을 가리킴
				*(operand_ptr+2)='\0';//X'--'에서 마지막 ' 를 지움
				for (x=2; x<=(int)(strlen(operand)-2); x++){
					IMRArray[loop]->ObjectCode=IMRArray[loop]->ObjectCode + StrToHex(operand_ptr);
					IMRArray[loop]->ObjectCode<<=8;
				}
			}
			//좌측으로 8bit 이동한 것을 원래 상태로 복귀
	    	IMRArray[loop]->ObjectCode>>=8;
		}	
	}
	

	CreateProgramList();//List file creation
	CreateObjectCode();//object file creation
	
	//intermediate structure free
	for (loop = 0; loop<ArrayIndex; loop++){
        free(IMRArray[loop]);
	}

	printf("Compeleted Assembly\n");
	fclose(fptr);//파일 종료
}