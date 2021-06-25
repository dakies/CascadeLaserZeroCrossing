#include "kernel.cu"
#include "untils.cpp"
#include <cublas.h>

//Variable declarations
// ns: dimension state space, no:dimension observations, Ts sampling rate
int ns = 3, no = 1;
int Ts = 1; 
int dev = 0;

/*
GEMM 
  status = cublasGemm<t>x(handle, CUBLAS_OP_N, CUBLAS_OP_N, N, N, N, &alpha,
                                    d_A, dtypeA, N,
                                    d_B, dtypeB, N,
                          &beta,    d_C, dtypeC, N, cublasComputeTypes[comptype],  CUBLAS_GEMM_DEFAULT_TENSOR_OP);
  if(status != CUBLAS_STATUS_SUCCESS){
    fprintf(stderr, "!!!! kernel execution error.\n");
    return EXIT_FAILURE;
  }
*/

//Memory Allocation
// Todo: Use cudamallochost
memAlloc(&X,ns,1);
memAlloc(&h_X,ns,1);
memAlloc(&P,ns,ns);
memAlloc(&F,ns,ns);
memAlloc(&Z,no,1);
memAlloc(&S,no,no);
memAlloc(&s,no,no);
memAlloc(&si,no,no);
memAlloc(&K,ns,no);
memAlloc(&H,no,ns);
memAlloc(&R,no,no);
memAlloc(&Ft,ns,ns);
memAlloc(&Ht,ns,no);
memAlloc(&Si,no,no);
memAlloc(&Y,no,1);
memAlloc(&I,ns,ns);
memAlloc(&Hint,no,ns);
memAlloc(&Sint,no,no);
memAlloc(&Kint,ns,no);
memAlloc(&Xint,ns,1);
memAlloc(&Pint,ns,ns);
memAlloc(&Pint2,ns,ns);
printf("\nHost allocation is completed...\n");
init(X,P,F,Q,H,R,I,Ht,Ft,ns,no,Ts);

//Device Variable declarations
float *d_X;//Estimate
float *d_P;//Estimate Covariance
float *d_F;//State transition Matrix
float *d_Q
float *d_Z;//Measurement
float *d_S;//Intermediate value
float *d_s;//Intermediate value
float *d_K;//Kalman gain
float *d_H;//Measurement function (linear)
float *d_R;//Measurement noise covaraince
float *d_Ft;//F transpose
float *d_Ht;//H transpose
float *d_Si;//Inverse of S
float *d_Y; //error
float *d_I;//Identity Matrix
float *d_Hint;//for intermediate calculations
float *d_Sint;//for intermediate calculations
float *d_Kint;//for intermediate calculations
float *d_Xint;// for intermediate calculations
float *d_Pint;// for intermediate calculations
float *d_Pint2;// for intermediate calculations
float *d_Ztemp;//to  store temporarily



