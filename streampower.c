#include<malloc.h>
#include<math.h>
#include<stdio.h>
#include<stdlib.h>

#define FREE_ARG char*
#define NR_END 1

#define sqrt2 1.414213562373
#define oneoversqrt2 0.707106781186
#define fillincrement 0.01

float **flow1,**flow2,**flow3,**flow4,**flow5,**flow6,**flow7,**flow8,**flow;
float **topo,**topoold,**topo2,**slope,deltax,*ax,*ay,*bx,*by,*cx,*cy,*ux,*uy;
float *rx,*ry,U,K,D,**slope,timestep,*topovec,thresh,thresholdarea;
int *topovecind,lattice_size_x,lattice_size_y,*iup,*idown,*jup,*jdown;

float *vector(nl,nh)
long nh,nl;
/* allocate a float vector with subscript range v[nl..nh] */
{
        float *v;

        v=(float *)malloc((unsigned int) ((nh-nl+1+NR_END)*sizeof(float)));
        return v-nl+NR_END;
}

int *ivector(nl,nh)
long nh,nl;
/* allocate an int vector with subscript range v[nl..nh] */
{
        int *v;

        v=(int *)malloc((unsigned int) ((nh-nl+1+NR_END)*sizeof(int)));
        return v-nl+NR_END;
}

void free_ivector(int *v, long nl, long nh)
/* free an int vector allocated with ivector() */
{
        free((FREE_ARG) (v+nl-NR_END));
}

void free_vector(float *v, long nl, long nh)
/* free an int vector allocated with ivector() */
{
        free((FREE_ARG) (v+nl-NR_END));
}

