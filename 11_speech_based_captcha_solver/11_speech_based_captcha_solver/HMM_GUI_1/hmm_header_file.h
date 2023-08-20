#include "stdafx.h"

#include<stdio.h>
#include<conio.h>
#include<stdlib.h>
#include<math.h>

#define N 5			
#define M 32	
long double pi[N+1] = {0.0, 1.0, 0.0, 0.0, 0.0, 0.0};
int Psi[150][N+1];
long double Alpha[150][N+1];
long double Beta[150][N+1];
long double A[N+1][N+1];
long double B[N+1][M+1];
long double Gamma[150][N+1];
long double Delta[150][N+1];
long double Xi[150][N+1][N+1];
long double pi_bar[N+1];
long double A_bar[N+1][N+1];
long double B_bar[N+1][M+1];
long double Avg_pi[10][N+1];
long double Avg_A[10][N+1][N+1];
long double Avg_B[10][N+1][M+1];
long double P[100];
long double Pstar=0;
int qstar[150];	
int T=0;
int O[150];								
double codebook[33][13];				
double sample[500000];				
int n = 0;	
int p=12;
int q=12;
double frame_ci[150][13];
double r[13];								
double a[13];								
double c[13];													
double tokhura_weights[13] = {0.0,1.0, 3.0, 7.0, 13.0, 19.0, 22.0, 25.0, 33.0, 42.0, 50.0, 56.0, 61.0};	
int num_correct_outputs;
double ct[13];							
double overall_accuracy=0.0;		
double dist_cepstral[50];
double accuracy_per_digit[10];

int get_rand_num(){
	return rand() % 10;
}


void read_codebook(){ 	
	FILE *myFile = fopen("codebook.txt","r");	
	int i,j;
	
	for(;!feof(myFile);)
	{
		int i=1;
		while(i<=32)
		{
			int j=1;
			while(j<=12)
			{
				fscanf(myFile,"%lf",&codebook[i][j]);
				j++;
			}
			i++;
		}
	}
	fclose(myFile);	
}

int energy(double *a){
	double noise_energy=0.0;
	double energy=0.0;
	int *ptr;
	double temp1=0.0;
	int i=0,j;
	double temp2=0.0,temp3=0.0;
	while(i<1000)
	{
		noise_energy+=(a[i]*a[i]);
		i++;
	}
	i=20;
	noise_energy/=1000;
	while(i<340)
	{
		temp1+=(a[i]*a[i]);
		i++;
	}
	i=340;
	temp1/=320;
	while(i<640)
	{
		temp2+=(a[i]*a[i]);
		i++;
	}
	i=660;
	temp2/=320;
	while(i<980)
	{
		temp3+=(a[i]*a[i]);
		i++;
	}
	i=0;
	temp3/=320;
	j=980;
	while(1){
		int aa = temp1>1000;
		int bb = temp2>1000;
		int cc = temp3>1000;
		if(aa && bb && cc)
		break;
		temp1=temp2;
		temp2=temp3;
		temp3=0.0;
		i=j;
		while(i<j+320)
		{
			temp3+=(a[i]*a[i]);
			i++;
		}	
		temp3/=320;
		j=j+320;
	}
	return j-320;
}

// Resets the values of variables required in accuracy calculation
void reset_acc_calculations(){
	int i=0;
	while(i<=9)
	{
		accuracy_per_digit[i] = 0.0;
		i++;
	}
	overall_accuracy = 0.0;
	num_correct_outputs = 0;
}

double get_overall_accuracy(){
	return overall_accuracy;
}

//	Loads the initial model
void load_initial_model(){
	FILE *myFile=fopen("A_initial.txt","r");
	int i=1,j=1;

	while(i<=N)
	{
		j=1;
		while(j<=N)
		{
			fscanf(myFile,"%Lf",&A[i][j]);
			j++;
		}
		i++;
	}
	fclose(myFile);

	myFile=fopen("B_initial.txt","r");
	i=1;
	j=1;
	while(i<=N)
	{
		j=1;
		while(j<=M)
		{
			fscanf(myFile,"%Lf",&B[i][j]);
			j++;
		}
		i++;
	}
	fclose(myFile);
}