int main(int argc, char** argv){

	ParseArguments(argc, argv);

	//Allocate vectors  in device memory
	cudaMalloc(&d_X, ns*1);
	cudaMalloc(&d_P, ns*ns);
	cudaMalloc(&d_F, ns*ns);
	cudaMalloc(&d_Q, ns*ns);
	cudaMalloc(&d_Z, no*1);
	cudaMalloc(&d_S, no*no);
	cudaMalloc(&d_s, no*no);
	cudaMalloc(&d_K, ns*no);
	cudaMalloc(&d_H, no*ns);
	cudaMalloc(&d_R, no*no);
	cudaMalloc(&d_Ft, ns*ns);
	cudaMalloc(&d_Ht, ns*no);
	cudaMalloc(&d_Si, no*no);
	cudaMalloc(&d_Y, no*1);
	cudaMalloc(&d_I, ns*ns);
	cudaMalloc(&d_Hint, no*ns);
	cudaMalloc(&d_Sint, no*no);
	cudaMalloc(&d_Kint, ns*no);
	cudaMalloc(&d_Xint, ns*1);
	cudaMalloc(&d_Pint, ns*ns);
	cudaMalloc(&d_Pint2, ns*ns);

	// Copy Input vectors from host memory to device memory
	cudaMemcpy(d_X, X, ns*1, cudaMemcpyHostToDevice);
	cudaMemcpy(d_P, P, ns*ns, cudaMemcpyHostToDevice);
	cudaMemcpy(d_F, F, ns*ns, cudaMemcpyHostToDevice);
	//cudaMemcpy(d_Z, Z, no*1, cudaMemcpyHostToDevice);
	//cudaMemcpy(d_S, S, no*no, cudaMemcpyHostToDevice);
	//cudaMemcpy(d_H, H, no*ns, cudaMemcpyHostToDevice);
	cudaMemcpy(d_Q, Q, ns*ns, cudaMemcpyHostToDevice);
	cudaMemcpy(d_R, R, no*no, cudaMemcpyHostToDevice);
	cudaMemcpy(d_I, I, ns*ns, cudaMemcpyHostToDevice);

	// Set the kernel arguments
	int threadsPerBlock = 256;
	int Nos = no*ns;
	int Ns =  ns;
	int No = no;
	int Ns2 = ns*ns;
	int No2 = no*no;
	int blocksPerGridNos = (Nos + threadsPerBlock - 1) / threadsPerBlock;
	int blocksPerGridNs = (Ns + threadsPerBlock - 1) / threadsPerBlock;
	int blocksPerGridNo = (No + threadsPerBlock - 1) / threadsPerBlock;
	int blocksPerGridNs2 = (Ns2 + threadsPerBlock - 1) / threadsPerBlock;
	int blocksPerGridNo2 = (No2 + threadsPerBlock - 1) / threadsPerBlock;

	//Predict
	// x = Fx
	// P = FPF^t 
		
	// 1)  X = FX
	MatMult<<<blocksPerGridNs2, threadsPerBlock>>>(d_Xint, d_F, d_X, ns, ns);
	MatCopy<<<blocksPerGridNs, threadsPerBlock>>>(d_X, d_Xint, ns, 1);
		
	// 2) P = FPFt + Q
		
	MatMult<<<blocksPerGridNs2, threadsPerBlock>>>(d_Pint, d_F, d_P, ns, ns);
	MatMult<<<blocksPerGridNs2, threadsPerBlock>>>(d_Pint2, d_Pint, d_Ft, ns, ns);
	MatAdd<<<blocksPerGridNs2, threadsPerBlock>>>(d_P, d_Pint2, d_Q)

	//Update
	// 0) H
	CalcH<<<blocksPerGridNos, threadsPerBlock>>>(d_H, d_X);
	MatTranspose<<<blocksPerGridNos, threadsPerBlock>>>(d_Ht, d_H, ns, no);

	// 1) Y = Z - HX
	MatMult<<<blocksPerGridNos, threadsPerBlock>>>(d_Y, d_H, d_X, no, ns);
	MatSub<<<blocksPerGridNo, threadsPerBlock>>>(d_Y, d_Z, d_Y, no, 1);
		
	// 2) S = HPHt + E
		
	MatMult<<<blocksPerGridNos, threadsPerBlock>>>(d_Hint, d_H, d_P, no, ns);
	MatMult<<<blocksPerGridNos, threadsPerBlock>>>(d_Sint, d_Hint, d_Ht, no, ns);
	MatAdd<<<blocksPerGridNo2, threadsPerBlock>>>(d_S, d_Sint, d_R, no, no);
		
	// 3) K = PH^tSi 
	// Todo: How to compute Si???? Just 1d so easy 1/S
	invS<<<blocksPerGridNos, threadsPerBlock>>>(Si, S);
	MatMult<<<blocksPerGridNos, threadsPerBlock>>>(d_Kint, d_P, d_Ht, no, ns);
	MatMult<<<blocksPerGridNos, threadsPerBlock>>>(d_K, d_Kint, d_Si, ns, no);
		
	// 4) X = X + KY

	MatMult<<<blocksPerGridNos, threadsPerBlock>>>(d_Xint, d_K, d_Y, ns, no);
	MatAdd<<<blocksPerGridNs, threadsPerBlock>>>(d_X, d_X, d_Xint, ns, 1);
		
	// 5) [I - KH]P
		
	MatMult<<<blocksPerGridNos, threadsPerBlock>>>(d_Pint, d_K, d_H, ns, no);
	MatSub<<<blocksPerGridNs2, threadsPerBlock>>>(d_Pint, d_I, d_Pint, ns, ns);
	MatMult<<<blocksPerGridNs2, threadsPerBlock>>>(d_Pint2, d_Pint, d_P, ns, ns);
	MatCopy<<<blocksPerGridNs2, threadsPerBlock>>>(d_P, d_Pint2, ns, ns);
		
}