int **imatrix(long nrl, long nrh, long ncl, long nch)
/* allocate a int matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
	int **m;

	m=(int **) malloc((size_t)((nrow+NR_END)*sizeof(int*)));
	m += NR_END;
	m -= nrl;

	m[nrl]=(int *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(int)));
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

	return m;
}

float **matrix(long nrl, long nrh, long ncl, long nch)
/* allocate a float matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
	float **m;

	m=(float **) malloc((size_t)((nrow+NR_END)*sizeof(float*)));
	m += NR_END;
	m -= nrl;

	m[nrl]=(float *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(float)));
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

	return m;
}

#define MBIG 1000000000
#define MSEED 161803398
#define MZ 0
#define FAC (1.0/MBIG)

float ran3(idum)
int *idum;
{
        static int inext,inextp;
        static long ma[56];
        static int iff=0;
        long mj,mk;
        int i,ii,k;

        if (*idum < 0 || iff == 0) {
                iff=1;
                mj=MSEED-(*idum < 0 ? -*idum : *idum);
                mj %= MBIG;
                ma[55]=mj;
                mk=1;
                for (i=1;i<=54;i++) {
                        ii=(21*i) % 55;
                        ma[ii]=mk;
                        mk=mj-mk;
                        if (mk < MZ) mk += MBIG;
                        mj=ma[ii];
                }
                for (k=1;k<=4;k++)
                        for (i=1;i<=55;i++) {
                                ma[i] -= ma[1+(i+30) % 55];
                                if (ma[i] < MZ) ma[i] += MBIG;
                        }
                inext=0;
                inextp=31;
                *idum=1;
        }
        if (++inext == 56) inext=1;
        if (++inextp == 56) inextp=1;
        mj=ma[inext]-ma[inextp];
        if (mj < MZ) mj += MBIG;
        ma[inext]=mj;
        return mj*FAC;
}

#undef MBIG
#undef MSEED
#undef MZ
#undef FAC

float gasdev(idum)
int *idum;
{
        static int iset=0;
        static float gset;
        float fac,r,v1,v2;
        float ran3();

        if  (iset == 0) {
                do {
                        v1=2.0*ran3(idum)-1.0;
                        v2=2.0*ran3(idum)-1.0;
                        r=v1*v1+v2*v2;
                } while (r >= 1.0);
                fac=sqrt(-2.0*log(r)/r);
                gset=v1*fac;
                iset=1;
                return v2*fac;
        } else {
                iset=0;
                return gset;
        }
}

#define SWAP(a,b) itemp=(a);(a)=(b);(b)=itemp;
#define M 7
#define NSTACK 100000

void indexx(n,arr,indx)
float arr[];
int indx[],n;
{
        unsigned long i,indxt,ir=n,itemp,j,k,l=1;
        int jstack=0,*istack;
        float a;

        istack=ivector(1,NSTACK);
        for (j=1;j<=n;j++) indx[j]=j;
        for (;;) {
                if (ir-l < M) {
                        for (j=l+1;j<=ir;j++) {
                                indxt=indx[j];
                                a=arr[indxt];
                                for (i=j-1;i>=1;i--) {
                                        if (arr[indx[i]] <= a) break;
                                        indx[i+1]=indx[i];
                                }
                                indx[i+1]=indxt;
                        }
                        if (jstack == 0) break;
                        ir=istack[jstack--];
                        l=istack[jstack--];
                } else {
                        k=(l+ir) >> 1;
                        SWAP(indx[k],indx[l+1]);
                        if (arr[indx[l+1]] > arr[indx[ir]]) {
                                SWAP(indx[l+1],indx[ir])
                        }
                        if (arr[indx[l]] > arr[indx[ir]]) {
                                SWAP(indx[l],indx[ir])
                        }
                        if (arr[indx[l+1]] > arr[indx[l]]) {
                                SWAP(indx[l+1],indx[l])
                        }
                        i=l+1;
                        j=ir;
                        indxt=indx[l];
                        a=arr[indxt];
                        for (;;) {
                                do i++; while (arr[indx[i]] < a);
                                do j--; while (arr[indx[j]] > a);
                                if (j < i) break;
                                SWAP(indx[i],indx[j])
                        }
                        indx[l]=indx[j];
                        indx[j]=indxt;
                        jstack += 2;
                        if (ir-i+1 >= j-l) {
                                istack[jstack]=ir;
                                istack[jstack-1]=i;
                                ir=j-1;
                        } else {
                                istack[jstack]=j-1;
                                istack[jstack-1]=l;
                                l=i;
                        }
                }
        }
        free_ivector(istack,1,NSTACK);
}
#undef M
#undef NSTACK
#undef SWAP

void tridag(float a[], float b[], float c[], float r[], float u[],
	unsigned long n)
{
	unsigned long j;
	float bet,*gam;

	gam=vector(1,n);
	u[1]=r[1]/(bet=b[1]);
	for (j=2;j<=n;j++) {
		gam[j]=c[j-1]/bet;
		bet=b[j]-a[j]*gam[j];
		u[j]=(r[j]-a[j]*u[j-1])/bet;
	}
	for (j=(n-1);j>=1;j--)
		u[j] -= gam[j+1]*u[j+1];
	free_vector(gam,1,n);
}

void setupgridneighbors()
{    int i,j;

     idown=ivector(1,lattice_size_x);
     iup=ivector(1,lattice_size_x);
     jup=ivector(1,lattice_size_y);
     jdown=ivector(1,lattice_size_y);
     for (i=1;i<=lattice_size_x;i++)
      {idown[i]=i-1;
       iup[i]=i+1;}
     idown[1]=1;
     iup[lattice_size_x]=lattice_size_x;
     for (j=1;j<=lattice_size_y;j++)
      {jdown[j]=j-1;
       jup[j]=j+1;}
     jdown[1]=1;
     jup[lattice_size_y]=lattice_size_y;
}

void fillinpitsandflats(i,j)
int i,j;
{    float min;

     min=topo[i][j];
     if (topo[iup[i]][j]<min) min=topo[iup[i]][j];
     if (topo[idown[i]][j]<min) min=topo[idown[i]][j];
     if (topo[i][jup[j]]<min) min=topo[i][jup[j]];
     if (topo[i][jdown[j]]<min) min=topo[i][jdown[j]];
     if (topo[iup[i]][jup[j]]<min) min=topo[iup[i]][jup[j]];
     if (topo[idown[i]][jup[j]]<min) min=topo[idown[i]][jup[j]];
     if (topo[idown[i]][jdown[j]]<min) min=topo[idown[i]][jdown[j]];
     if (topo[iup[i]][jdown[j]]<min) min=topo[iup[i]][jdown[j]];
     if ((topo[i][j]<=min)&&(i>1)&&(j>1)&&(i<lattice_size_x)&&(j<lattice_size_y))
      {topo[i][j]=min+fillincrement;
       fillinpitsandflats(i,j);
       fillinpitsandflats(iup[i],j);
       fillinpitsandflats(idown[i],j);
       fillinpitsandflats(i,jup[j]);
       fillinpitsandflats(i,jdown[j]);
       fillinpitsandflats(iup[i],jup[j]);
       fillinpitsandflats(idown[i],jup[j]);
       fillinpitsandflats(idown[i],jdown[j]);
       fillinpitsandflats(iup[i],jdown[j]);}
}

void mfdflowroute(i,j)
int i,j;
{    float tot;

     tot=0;
     if (topo[i][j]>topo[iup[i]][j])
      tot+=pow(topo[i][j]-topo[iup[i]][j],1.1);
     if (topo[i][j]>topo[idown[i]][j])
      tot+=pow(topo[i][j]-topo[idown[i]][j],1.1);
     if (topo[i][j]>topo[i][jup[j]])
      tot+=pow(topo[i][j]-topo[i][jup[j]],1.1);
     if (topo[i][j]>topo[i][jdown[j]])
      tot+=pow(topo[i][j]-topo[i][jdown[j]],1.1);
     if (topo[i][j]>topo[iup[i]][jup[j]])
      tot+=pow((topo[i][j]-topo[iup[i]][jup[j]])*oneoversqrt2,1.1);
     if (topo[i][j]>topo[iup[i]][jdown[j]])
      tot+=pow((topo[i][j]-topo[iup[i]][jdown[j]])*oneoversqrt2,1.1);
     if (topo[i][j]>topo[idown[i]][jup[j]])
      tot+=pow((topo[i][j]-topo[idown[i]][jup[j]])*oneoversqrt2,1.1);
     if (topo[i][j]>topo[idown[i]][jdown[j]])
      tot+=pow((topo[i][j]-topo[idown[i]][jdown[j]])*oneoversqrt2,1.1);
     if (topo[i][j]>topo[iup[i]][j])
      flow1[i][j]=pow(topo[i][j]-topo[iup[i]][j],1.1)/tot;
       else flow1[i][j]=0;
     if (topo[i][j]>topo[idown[i]][j])
      flow2[i][j]=pow(topo[i][j]-topo[idown[i]][j],1.1)/tot;
       else flow2[i][j]=0;
     if (topo[i][j]>topo[i][jup[j]])
      flow3[i][j]=pow(topo[i][j]-topo[i][jup[j]],1.1)/tot;
       else flow3[i][j]=0;
     if (topo[i][j]>topo[i][jdown[j]])
      flow4[i][j]=pow(topo[i][j]-topo[i][jdown[j]],1.1)/tot;
       else flow4[i][j]=0;
     if (topo[i][j]>topo[iup[i]][jup[j]])
      flow5[i][j]=pow((topo[i][j]-topo[iup[i]][jup[j]])*oneoversqrt2,1.1)/tot;
       else flow5[i][j]=0;
     if (topo[i][j]>topo[iup[i]][jdown[j]])
      flow6[i][j]=pow((topo[i][j]-topo[iup[i]][jdown[j]])*oneoversqrt2,1.1)/tot;
       else flow6[i][j]=0;
     if (topo[i][j]>topo[idown[i]][jup[j]])
      flow7[i][j]=pow((topo[i][j]-topo[idown[i]][jup[j]])*oneoversqrt2,1.1)/tot;
       else flow7[i][j]=0;
     if (topo[i][j]>topo[idown[i]][jdown[j]])
      flow8[i][j]=pow((topo[i][j]-topo[idown[i]][jdown[j]])*oneoversqrt2,1.1)/tot;
       else flow8[i][j]=0;
     flow[iup[i]][j]+=flow[i][j]*flow1[i][j];
     flow[idown[i]][j]+=flow[i][j]*flow2[i][j];
     flow[i][jup[j]]+=flow[i][j]*flow3[i][j];
     flow[i][jdown[j]]+=flow[i][j]*flow4[i][j];
     flow[iup[i]][jup[j]]+=flow[i][j]*flow5[i][j];
     flow[iup[i]][jdown[j]]+=flow[i][j]*flow6[i][j];
     flow[idown[i]][jup[j]]+=flow[i][j]*flow7[i][j];
     flow[idown[i]][jdown[j]]+=flow[i][j]*flow8[i][j];
}

void calculatealongchannelslope(i,j)
int i,j;
{    float down;

     down=0;
     if (topo[iup[i]][j]-topo[i][j]<down) down=topo[iup[i]][j]-topo[i][j];
     if (topo[idown[i]][j]-topo[i][j]<down) down=topo[idown[i]][j]-topo[i][j];
     if (topo[i][jup[j]]-topo[i][j]<down) down=topo[i][jup[j]]-topo[i][j];
     if (topo[i][jdown[j]]-topo[i][j]<down) down=topo[i][jdown[j]]-topo[i][j];
     if ((topo[iup[i]][jup[j]]-topo[i][j])*oneoversqrt2<down)
      down=(topo[iup[i]][jup[j]]-topo[i][j])*oneoversqrt2;
     if ((topo[idown[i]][jup[j]]-topo[i][j])*oneoversqrt2<down)
      down=(topo[idown[i]][jup[j]]-topo[i][j])*oneoversqrt2;
     if ((topo[iup[i]][jdown[j]]-topo[i][j])*oneoversqrt2<down)
      down=(topo[iup[i]][jdown[j]]-topo[i][j])*oneoversqrt2;
     if ((topo[idown[i]][jdown[j]]-topo[i][j])*oneoversqrt2<down)
      down=(topo[idown[i]][jdown[j]]-topo[i][j])*oneoversqrt2;
     slope[i][j]=fabs(down)/deltax;
}

void hillslopediffusioninit()
{    int i,j,count;
     float term1;

     ax=vector(1,lattice_size_x);
     ay=vector(1,lattice_size_y);
     bx=vector(1,lattice_size_x);
     by=vector(1,lattice_size_y);
     cx=vector(1,lattice_size_x);
     cy=vector(1,lattice_size_y);
     ux=vector(1,lattice_size_x);
     uy=vector(1,lattice_size_y);
     rx=vector(1,lattice_size_x);
     ry=vector(1,lattice_size_y);
     D=10000000.0;
     count=0;
     term1=D/(deltax*deltax);
     for (i=1;i<=lattice_size_x;i++)
      {ax[i]=-term1;
       cx[i]=-term1;
       bx[i]=4*term1+1;
       if (i==1)
        {bx[i]=1;
         cx[i]=0;}
       if (i==lattice_size_x)
        {bx[i]=1;
         ax[i]=0;}}
     for (j=1;j<=lattice_size_y;j++)
      {ay[j]=-term1;
       cy[j]=-term1;
       by[j]=4*term1+1;
       if (j==1)
        {by[j]=1;
         cy[j]=0;}
       if (j==lattice_size_y)
        {by[j]=1;
         ay[j]=0;}}
     while (count<5)
      {count++;
       for (i=1;i<=lattice_size_x;i++)
        for (j=1;j<=lattice_size_y;j++)
         topo2[i][j]=topo[i][j];
       for (i=1;i<=lattice_size_x;i++)
        {for (j=1;j<=lattice_size_y;j++)
          {ry[j]=term1*(topo[iup[i]][j]+topo[idown[i]][j])+topoold[i][j];
           if (j==1)
            ry[j]=topoold[i][j];
           if (j==lattice_size_y)
            ry[j]=topoold[i][j];}
         tridag(ay,by,cy,ry,uy,lattice_size_y);
         for (j=1;j<=lattice_size_y;j++)
          topo[i][j]=uy[j];}
       for (i=1;i<=lattice_size_x;i++)
        for (j=1;j<=lattice_size_y;j++)
         topo2[i][j]=topo[i][j];
       for (j=1;j<=lattice_size_y;j++)
        {for (i=1;i<=lattice_size_x;i++)
          {rx[i]=term1*(topo[i][jup[j]]+topo[i][jdown[j]])+topoold[i][j];
           if (i==1)
            rx[i]=topoold[i][j];
           if (i==lattice_size_x)
            rx[i]=topoold[i][j];}
         tridag(ax,bx,cx,rx,ux,lattice_size_x);
         for (i=1;i<=lattice_size_x;i++)
          topo[i][j]=ux[i];}}
}

void avalanche(i,j)
int i,j;
{
     if (topo[iup[i]][j]-topo[i][j]>thresh)
      topo[iup[i]][j]=topo[i][j]+thresh;
     if (topo[idown[i]][j]-topo[i][j]>thresh)
      topo[idown[i]][j]=topo[i][j]+thresh;
     if (topo[i][jup[j]]-topo[i][j]>thresh)
      topo[i][jup[j]]=topo[i][j]+thresh;
     if (topo[i][jdown[j]]-topo[i][j]>thresh)
      topo[i][jdown[j]]=topo[i][j]+thresh;
     if (topo[iup[i]][jup[j]]-topo[i][j]>(thresh*sqrt2))
      topo[iup[i]][jup[j]]=topo[i][j]+thresh*sqrt2;
     if (topo[iup[i]][jdown[j]]-topo[i][j]>(thresh*sqrt2))
      topo[iup[i]][jdown[j]]=topo[i][j]+thresh*sqrt2;
     if (topo[idown[i]][jup[j]]-topo[i][j]>(thresh*sqrt2))
      topo[idown[i]][jup[j]]=topo[i][j]+thresh*sqrt2;
     if (topo[idown[i]][jdown[j]]-topo[i][j]>(thresh*sqrt2))
      topo[idown[i]][jdown[j]]=topo[i][j]+thresh*sqrt2;
}

main()
{    FILE *fp1;
     float deltah,time,max,duration;
     int printinterval,idum,i,j,t,step;

     fp1=fopen("streampowertopo","w");
     lattice_size_x=250;
     lattice_size_y=250;
     idum=-678;
     U=1;                /* m/kyr */
     K=0.05;             /* kyr^-1 */
     printinterval=100;
     deltax=200.0;       /* m */
     thresh=0.58*deltax; /* 30 deg */
     timestep=1;         /* kyr */
     duration=100;
     setupgridneighbors();
     topo=matrix(1,lattice_size_x,1,lattice_size_y);
     topo2=matrix(1,lattice_size_x,1,lattice_size_y);
     topoold=matrix(1,lattice_size_x,1,lattice_size_y);
     slope=matrix(1,lattice_size_x,1,lattice_size_y);
     flow=matrix(1,lattice_size_x,1,lattice_size_y);
     flow1=matrix(1,lattice_size_x,1,lattice_size_y);
     flow2=matrix(1,lattice_size_x,1,lattice_size_y);
     flow3=matrix(1,lattice_size_x,1,lattice_size_y);
     flow4=matrix(1,lattice_size_x,1,lattice_size_y);
     flow5=matrix(1,lattice_size_x,1,lattice_size_y);
     flow6=matrix(1,lattice_size_x,1,lattice_size_y);
     flow7=matrix(1,lattice_size_x,1,lattice_size_y);
     flow8=matrix(1,lattice_size_x,1,lattice_size_y);
     topovec=vector(1,lattice_size_x*lattice_size_y);
     topovecind=ivector(1,lattice_size_x*lattice_size_y);
     for (i=1;i<=lattice_size_x;i++)
      for (j=1;j<=lattice_size_y;j++)
       {topo[i][j]=0.5*gasdev(&idum);
        topoold[i][j]=topo[i][j];
        flow[i][j]=1;}
     /*construct diffusional landscape for initial flow routing */
     for (step=1;step<=10;step++)
      {hillslopediffusioninit();
       for (i=2;i<=lattice_size_x-1;i++)
        for (j=2;j<=lattice_size_y-1;j++)
         {topo[i][j]+=0.1;
          topoold[i][j]+=0.1;}}
     time=0;
     while (time<duration)
      {/*perform landsliding*/
       for (j=1;j<=lattice_size_y;j++)
        for (i=1;i<=lattice_size_x;i++)
         topovec[(j-1)*lattice_size_x+i]=topo[i][j];
       indexx(lattice_size_x*lattice_size_y,topovec,topovecind);
       t=0;
       while (t<lattice_size_x*lattice_size_y)
        {t++;
         i=(topovecind[t])%lattice_size_x;
         if (i==0) i=lattice_size_x;
         j=(topovecind[t])/lattice_size_x+1;
         if (i==lattice_size_x) j--;
         avalanche(i,j);}
       for (j=1;j<=lattice_size_y;j++)
        for (i=1;i<=lattice_size_x;i++)
         topoold[i][j]=topo[i][j];
       for (j=1;j<=lattice_size_y;j++)
        for (i=1;i<=lattice_size_x;i++)
         fillinpitsandflats(i,j);
       for (j=1;j<=lattice_size_y;j++)
        for (i=1;i<=lattice_size_x;i++)
         {flow[i][j]=1;
          topovec[(j-1)*lattice_size_x+i]=topo[i][j];}
       indexx(lattice_size_x*lattice_size_y,topovec,topovecind);
       t=lattice_size_x*lattice_size_y+1;
       while (t>1)
        {t--;
         i=(topovecind[t])%lattice_size_x;
         if (i==0) i=lattice_size_x;
         j=(topovecind[t])/lattice_size_x+1;
         if (i==lattice_size_x) j--;
         mfdflowroute(i,j);}
       for (i=2;i<=lattice_size_x-1;i++)
        for (j=2;j<=lattice_size_y-1;j++)
         {topo[i][j]+=U*timestep;
          topoold[i][j]+=U*timestep;}
       /*perform upwind erosion*/
       max=0;
       for (i=2;i<=lattice_size_x-1;i++)
        for (j=2;j<=lattice_size_y-1;j++)
         {calculatealongchannelslope(i,j);
          deltah=timestep*K*sqrt(flow[i][j])*deltax*slope[i][j];
          topo[i][j]-=deltah;
          if (topo[i][j]<0) topo[i][j]=0;
          if (K*sqrt(flow[i][j])*deltax>max) max=K*sqrt(flow[i][j])*deltax;}
       time+=timestep;
       if (max>0.3*deltax/timestep)
        {time-=timestep;
         timestep/=2.0;
         for (i=2;i<=lattice_size_x-1;i++)
          for (j=2;j<=lattice_size_y-1;j++)
           topo[i][j]=topoold[i][j]-U*timestep;}
        else
        {if (max<0.03*deltax/timestep) timestep*=1.2;
         for (j=1;j<=lattice_size_y;j++)
          for (i=1;i<=lattice_size_x;i++)
           topoold[i][j]=topo[i][j];}
       if (time>printinterval)
        {printinterval+=100;
         for (i=1;i<=lattice_size_x;i++)
          for (j=1;j<=lattice_size_y;j++)
           {fprintf(fp1,"%f\n",topo[i][j]);}}}
	   fclose(fp1);
}