//	Loads the average model 
void load_average_model(int digit,int iter){
	int i=1;
	char filename[100];
	sprintf(filename,"lambdas/word%d_A%d_matrix.txt",digit,iter);
	int j=1;
	FILE *myFile = fopen(filename, "r");
	
	while(i<=N)
	{
		j=1;
		while(j<=N)
		{
			fscanf(myFile,"%Le",&A[i][j]);
			j++;
		}
		i++;
	}

	fclose(myFile);

	sprintf(filename,"lambdas/word%d_B%d_matrix.txt",digit,iter);
	myFile = fopen(filename, "r");
	
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=M)
		{
			fscanf(myFile,"%Le",&B[i][j]);
			j++;
		}
		i++;
	}
	fclose(myFile);
}


//	resets the model to zero
void reset_model(){
	int i,j;

	i=1;
	while(i<=N)
	{
		pi_bar[i]=0;
		i++;
	}
	
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=N)
		{
			A_bar[i][j]=0;
			j++;
		}
		i++;
	}
	
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=M)
		{
			B_bar[i][j]=0;
			j++;
		}
		i++;
	}
}

//	Perform DC shift and normalization on the recorded data
void preprocess(double arr[], int n){
  double peak_value=fabs(arr[0]);
  int i=0;
  while(i<n)
  {
	  if(fabs(arr[i]) > peak_value){
		peak_value=fabs(arr[i]);
		}
	  i++;
  }

  double normalization_factor = 5000.0 / peak_value, dc_shift = 0.0;

  for(i=0;i<n;i++)
  {
	dc_shift += arr[i];
  }
  dc_shift /= (double)n;

  i=0;
  while(i<n)
  {
	  arr[i] -= dc_shift;
	  arr[i] *= normalization_factor;
	  i++;
  }
}

//	Computes Ri values
void compute_Ri(double arr[],double R[]){
  int i,j;

  i=0;
  while(i<=p)
  {
	  R[i]=0;
	  j=0;
	  while(j<=319-i)
	  {
			R[i]+=arr[j]*arr[i+j];
			j++;
	  }
	  i++;
  }
}

//	Compute Ai values
void compute_Ai(double r[],double a[]){
	double e[13];
	int *ptr;
	double k[13];
	int j=1;
	double x[13][13];
	int i=1;
	double s=0.0;

	e[0]=r[0];

	i=1;
	while(i<=p)
	{
		j=1;
		while(j<=i-1)
		{
			s+=x[i-1][j]*r[i-j];
			j++;
		}
		double div=e[i-1];
		k[i]=(r[i]-s)/div;
		x[i][i]=k[i];	
		
		j=1;
		while(j<=(i-1))
		{
			double temp = k[i]*x[i-1][i-j];
			x[i][j]=x[i-1][j]-temp;
			j++;
		}
		e[i]=(1-k[i]*k[i])*e[i-1];
		s=0.0;
		i++;
	}
		
	i=1;
	while(i<=p)
	{
		a[i]=x[p][i];
		i++;
	}
}

//	Compute Ci values
void compute_Ci(double a[],double c[]){
  double s = 0.0;
  int i=1;
  int j=1;
  c[0] = log10(r[0]*r[0]);

  i=1;
  while(i<=p)
  {
	  j=1;
	  while(j<=i-1)
	  {
		  s+=((double)j/(double)i)*c[j]*a[i-j];
		  j++;
	  }
	c[i]=a[i]+s;
	s=0;
	i++;
  }
}


//	Calculates the tokhura distance
void calculate_tokhura_dist(double ct[150][13]){
	int i,j,t;
	t=1;
	while(t<=T)
	{
		i=1;
		while(i<=32)
		{
			double dist=0;
			j=1;
			while(j<=12)
			{
				double temp1=ct[t][j]-codebook[i][j];
				double temp2 =ct[t][j]-codebook[i][j];
				dist+=tokhura_weights[j] * temp1*temp2;
				j++;
			}
			dist_cepstral[i]=dist;
			i++;
		}

		double min_val=dist_cepstral[1];
		int min_idx=1;
		i=2;
		while(i<=32)
		{
			if(dist_cepstral[i]<min_val){
				min_val=dist_cepstral[i];
				min_idx=i;
			}
			i++;
		}
		O[t]=min_idx;
		t++;
	}
}

