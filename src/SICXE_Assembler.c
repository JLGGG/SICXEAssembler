//---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

//Code Author : Jun Su. Lim
//Student Num : 2010111661
//Created     : 2015-11-05
//Description : SICXE Assembler (floating point instruction ����)

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

//IEEE 754 �԰ݿ� ���� �Ǽ� ���
union {
	float f;            //�Ǽ� ����
	unsigned char c[8]; //�Ǽ� ǥ��
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

//�������� ����
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

//operation ����
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
//Return       : char*, ���̺� ��ȯ
//Description  : ���ۿ��� ���̺� �о�� ������ ��ȯ
char* ReadLabel(){
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Label[j++] = Buffer[Index++];
	Label[j] = '\0';
	return(Label);
}

//Parameter    : void
//Return       : void
//Description  : ���� skip
void SkipSpace(){
	while (Buffer[Index] == ' ' || Buffer[Index] =='\t')
		Index++;
}

//Parameter    : void
//Return       : char*, operation return
//Description  : ���ۿ��� ������ �о�� ������ ��ȯ
char* ReadOprator(){
	j = 0;//zeroing
	while(Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Mnemonic[j++] = Buffer[Index++];
	Mnemonic[j] = '\0';
	return(Mnemonic);
}

//Parameter    : void
//Return       : char*, operand return
//Description  : ���ۿ��� �ǿ����� �о�� ������ ��ȯ
char* ReadOperand(){
	j = 0;//zeroing
	while(Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Operand[j++] = Buffer[Index++];
	Operand[j] = '\0';
	return(Operand);
}

//Parameter    : char* label, ���̺� ����
//Return       : void
//Description  : ���ڷ� ���޵� ���̺� symbol table�� ���� �� �ּ� ����
void RecordSymtab(char* label){
	strcpy(SYMTAB[SymtabCounter].Label,label);
	SYMTAB[SymtabCounter].Address = LOCCTR[LocctrCounter-1];
	SymtabCounter++;	
}

//Parameter    : char* label, ���̺� ����
//Return       : int, flag return
//Description  : ���̺��� symbol table���� �˻�, ���̺��� �����ϸ� flag 1 ������ flag 0
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

//Parameter    : char* Mnemonic, �ϸ�� ����
//Return       : int, flag return
//Description  : �ϸ���� op table���� �˻�, �ϸ���� �����ϸ� flag 1 ������ flag 0
//             : OPTAB���� �ϸ�� ��ġ Counter�� ����
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

//Parameter    : char* c, ���ڿ� ����
//Return       : int, decimal return
//Description  : �Ű������� ����� ���ڿ��� �������� ��ȯ �� ��ȯ
int StrToDec(char* c){
	int dec_num = 0;
	int k,l;
	char temp[10];
	int len = strlen(c);
	strcpy(temp,c);

	for (k = len-1, l = 1; k>=0; k--)
	{
		dec_num = dec_num+(int)(temp[k]-'0')*l;
		l = l*10;//�������� ��ȯ
	}
	return (dec_num);
}

//Parameter    : char* c, ���ڿ� ����
//Return       : int, hex return
//Description  : �Ű������� ����� ���ڿ��� 16������ ��ȯ �� ��ȯ
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
		l = l*16;//16���� ��ȯ
	}
	return (hex_num);
}

//Parameter    : char* c, ���ڿ� ����
//Return       : ���� ��ȯ
//Description  : �Ű������� ����� C' , X' ���ڿ� ���� ��ȯ
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
//Description  : List���� ����
void CreateProgramList(){
	int loop;
	FILE *fptr_list; 

	//���� ����(���� ���)
	fptr_list = fopen("sicxe.list" , "w");
	
	//����ó��
    if (fptr_list == NULL)
	{
		printf("ERROE: Unable to open the sic.list.\n");
		exit(1);
	}

	//list���� ���
	fprintf(fptr_list, "%-4s\t%-10s%-10s%-10s\t%s\n", "LOC", "LABEL", "OPERATOR", "OPERAND", "OBJECT CODE");
	for (loop = 0; loop<ArrayIndex; loop++)
	{
		//�����ڰ� START, RESW, RESB, END�� �� ����
		//�����ڵ尡 �������� �ʴ´�.
		if (!strcmp(IMRArray[loop]->OperatorField,"START") || !strcmp(IMRArray[loop]->OperatorField,"RESW") || 
			!strcmp(IMRArray[loop]->OperatorField,"RESB") || !strcmp(IMRArray[loop]->OperatorField,"END"))
		    fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\n", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField,
			IMRArray[loop]->OperandField);
		//�Ǽ����� �ƴ� �� ���
        else if(IMRArray[loop]->FloatFlag==0)
		    fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\t%06x\n", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField,
			IMRArray[loop]->OperandField, IMRArray[loop]->ObjectCode);
		//�Ǽ����� �� ���
		else if(IMRArray[loop]->FloatFlag==1)
		{
			int i;
			 fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\t", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField, IMRArray[loop]->OperandField);

			 //����ü 4byte float��������� �Ǽ��� �����Ѵ�.
			 //float������ char�� 1byte������ �ɰ��� hex������ ����Ѵ�.
			 //�Ǽ� ��� �ݺ���
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
//Description  : object ���� ����
void CreateObjectCode(){
	int first_address;
	int last_address;
	int temp_address;
	int first_index;
	int last_index;
	int x, xx, i;
	int loop;
	//�ӽú���
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

	//�����ڰ� START�̸� H���ڵ带 ����Ѵ�.
	loop = 0;
	if (!strcmp(IMRArray[loop]->OperatorField, "START"))
	{
		printf("H%-6s%06x%06x\n",IMRArray[loop]->LabelField, start_address, program_length);
		fprintf(fptr_obj,"H^%-6s^%06x^%06x\n",IMRArray[loop]->LabelField, start_address, program_length);
		loop++;
	}

	//T���ڵ� ��� ����
	while(1)
	{

		first_address = IMRArray[loop]->Loc;
		last_address = IMRArray[loop]->Loc + 27;
		first_index = loop;

		//intermediate file�κ��� �����͸� �о�� ���� �ӽú����� �����Ѵ�.
		//END�� ���������� �����͸� �о�´�.
		//RESB, RESW�� ������ address�� �����Ѵ�.
		for (x = 0, temp_address = first_address ; temp_address<=last_address; loop++)
		{
			if (!strcmp(IMRArray[loop]->OperatorField, "END"))
				break;
			else if (strcmp(IMRArray[loop]->OperatorField, "RESB") && strcmp(IMRArray[loop]->OperatorField, "RESW"))
			{
				temp_format[x]=IMRArray[loop]->formType;
				temp_objectcode[x] = IMRArray[loop]->ObjectCode;
				floatFlag[x]=IMRArray[loop]->FloatFlag;

				//�Ǽ� �о���� ����
				//floatFlag�� 1�̸� intermediate file�� �Ǽ��� �ӽú����� ����
				//floatFlag�� 0�̸� �ӽú����� 0.0 ����
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

		//T���ڵ忡 �����ּҿ� T���ڵ� �� ������ ���� ���
		printf("T%06x%02x", first_address, (IMRArray[last_index]->Loc - IMRArray[first_index]->Loc));
		fprintf(fptr_obj, "T^%06x^%02x", first_address, (IMRArray[last_index]->Loc - IMRArray[first_index]->Loc));

		//�ӽú����� ����� �����͸� ���Ͽ� �����ϴ� ����
		//BYTE x'--' ��ɾ�� 16���� 2�ڸ��� ���Ͽ� ����
		//format3, �����ڰ� WORD, BYTE C'---'�߿� �ϳ��� ���̸鼭 �Ǽ� flag�� 0�� �� 16���� 6�ڸ��� ���
		//�Ǽ� flag�� 1�̸� �Ǽ��� 16���� 8�ڸ��� ���Ͽ� ���
		//format4�̸� 16���� 8�ڸ��� ���Ͽ� ���
		//format2�̸� 16���� 4�ڸ��� ���
		//format1�̸� 16���� 2�ڸ��� ���
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

				 //����ü 4byte float��������� �Ǽ��� �����Ѵ�.
				 //float������ char�� 1byte������ �ɰ��� hex������ ����Ѵ�.
				 //�Ǽ� ��� �ݺ���
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

	//M���ڵ� ��� ����
	//���α׷��� �����ּҰ� 0�� �� M���ڵ� ���
	//���α׷��� �����ּҰ� 0�� �ƴϸ� program relocation �Ǿ����Ƿ� M���ڵ� ��� ����.
	//format4�� absolute-addressing�̹Ƿ� ���α׷� ���ġ�� ���� M���ڵ� ����ؾ� ��.
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

	//E���ڵ� ���, ���α׷� �����ּ� ����
	printf("E%06x\n\n", start_address);
	fprintf(fptr_obj, "E^%06x\n\n", start_address);
	fclose(fptr_obj);//���� ����
}

/******************************* MAIN FUNCTION *******************************/
void main (void)
{
	//���� ����
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
	fptr = fopen(filename,"r");//���� ����

	//����ó��
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
		//���۷κ��� ���̺��� �о�� ������ ����
		strcpy(label,ReadLabel());

		//��ɾ����� �ּ����� �Ǵ��ϴ� ����
        if (Label[0] == '.')
			is_comment = 1;
		else
			is_comment = 0;

		//�о�� ��ɾ��� ���̰� 1�̻� �̸鼭 �ּ��� �ƴ� �� ����Ǵ� ����
		if (is_empty_line>1 && is_comment!=1)
		{
			Index = 0;
			j = 0;
		    
			//intermediate structure ���� ���� �Ҵ�
			IMRArray[ArrayIndex] = (IntermediateRec*)malloc(sizeof(IntermediateRec));
			
			//���۷κ��� ���̺��� �о�� �߰����Ͽ� ����
			IMRArray[ArrayIndex]->LineIndex = ArrayIndex;
			strcpy(label,ReadLabel());
			strcpy(IMRArray[ArrayIndex]->LabelField,label);
			SkipSpace();

			//���۷κ��� �����ڸ� �о�� ���� �߰����Ͽ� ����
			//�����ڰ� START�̸� �ǿ����ڸ� �о�� ���� �߰����Ͽ� ����
			//�ǿ����ڸ� 16������ ��ȯ�� ���� location counter�� ����
			//16������ ��ȯ�� ���� ���α׷��� �����ּҰ� ��.
			if (line == 0)
			{
				strcpy(opcode,ReadOprator());
				strcpy(IMRArray[ArrayIndex]->OperatorField,opcode);
				if (!strcmp(opcode,"START"))
				{
					SkipSpace();//�����ڿ� �ǿ����� ������ ���� ����
					strcpy(operand,ReadOperand());
					strcpy(IMRArray[ArrayIndex]->OperandField, operand);/* [A] */
					LOCCTR[LocctrCounter] = StrToHex(operand);
					start_address = LOCCTR[LocctrCounter];
				}
				//START�����ڰ� ������ ���α׷��� �����ּҴ� 0
				else
				{
					LOCCTR[LocctrCounter] = 0;
					start_address = LOCCTR[LocctrCounter];
				}
			}
			//START�����ڰ� ���Ե� ��ɾ ������ ��� ��ɾ� ó�� ����
			//�����ڿ� �ǿ����ڸ� ���۷κ��� �о�� ���� �߰����Ͽ� ����
			else
			{
				strcpy(opcode,ReadOprator());
				strcpy(IMRArray[ArrayIndex]->OperatorField,opcode);
				SkipSpace();
				strcpy(operand,ReadOperand());
				strcpy(IMRArray[ArrayIndex]->OperandField,operand);

				//�����ڰ� END�� �ƴ� �� ����
				if (strcmp(opcode,"END"))
				{
					if (label[0] != '\0')
					{
						//���̺��� symbol table�� �����ϴ��� Ȯ��
						//���̺��� �����ϸ� �ߺ� �ɺ��̹Ƿ� ���� ��� �� ���� ����
						if (SearchSymtab(label))
						{
							fclose(fptr);
							printf("ERROE: Duplicate Symbol\n");
							FoundOnSymtab_flag = 0;
							exit(1);
						}
						//symbol table�� ���̺� ����
						RecordSymtab(label);
					}

					//format4�� �� ����Ǵ� ����
					if (SearchOptab (opcode) || opcode[0]=='+')
					{
						if(opcode[0]=='+')
						{
							//������ �տ� ���� +�� ������ ���·� ����� �ݺ���
							//�迭�� 0������ ����� +�� �ڿ� ����� �����͸� ������ ��ĭ�� �̵��ؼ� +����
							for(i=0; i<strlen(opcode)-1; i++)
								opcode[i]=opcode[i+1];
							opcode[strlen(opcode)-1]='\0';

							//�����ڰ� OPTAB�� �����ϴ��� Ȯ��
							//format4�̹Ƿ� ���̴� 4byte ����
							//�߰����� formType�� format4�� �� ����
							SearchOptab(opcode);
							LOCCTR[LocctrCounter] = loc + 4;
							IMRArray[ArrayIndex]->formType=formatFour;
						}
						//������ ������ ö�ڿ� R�� �پ������鼭 OR�� �ƴ� �� ����
						//OR�� format3�̹Ƿ� �ش� �������� ����Ǹ� �ȵ�.
						//������ ������ ö�ڿ� R�� �پ������� �������� ����.
						//�������� ������ format2�̹Ƿ� ���� 2��ŭ ����
						//�߰����� formType�� format2�� �� ����
						else if(opcode[strlen(opcode)-1]=='R' && strcmp(opcode, "OR"))
						{
							LOCCTR[LocctrCounter] = loc + 2;
							IMRArray[ArrayIndex]->formType=formatTwo;
						}
						//������ ������ ö�ڿ� R�� �� ���� format2 ��ɾ� ó�� ����
						//�߰����� formType�� format2�� �� ����
						else if(!strcmp(opcode, "SHIFTL") || !strcmp(opcode, "RMO") || !strcmp(opcode, "SVC"))
						{
							LOCCTR[LocctrCounter] = loc + 2;
							IMRArray[ArrayIndex]->formType=formatTwo;
						}
						//format1 ������ ó�� ����
						//������ ������ ö�ڰ� O�̰ų� FIX, FLOAT, NORM�� �� ����
						//�߰����� formType�� format1�� �� ����
						//format1�̹Ƿ� ���� 1��ŭ ����
						else if(opcode[strlen(opcode)-1]=='O' || !strcmp(opcode, "FIX") || !strcmp(opcode, "FLOAT") || !strcmp(opcode, "NORM"))
						{
							LOCCTR[LocctrCounter] = loc + 1;
							IMRArray[ArrayIndex]->formType=formatOne;
						}
						//format3 ó�� ����
						//OPTAB�� ����Ǿ��ִ� format�� �о�� ������ ��ȯ�� ���� location counter�� ����
						//�߰����� formType�� format3�� �� ����
						else
						{
							LOCCTR[LocctrCounter] = loc + (int)(OPTAB[Counter].Format-'0') ;
							IMRArray[ArrayIndex]->formType=formatThree;
						}
					}
					//���� �����ڰ� �ƴ� �޸� �������� ��� ����
					//WORD�� SICXE���� 3byte�� ǥ���ǹǷ� ���� 3 ����
					//RESW�� �ǿ����ڿ� ����� ��*3��ŭ ���� ����
					//RESB�� �ǿ����ڿ� ����� ��*1��ŭ ���� ����
					//BYTE �������� �ǿ����ڴ� C' �Ǵ� X'�� ���� ���̹Ƿ� C'---', X'--' ������ ���� ���� �� location counter�� ���� 
					else if (!strcmp(opcode,"WORD"))
						LOCCTR[LocctrCounter] = loc + 3;
					else if (!strcmp(opcode,"RESW"))
						LOCCTR[LocctrCounter] = loc + 3 * StrToDec(operand);
					else if (!strcmp(opcode,"RESB"))
						LOCCTR[LocctrCounter] = loc + StrToDec(operand);
					else if (!strcmp(opcode,"BYTE"))
						LOCCTR[LocctrCounter] = loc + ComputeLen(operand);
					else{//����ó��
						fclose(fptr);
						printf("ERROE: Invalid Operation Code\n");
						exit(1);
					}
				}
			}
			//START�� ������ ��ɾ� ���̸� �߰����Ͽ� �����ϱ� ���ؼ� LocctrCounter-1�� ��.
			loc = LOCCTR[LocctrCounter];
			IMRArray[ArrayIndex]->Loc = LOCCTR[LocctrCounter-1];
			LocctrCounter++;
			ArrayIndex++;
		}
		FoundOnOptab_flag = 0;
		line += 1;
	}
	//���α׷� ��ü ���� ���
	//�ݺ����� ����� �� LocctrCounter�� 1�����ϸ鼭 ����Ǿ����Ƿ� ������ ��ɾ��� ��ġ�� LocctrCounter-2�� �ؾ� ��.
	program_length = LOCCTR[LocctrCounter-2]- LOCCTR[0];

	//symbol table ���
	//���̺�� �ּҸ� ����Ѵ�.
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

	//intermediate stucture array�� ���̸�ŭ �ݺ�
	for (loop = 1; loop<ArrayIndex; loop++){
		inst_fmt_opcode = 0;
		inst_fmt_index = 0;
		inst_fmt_address = 0;
		IMRArray[loop]->FloatFlag = 0;

		strcpy(opcode,IMRArray[loop]->OperatorField);

		//�����ڰ� OPTAB�� �����ϸ� ����
		if (SearchOptab(opcode) || opcode[0]=='+'){
			//format4�� ��� ����
			if(opcode[0]=='+' && IMRArray[loop]->formType==formatFour)
			{
				//������ �տ� ���� +�� ������ ���·� ����� �ݺ���
				//�迭�� 0������ ����� +�� �ڿ� ����� �����͸� ������ ��ĭ�� �̵��ؼ� +����
				for(i=0; i<strlen(opcode)-1; i++)
						opcode[i]=opcode[i+1];
				opcode[strlen(opcode)-1]='\0';
				
				//�����ڸ� OPTAB���� �˻��� �� �������� �ӽ��ڵ�� 3h�� ���� ���� �ӽú����� ����
				//3h�� ���ϴ� ������ SICXE��ɾ��� opcode 8bit���� n , i flag�� �׻� 1�̹Ƿ� 3�� ���Ѵ�.
				//�ӽú����� ����� opcode�� �������� 24bit��ŭ �̵��Ѵ�.
				//�ӽú����� ����� opcode�� �߰����Ͽ� �����Ѵ�.
				//�ǿ����ڸ� �߰����Ͽ��� �о�� ���� operand������ ����
				//�ǿ����ڰ� operand,X �����̸� indexed addressing�̹Ƿ� �÷����� ���� 9�� ��
				//operand,X ���°� �ƴϸ� �÷����� ���� 1�� ��
				//�ǿ����ڸ� SYMTAB���� �˻��� ���� SYMTAB�� ����� �ּҸ� inst_fmt_address�� ����
				//inst_fmt_opcode + inst_fmt_index + inst_fmt_address�� ���ϸ� �����ڵ尡 �ȴ�.
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
			//format2�� ��� ����
			//format2�� 16bit�̹Ƿ� opcode�� 8bit �������� �̵� 
			//�������� ������ �ǿ����ڰ� 2���̰ų� 1���� ��찡 �����Ѵ�.
			//�ǿ����ڷ� ���� �������͸� REGTAB���� �˻��Ѵ�.
			//�ǿ����� �������Ͱ� 2���� ��� ù ��° �������ʹ� ���������� �����ٰ� 16�� ���Ѵ�.
			//�� ��° �������ʹ� ���������� �����ٰ� 1�� ���Ѵ�. 2���� ���� ���� �� address�� ����
			//16�� 1�� ���ϴ� ������ operand field�� 1byte�ε� ù ��° �������ʹ� ���� 4bit, �� ��° �������ʹ� ���� 4bit�� ����Ǳ� �����̴�.
			//�ǿ����ڰ� 1���� ��� REGTAB���� �������͸� �˻��� �� ���������� ���� 16�� ���� ���� address�� ����
			//16�� ���ϴ� ������ �������Ͱ� 1���� �� 1byte���� ���� 4bit�� ���� ����ǰ� ���� 4bit�� 0���� ä������ �����̴�.
			//inst_fmt_opcode+inst_fmt_address�� ���� �����ڵ尡 �ȴ�. format2�� �÷��װ� �����Ƿ� inst_fmt_index�� ����.
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
			//format2�� ��� ����
			//format2�� 16bit�̹Ƿ� opcode�� 8bit �������� �̵� 
			//SHIFTL, RMO, SVC ������ �ǿ����ڰ� 2���̰ų� 1���� ��찡 �����Ѵ�.
			//�ǿ����ڷ� ���� �������͸� REGTAB���� �˻��Ѵ�.
			//�ǿ����� �������Ͱ� 2���� ��� ù ��° �������ʹ� ���������� �����ٰ� 16�� ���Ѵ�.
			//�� ��° �������ʹ� ���������� �����ٰ� 1�� ���Ѵ�. 2���� ���� ���� �� address�� ����
			//16�� 1�� ���ϴ� ������ operand field�� 1byte�ε� ù ��° �������ʹ� ���� 4bit, �� ��° �������ʹ� ���� 4bit�� ����Ǳ� �����̴�.
			//�ǿ����ڰ� 1���� ��� REGTAB���� �������͸� �˻��� �� ���������� ���� 16�� ���� ���� address�� ����
			//16�� ���ϴ� ������ �������Ͱ� 1���� �� 1byte���� ���� 4bit�� ���� ����ǰ� ���� 4bit�� 0���� ä������ �����̴�.
			//inst_fmt_opcode+inst_fmt_address�� ���� �����ڵ尡 �ȴ�. format2�� �÷��װ� �����Ƿ� inst_fmt_index�� ����.
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
			//format1�� ��� ����
			//format1�� 1byte�̹Ƿ� OPTAB�� machine code�� �����ڵ尡 �ȴ�.
			else if((opcode[strlen(opcode)-1]=='O' || !strcmp(opcode, "FIX") || !strcmp(opcode, "FLOAT") || !strcmp(opcode, "NORM"))
				&& IMRArray[loop]->formType==formatOne)
			{
				inst_fmt_opcode = OPTAB[Counter].ManchineCode;
				IMRArray[loop]->ObjectCode = inst_fmt_opcode;	

				inst_fmt=inst_fmt_opcode;
				IMRArray[loop]->ObjectCode=inst_fmt;
			}
			//format3�� ��� ����
			//opcode�� SICXE�̹Ƿ� �����ڵ�+3h�� �ؼ� ���Ѵ�.
			//foramt3�� 24bit�� ����ؼ� ��ɾ ǥ���ϹǷ� opcode�� 16bit �������� �̵��Ѵ�.
			//opcode�� RSUB�� ��쿡�� �÷��� ��Ʈ�� TA�� �ʿ����. �����ڵ尡 �����ڵ尡 �ȴ�.
			//format3�� relative-addressing���� �����ǹǷ� �ش� ���α׷������� pc-relative addressing�� ����ߴ�.
			//TA�� pc�� ����Ű�� �ּҸ� ���� relative-address�� ���ߴ�.
			//operand,X�� ��� indexed addressing�̹Ƿ� �÷��״� 8+2=A�� �ȴ�.
			//operand,X�� �ƴ� ��� �÷��״� 2�� �ȴ�.
			//inst_fmt_opcode+inst_fmt_index+inst_fmt_address�� ���� �����ڵ尡 �ȴ�.
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
		//�����ڰ� WORD�� ��� ����
		//�ǿ����ڸ� �߰����Ͽ��� ������ ���� �ǿ����� ���ο� '.'�� �ִ��� Ȯ��
		//'.'�� �����ϸ� �ǿ������� ���� �Ǽ��̴�.
		//strchr�� �Ǽ��� �Ǻ��� �� �Ǽ��̸� �ǿ����ڸ� atof�� �Ǽ��� ��ȯ�� ���� �߰����� FloatNum�� ����
		//�Ǽ��̸� �߰����� FloatFlag�� 1�� ��
		//�Ǽ��� �ƴϸ� �ǿ����ڸ� ������ ��ȯ�� ���� �߰����� ObjectCode�� ����
		else if (!strcmp(opcode, "WORD")){
			strcpy(operand,IMRArray[loop]->OperandField);
			if(strchr(operand,'.'))
			{
				IMRArray[loop]->FloatNum=atof(operand);
				IMRArray[loop]->FloatFlag=1;
			}
			else
			{
				IMRArray[loop]->ObjectCode = StrToDec(operand);//float ������ ��
			}
		}
		//�����ڰ� BYTE�� ��� ����
		//BYTE C'---' ������ ��� for���� ����ؼ� C'---' ������ ���� int������ ��ȯ�� ���� ObjectCode�� ����
		//ObjectCode�� ����� ���� �������� 8bit �̵��Ѵ�. �̵��ϴ� ������ C'�� ���ֱ� ���ؼ�
		//BYTE X'--' ������ ��� X'--' ������ ���� hex�� ��ȯ�� ���� ObjectCode�� �����Ѵ�.
		//ObjectCode�� ����� ���� �������� 8bit �̵��Ѵ�. �̵��ϴ� ������ X'�� ���ֱ� ���ؼ�
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
				operand_ptr = &operand[2];//X' ������ ���� ����Ŵ
				*(operand_ptr+2)='\0';//X'--'���� ������ ' �� ����
				for (x=2; x<=(int)(strlen(operand)-2); x++){
					IMRArray[loop]->ObjectCode=IMRArray[loop]->ObjectCode + StrToHex(operand_ptr);
					IMRArray[loop]->ObjectCode<<=8;
				}
			}
			//�������� 8bit �̵��� ���� ���� ���·� ����
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
	fclose(fptr);//���� ����
}