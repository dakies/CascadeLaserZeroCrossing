#include <stdio.h>
#include <stdlib.h>
#include <cmath>

//Memory Allocation Function
void memAlloc(float **data_ptr, int dim_x, int dim_y)
{
	*data_ptr = (float *) malloc(sizeof(float *) * dim_x * dim_y);
}

//Transpose
void transpose(float* B, float* A,int h, int w)
{
	int i,j;
	for (i = 0; i < h; i++)
		{
		for(j = 0; j < w ;j++)
			{
			B[j * h + i] = A[i * w + j];
			}
		}
}

//Ideintity Matrix Generation
void Identity(float *data, int n)
{
	for (int i = 0; i < (n*n); i=i+1)
		{
		if((i%(n+1))==0)
			data[i] = 1;
		else
			data[i] = 0;
		}        
}


void init(float *X,float *P,float *F,float *Q,float *H,float *R,float *I,float *Ht,float *Ft, int ns, int no, int Ts)
{

	// Initial state X_0
	X[0]=1;
	X[1]=1;
	X[2]=1;

	// Initial coovariance P_0
	Identity(P, ns);

	// F System dynmic matrix of autonomous system
	F[0] = 1;
	F[1] = 2*Ts*M_PI;
	F[2] = 0;
	F[3] = 0;
	F[4] = 1;
	F[5] = 0;
	F[6] = 0;
	F[7] = 0;
	F[8] = 1;
	transpose(Ft,F,ns,ns);

	// Measurement Noise
	R[0] = 1.0;

	// Process Noise
	Identity(P, ns);
	//Helpers
	//  Identity	
	Identity(I, ns);
}