//	Computes the Alpha matrix
void forward_procedure(){
	long double Prob=0.0;
	int i,j,t;

	i=1;
	while(i<=N)
	{
		Alpha[1][i]=pi[i]*B[i][O[1]];
		i++;
	}

	t=1;
	while(t<=T-1)
	{
		j=1;
		while(j<=N)
		{
			long double summation=0.0;
			i=1;
			while(i<=N)
			{
				summation+=Alpha[t][i]*A[i][j];
				i++;
			}
			Alpha[t+1][j]=summation * B[j][O[t+1]];
			j++;
		}
		t++;
	}
	
	i=1;
	while(i<=N)
	{
		Prob+=Alpha[T][i];
		i++;
	}
}

//	Computes the Beta matrix
void backward_procedure(){
	int i,j,t;

	i=1;
	while(i<=N)
	{
		Beta[T][i]=1.0;
		i++;
	}

	t=T-1;
	while(t>=1)
	{
		i=1;
		while(i<=N)
		{
			long double summation=0;
			j=1;
			while(j<=N)
			{
				long double temp = A[i][j]*B[j][O[t+1]];
				summation+=temp*Beta[t+1][j];
				j++;
			}
			Beta[t][i]=summation;
			i++;
		}
		t--;
	}
}


//	computes the Gamma matrix
void calculate_gamma(){
  int i,j,t;

  t=1;
  while(t<=T)
  {
	  j=1;
	  while(j<=N)
	  {
		  long double numerator = Alpha[t][j]*Beta[t][j];
		long double denominator = 0.0;
		i=1;
		while(i<=N)
		{
			denominator+=Alpha[t][i]*Beta[t][i];
			 i++;
		}
		Gamma[t][j] = numerator/denominator;
		j++;
	  }
	  t++;
  }
}


//	Vitarbi Algorithm
void viterbi_algo(){
	int i,j=1,t;

	//	Initialization
	i=1;
	while(i<=N)
	{
		Delta[1][i] = pi[i]*B[i][O[1]];
		j++;
		Psi[1][i] = 0;
		i++;
	}


	// Recursion
	t=2;
	while(t<=T)
	{
		j=1;
		while(j<=N)
		{
			long double temp = 0.0, curr_max = 0.0;
			int idx = 0;
			i=1;
			while(i<=N)
			{
				temp = Delta[t-1][i] * A[i][j];
				curr_max=(temp>=curr_max)?temp:curr_max;
				idx=(temp>=curr_max)?i:idx;
				i++;
			}
			long double tempo=B[j][O[t]];
			Delta[t][j]= curr_max*tempo;
			Psi[t][j]=idx;
			j++;
		}
		t++;
	}

	// Termination
	long double curr_max = 0.0;
	i=1;
	while(i<=N)
	{ 
		long double temp = Delta[T][i];
		if(temp<curr_max){}
		else if(temp>=curr_max){
		  curr_max = temp;
		  qstar[T] = i;
		}
		Pstar=curr_max;
		i++;
	}

	//	Path Backtracking
	t=T-1;
	while(t>=1)
	{
		qstar[t] = Psi[t+1][qstar[t+1]];
		t--;
	}
}

void re_estimation(){
	int i,j,t,k;
	
	t=1;
	while(t<=T)
	{
		long double summation = 0.0;
		i=1;
		while(i<=N)
		{
			j=1;
			while(j<=N)
			{
				long double temp=A[i][j]*B[j][O[t+1]];
				summation+=Alpha[t][i]*temp*Beta[t+1][j];
				j++;
			}
			i++;
		}
		i=1;
		while(i<=N)
		{
			j=1;
			while(j<=N)
			{
				long double temp=A[i][j]*B[j][O[t+1]];
				Xi[t][i][j] = Alpha[t][i]*temp*Beta[t+1][j] / summation;
				j++;
			}
			i++;
		}
		t++;
	}


	i=1;
	while(i<=N)
	{
		pi[i]=Gamma[1][i];
		i++;
	}

	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=N)
		{
			long double numerator = 0.0,denominator = 0.0;
			t=1;
			while(t<=T-1)
			{
				numerator+=Xi[t][i][j];
				denominator+=Gamma[t][i];
				t++;
			}
			A[i][j]=numerator/denominator;
			j++;
		}
		i++;
	}

	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=M)
		{
			long double numerator = 0.0,denominator = 0.0;
			t=1;
			while(t<=T)
			{
				if(O[t]==j){
					numerator+=Gamma[t][i];
				}
				denominator+=Gamma[t][i];
				t++;
			}
			B[i][j] = numerator/denominator;
			j++;
		}
		i++;
	}
}

//	Makes B stocahstic if its not a stochastic matrix
void make_B_Stochastic(){
	int i,j;

	i=1;
	while(i<=N)
	{
		int num_zeros_num_correct_outputs = 0;
		long double max_val = 0.0;
		int max_val_idx = 0;

		j=1;
		while(j<=M)
		{
			max_val = (B[i][j] > max_val)?B[i][j]:max_val;
			max_val_idx = (B[i][j] > max_val)?j:max_val_idx;

			num_zeros_num_correct_outputs=(B[i][j] == 0)?num_zeros_num_correct_outputs+1:num_zeros_num_correct_outputs;
			if(B[i][j] == 0){
				B[i][j] = pow(10.0,-30);
			}
			j++;
		}
		B[i][max_val_idx]-=num_zeros_num_correct_outputs*pow(10.0,-30);
		i++;
	}
}


//	Saves the model into files
void save_model(int k,int iter){
	int i=1;
	char filename[100];
	int *ptr;
	sprintf(filename,"lambdas/word%d_A%d_matrix.txt",k,(iter+1));
	int j=1;
	FILE *myFile = fopen(filename, "w");

	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=N)
		{
			fprintf(myFile, "%Le\t",Avg_A[k][i][j]);
			j++;
		}
		fprintf(myFile,"\n");
		i++;
	}
	fclose(myFile);

	sprintf(filename,"lambdas/word%d_B%d_matrix.txt",k,(iter+1));
	myFile = fopen(filename, "w");

	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=M)
		{
			fprintf(myFile,"%Le\t", Avg_B[k][i][j]);
			j++;
		}
		fprintf(myFile, "\n");
		i++;
	}
	fclose(myFile);
}

//	Gives model for each utterance
void get_models(){
	int i,j;
	i=1;
	while(i<=20)
	{
		forward_procedure();
		backward_procedure();
		calculate_gamma();
		viterbi_algo();
		re_estimation();
		make_B_Stochastic();
		i++;
	}
	make_B_Stochastic();

	i=1;
	while(i<=N)
	{
		pi_bar[i]+=pi[i];
		i++;
	}
	
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=N)
		{
			A_bar[i][j]+=A[i][j];
			j++;
		}
		i++;
	}
	
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=M)
		{
			B_bar[i][j]+=B[i][j];
			j++;
		}
		i++;
	}
}


//	Gives average model for each digit
void average(int d){
	int i,j;
	
	i=1;
	while(i<=N)
	{
		pi_bar[i]/=20;
		i++;
	}
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=N)
		{
			A_bar[i][j]/=20;
			j++;
		}
		i++;
	}
	
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=M)
		{
			B_bar[i][j]/=20;
			j++;
		}
		i++;
	}
	
	i=1;
	while(i<=N)
	{
		Avg_pi[d][i]=pi_bar[i];
		i++;
	}
	
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=N)
		{
			Avg_A[d][i][j]=A_bar[i][j];
			j++;
		}
		i++;
	}
	
	i=1;
	while(i<=N)
	{
		j=1;
		while(j<=M)
		{
			Avg_B[d][i][j]=B_bar[i][j];
			j++;
		}
		i++;
	}

}


//	Training
void train(int iteration){
	int i=0;
	while(i<=9)
	{
		reset_model();
		int j=1,ii=0;
		while(j<=20)
		{
			if(iteration!=1){
				load_average_model(i,iteration);
			}
			else{
				ii++;
				load_initial_model();
			}
			
			char filename[100];
			sprintf(filename,"Recordings/%d_%d.txt",i,j);

			FILE *myFile = fopen(filename, "r");

			if (myFile){
				n=0;
				for(;!feof(myFile);)
				{
       				fscanf(myFile,"%lf",&sample[n]);
       				n++;
    			}
			}
 			fclose(myFile);
			int *ptr=NULL;
			preprocess(sample,n);
			int start = energy(sample);
			int num_frames = 0;
			double current_frame[320];
			
			//for(int i=1,k=start;k<start+ (320*140) && k<n-8000;i++,k+=320){

			int i=1;
			int k=start;
			while(i<=140 && k<n-8000)
			{
				num_frames++;
				int lmk =0;
				while(lmk<320)
				{
					current_frame[lmk] = sample[lmk+k];
					lmk++;
				}

				compute_Ri(current_frame,r);
 				compute_Ai(r,a);
 				compute_Ci(a,c);

				int j=1;
				while(j<=12)
				{
					frame_ci[i][j]= c[j];
					long double tempo =(1+ (6.0 * sin((3.14*j)/12.0)));
					frame_ci[i][j] *= tempo;
					j++;
				}
				k+=240;
				i++;
			}

			T = num_frames;
			calculate_tokhura_dist(frame_ci);
			get_models();
			j++;
		}
		average(i);
		save_model(i,iteration);
		i++;
	}
	iteration++;
}


void forward_procedure_test(int actual_digit){
	int i,j,t,k;

	k=0;
	while(k<=9)
	{
		long double Prob=0.0;

		i=1;
		while(i<=N)
		{
			long double multi = Avg_B[k][i][O[1]];
			Alpha[1][i]=Avg_pi[k][i]*multi;
			i++;
		}
	
		t=1;
		while(t<=T-1)
		{
			j=1;
			while(j<=N)
			{
				long double summation=0.0;
				i=1;
				while(i<=N)
				{
					summation+=Alpha[t][i]*Avg_A[k][i][j];
					i++;
				}
				long double multi = Avg_B[k][j][O[t+1]];
				int *ptr=NULL;
				Alpha[t+1][j]=summation * multi;
				j++;
			}
			t++;
		}
		
		i=1;
		while(i<=N)
		{
			Prob+=Alpha[T][i];	
			i++;
		}
		P[k]=Prob;
		k++;
	}

	long double max_val=P[0];
	int max_idx=0;
	
	i=1;
	while(i<=9)
	{
		if(P[i]>max_val)
		{
			max_val=P[i];
			max_idx=i;
		}
		i++;
	}
	//printf("%d ",max_idx);
	
	/*
  for(i=0;i<=9;i++){
    fprintf(write_p_values, "P value for %d is - %Le\n",i,P[i]);
  }
  fprintf(write_p_values, "\nActual digit : %d, Predicted digit : %d\n",actual_digit,max_idx);

  fprintf(write_p_values, "--------------------------------------------------------\n\n");
  */
  double addition=1.0;	
  if(max_idx==actual_digit){
		num_correct_outputs++;
		accuracy_per_digit[actual_digit]+=addition;
  }
}

//	Testing using pre-recorded data (seen data)
void test(){
	//printf("Testing with pre recorded data (Previously seen):-\n");
	
	reset_acc_calculations();
	int i=0;
	while(i<=9)
	{
		//printf("\nFor Digit %d:\n",i);
		int j=1;
		while(j<=20)
		{
			char filename[120];
			sprintf(filename,"Recordings/%d_%d.txt",i,j);
			int num = 0,*ptr=NULL;
			FILE *myFile = fopen(filename, "r");
			if (myFile){
				for(;!feof(myFile);)
				{
					fscanf(myFile,"%lf",&sample[num++]);
    			}
 			}
 			fclose(myFile);
			preprocess(sample,num);

			int start = energy(sample);
			int num_frames =0;
			double current_frame[320];

			for(int i=1,k=start;i<=140 && k<n-8000;i++,k+=240){
				num_frames++;
				int lmk=0;
				while(lmk<320)
				{
					current_frame[lmk] = sample[lmk+k];
					lmk++;
				}
			
				compute_Ri(current_frame,r);
 				compute_Ai(r,a);
 				compute_Ci(a,c);

				int j=1;
				while(j<=12)
				{
					frame_ci[i][j]=c[j];
					long double multi=(1+ (6.0 * sin((3.14*j)/12.0)));
					frame_ci[i][j] *= multi;
					j++;
				}	
			}
			
			T=num_frames;
			calculate_tokhura_dist(frame_ci);

			//fprintf(write_p_values, "For file %s\n",filename);

			forward_procedure_test(i);
			j++;
		}
		i++;
	}

	i=0;
	while(i<=9)
	{
		accuracy_per_digit[i]/=20.0;
		accuracy_per_digit[i]*=100.0;
  		//printf("\nAccuracy for digit %d is %.2lf %%",i,accuracy_per_digit[i]);
		i++;
	}

	overall_accuracy=(num_correct_outputs*100.0)/200.0;
	//printf("\nOverall overall_accuracy: %.2lf %%\n",overall_accuracy);
}


//	Testing using pre-recorded data (unseen data)
void test_offline(){
	reset_acc_calculations();
	//printf("Testing with pre recorded data (Previously unseen):-\n");
	int i=0;
	while(i<=9)
	{
		//printf("\nFor Digit %d:\n",i);
		int j=1;
		while(j<=10)
		{
			char filename[120];
			sprintf(filename,"Recordings/test_data/%d_%d.txt",i,j);
			int num = 0,*ptr=NULL;
			FILE *myFile = fopen(filename, "r");
			if (myFile){
				for(;!feof(myFile);){
					fscanf(myFile,"%lf",&sample[num++]);
    			}
 			}
 			fclose(myFile);
		
			preprocess(sample,num);

			int start = energy(sample);
			int num_frames =0;
			double current_frame[320];

			for(int i=1,k=start;i<=140 && k<n-8000;i++,k+=240){
				num_frames++;
				int lmk=0;
				while(lmk<320)
				{
					current_frame[lmk] = sample[lmk+k];
					lmk++;
				}
			
				compute_Ri(current_frame,r);
 				compute_Ai(r,a);
 				compute_Ci(a,c);

				int j=1;
				while(j<=12)
				{
					frame_ci[i][j]=c[j];
					long double multi=(1+ (6.0 * sin((3.14*j)/12.0)));
					frame_ci[i][j] *= multi ;
					j++;
				}	
			}
			
			T=num_frames;
			calculate_tokhura_dist(frame_ci);
			forward_procedure_test(i);
			j++;
		}
		i++;
	}


  i=0;
  while(i<=9)
  {
	   accuracy_per_digit[i]/=10.0;
		accuracy_per_digit[i]*=100.0;
  		//printf("\nAccuracy for digit %d is %.2lf %%",i,accuracy_per_digit[i]);
	  i++;
  }

   overall_accuracy=(num_correct_outputs*100.0)/100.0;
   //printf("\nOverall overall_accuracy: %.2lf %%\n",overall_accuracy);

}


//	Testing using live data
int test_livedata(){
	//reset_acc_calculations();

	system(".\\Recording_module.exe 3 live_data.wav live_data.txt");
	FILE *myFile = fopen("live_data.txt", "r");

	int num = 0;
	
	for(int xx=0;!feof(myFile);xx++)
	{
		fscanf(myFile,"%lf",&sample[num++]);
	}
	
	fclose(myFile);

	preprocess(sample,num);
	int num_frames =0;
	int start = energy(sample);
	int i=1,k=start;
	double current_frame[320];
	while(i<=140 && k<n-8000)
	{
		num_frames++;
		int lmk=0;
		while(lmk<320)
		{
			current_frame[lmk] = sample[lmk+k];
			lmk++;
		}
	
		compute_Ri(current_frame,r);
		compute_Ai(r,a);
		compute_Ci(a,c);

		int j=1;
		while(j<=12)
		{
			frame_ci[i][j]=c[j];
			long double multi =(1+ (6.0 * sin((3.14*j)/12.0)));
			frame_ci[i][j] *= multi;
			j++;
		}
		i++;
		k+=240;
	}
	
	T=num_frames;
	calculate_tokhura_dist(frame_ci);

	int j,t;
	k=0;
	while(k<=9)
	{
		long double temp=0;
		long double Prob=0;

		i=1;
		while(i<=N)
		{
			long double tempo = Avg_B[k][i][O[1]];
			Alpha[1][i]=Avg_pi[k][i]*tempo;
			i++;
		}
	
		t=1;
		while(t<=T-1)
		{
			j=1;
			while(j<=N)
			{
				temp=0;
				i=1;
				while(i<=N)
				{
					temp+=Alpha[t][i]*Avg_A[k][i][j];
					i++;
				}
				temp*=Avg_B[k][j][O[t+1]];
				int *ptr;
				Alpha[t+1][j]=temp;
				j++;
			}
			t++;
		}
	
		i=1;
		while(i<=N)
		{
			Prob+=Alpha[T][i];
			i++;
		}
		P[k]=Prob;
		k++;
	}

	long double max=P[0];
	int maxindex=0;
	
	i=1;
	while(i<=9)
	{
		if(P[i]>max)
		{
			max=P[i];
			maxindex=i;
		}
		i++;
	}
	return maxindex;
}