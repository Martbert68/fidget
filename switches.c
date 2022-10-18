#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/divider.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "pico/util/queue.h"


int going;
typedef struct element { int lamp; int value; } element_t;

int64_t game_timeout(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    going=0;
    return 0;
}

int64_t switch_timeout(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    int *la;
    la=(int *)user_data;
    printf("I am here %d\n",*la);
    *la=0;
    return 0;
}

queue_t sample_fifo;


int sound (int f, int vol)
{
	element_t element;
	element.value=vol; 
	element.lamp=f+10; 
    	if (!queue_try_add(&sample_fifo, &element)) { printf("FIFO was full\n");}
}

int set_lamps (int *lamps)
{
	element_t element;
	int loop;
	for (loop=0;loop<10;loop++) 
	{ 
		element.value=lamps[loop]; 
		element.lamp=loop; 
	    	if (!queue_try_add(&sample_fifo, &element)) { printf("FIFO was full\n");}
	}
}

int clear_lamps (int *lamps)
{
	element_t element;
	int loop;
	for (loop=0;loop<10;loop++) 
	{ 
		lamps[loop]=19;
		element.value=lamps[loop]; 
		element.lamp=loop; 
	    	if (!queue_try_add(&sample_fifo, &element)) { printf("FIFO was full\n");}
	}
}

int clear_lamps_d (int *lamps)
{
	int loop;
	for (loop=0;loop<10;loop++) 
	{ 
		lamps[loop]=19;
	}
}

int set_lamp (int *lamps,int lamp,int value)
{
	element_t element;
	element.value=value; 
	element.lamp=lamp; 
	lamps[lamp]=value;
    	if (!queue_try_add(&sample_fifo, &element)) { printf("FIFO was full\n");}
}

void lamp_thread()
{
    	sleep_ms(500);
	int loop,*lamps,*want,point,aud,freq,tri,amp;
	long bright,to;
    	lamps=(int *)malloc(sizeof(int)*10);
    	want=(int *)malloc(sizeof(int)*11);
	uint64_t delay_time,wanted_time;
	bright=0;
	point=0;
	aud=0;
	tri=0;
	amp=10;
	to=1;
	

	static int vals[]={1,2,4,6,8,10,12,15,18,21,24,28,34,40,50,60,100,200,400,0};

	float p;
	p=1;
	for (loop=0; loop<20; loop++){ want[loop/2]=vals[loop];printf("%d %d\n",loop,vals[loop]);}
    	delay_time=(uint64_t)10;
	wanted_time=time_us_64();
	freq=14+1;
	amp=100;
	int v,fc;
	fc=0;
	int on,off;

	while (1)
	{
		int tick;
		tick=0;
		while (time_us_64()<wanted_time)
		{	
			busy_wait_us (1);
			tick++;
		}	
		wanted_time+=delay_time;

		for (loop=2; loop<12; loop++){ 
			int val;
			val=want[loop-2];
			if (val!=0){ if ((bright)%val==0){gpio_put(loop,1);}else{gpio_put(loop,0);}} else{gpio_put(loop,0);}
		}
		//set_lamps(lamps);
		if (bright%1413==0){
			// check the queue
			int count = queue_get_level(&sample_fifo);
        		if (count) {
            			//printf("Getting %d:\n",  count);
            			if (tick<1){printf("t %d:\n",  tick);}
            			for (; count > 0; count--) {
                			element_t element;
                			queue_remove_blocking(&sample_fifo, &element);
                			//printf("  got %d %d\n", element.value, element.lamp);
					if (element.lamp>10){ 
						freq=element.lamp-10;
						amp=element.value;
						//on=freq;
						on=amp*freq/100;
						//off=(2*freq)-on;
                			//printf("  got %d %d\n", element.value, element.lamp);
					}else{
					want[element.lamp]=vals[element.value];}
				}
			}
		}

		// triange but too noisy
		// tone 1 cycle seconds 1/freq  10000000/9*freq*11*11 
		//
		/*
		v=bright%16;
			fc++;
			if (fc>=freq)
			{
				fc=0;
				tri+=to;
				if (tri>7){tri=6;to=-to;}
				else if (tri<-7){tri=-6;to=-to;}
				aud=7+((tri*amp)/100);
			}
		if (v<=aud) {gpio_put(22,1);}else{gpio_put(22,0);}

			//aud=7+(tri*amp/100);} */
			//
			//
			//
		if (fc<on) { gpio_put(22,1);} else{gpio_put(22,0);}
		fc++;if(fc>(2*freq)){fc=0;}
		bright++;
	}
	free(lamps);
}

int get_switches (int *switches)
{
	int loop,res;
	res=10;
	for (loop=0; loop<10; loop++)
    	{
       		int swtch,val; 
		val=gpio_get(loop+12);
		if (val != switches[loop]){ res=loop;}
		switches[loop]=val;
	}
	return res;
}

int score(int score)
{
	if (score<0){return(0);}
	int loop;
	int lamps[10];
	int dummy[10];
	clear_lamps(lamps);
	for (loop=0;loop<score;loop++) 
	{ 
		int order;
		order=loop;
		if (order<5){order=4-order;}else{ order=14-order;}
		while (lamps[order]>0)
		{
			int v;
			lamps[order]--;
			set_lamp(lamps,order,lamps[order]);
			sleep_ms(60/(score));
		}
	}
	for (loop=0;loop<5;loop++)
	{
	sleep_ms(400);
	clear_lamps(dummy);
	sleep_ms(400);
	set_lamps(lamps);
	}
}

// mole
int game0()
{
	int lamps[10];
	int switches[10];
	int score,loop;
	score=0;
	while (going)
	{
		int rat;
		rat=rand()%10;
		clear_lamps(lamps);
		set_lamp(lamps,rat,1);
		for (loop=0;loop<1000-(score*80);loop++)
		{
			int got;
			got=get_switches(switches);
			if (got==rat)
			{
				score++;
				break;
			}
			sleep_ms(1);
		}
		if (score==10){ going=0;}
	}
	return score;
}
// nands
int game1()
{

	int connection[10][10];
	int source, dest,loop,combo,best,go,solution;
	int lamps[10];
	int switches[10];
	int i,mask,j,connect[10];

	// wire up
	best=0;go=1;
	while (best<1) 
	{
		// we have wiring
		for (dest=0;dest<10;dest++)
		{
			int count=0;
			for (source=0;source<10;source++)
			{
				int con=rand()%2;
				connection[source][dest]=con;
				count+=con;
			}
			if (count==0){ dest--;}
		}		

		//test it
		int sols;
		sols=0;
		for (combo=0;combo<1024;combo++)
		{
			mask=1;
			for (i=0;i<10;i++){ connect[i]=0;}
			for (i=0;i<10;i++)
			{
				int v;
				v=combo&mask;
				mask=mask<<1;	
				if (v)
				{
					for (j=0;j<10;j++)
					{
						if (connection[i][j]){ connect[j]++;}		
					}
				}	
			}
			int ones;
			ones=0;
			for (i=0;i<10;i++)
			{
				if (connect[i]==1)
				{
					ones++;
				}
			}	
			if (ones==10 ){sols++;solution=combo;}
		}	
		if (sols>best){ 
			best=sols; 
			printf("BEST loop %d sols %d\n",loop,sols);}
	}
	// we have a 2 solution matrix
	int clue,finish;
	clue=0;finish=0;
	while (go)
	{
		int got,count;
		count=0;
		got=get_switches(switches);
		if (got<10)
		{
			if (got==0){ 
				add_alarm_in_ms	((uint32_t)20000, switch_timeout, &finish, 1);
				finish++; if (finish>2){ go=0;}
			}else{finish=0;}

			if (got==5){ clue++;
				if (clue>2)
				{
					clue=0;
					int mask=1;
					for (i=0;i<10;i++)
					{		
						int v;
						v=solution&mask;
						mask=mask<<1;	
						if (v){lamps[i]=0; }else{lamps[i]=19;}
					}
					set_lamps(lamps);
					sleep_ms(2000);
				}
			}else{clue==0;}
			int j,i,connect[10];
			for (i=0;i<10;i++){ connect[i]=0;}
			for (i=0;i<10;i++)
			{
				if (switches[i])
				{
					for (j=0;j<10;j++)
					{
						if (connection[i][j]){ connect[j]++;}
					}
				}
			}	
			for (i=0;i<10;i++)
			{
				if (connect[i]==1){lamps[i]=0;count++;}else{lamps[i]=19;}
			}
			set_lamps(lamps);
		}
		sleep_ms(10);
	}
	return -1;
}

// dice
int game2()
{
	int go,lamps[10],switches[10],i,click,j;
	go=1;
	clear_lamps(lamps);
	get_switches(switches);
	click=0;
	while (go)
	{
		sleep_ms(10);
		int got,v,b;
		got=get_switches(switches);
		if (got<10)
		{
			if (got==0){go=0;} // exit
			else if (got==1) //left dice
			{
				v=rand()%6;
				for(i=5;i<10;i++){lamps[i]=19;} // clear other bank
				for (b=10;b<20;b++) // roll
				{
					for(i=0;i<5;i++){lamps[i]=b;if((i+b)%2==0){lamps[i]=19;}}
					sleep_ms(80); set_lamps(lamps);
				}
				for(i=0;i<5;i++){lamps[i]=19;} // clear bank
				if (v==5){ for(i=0;i<5;i++){lamps[i]=0;}}else{lamps[v]=0; }  //set bank
			}else if (got==2) // right dice
		       	{
				v=rand()%6;
				for(i=0;i<5;i++){lamps[i]=19;} // clear other bank
				for (b=10;b<20;b++) // roll
				{
					for(i=5;i<10;i++){lamps[i]=b;if((i+b)%2==0){lamps[i]=19;}}
					sleep_ms(80); set_lamps(lamps);
				}
				for(i=5;i<10;i++){lamps[i]=19;} // set
				if (v==5){ for(i=5;i<10;i++){lamps[i]=0;}}else{lamps[v+5]=0; }
			}	
			else if (got==3) // double dice
			{
				for (b=20;b>0;b--) // roll
				{
					for(i=0;i<10;i++){lamps[i]=b;if((i+b)%2==0){lamps[i]=19;}}
					sleep_ms(50+(2*b)); set_lamps(lamps);
				}
				clear_lamps(lamps);
				v=rand()%6;
				if (v==5){ for(i=0;i<5;i++){lamps[i]=0;}}else{lamps[v]=0; }
				v=rand()%6;
				if (v==5){ for(i=5;i<10;i++){lamps[i]=0;}}else{lamps[v+5]=0; }
			}	
			else if (got==4) // yes/no 
			{
				float speed;
				speed=1;

				for (b=20-rand()%3;b>rand()%2;b--) // roll
				{
					if (b%2==0)
					{
						for (j=0;j<20;j++)
						{
							for(i=0;i<5;i++){lamps[i]=j;} for(i=5;i<10;i++){lamps[i]=19-j;}
							set_lamps(lamps); sleep_ms(speed);
						}
					} else {
						for (j=0;j<20;j++)
						{
							for(i=0;i<5;i++){lamps[i]=19-j;} for(i=5;i<10;i++){lamps[i]=j;}
							set_lamps(lamps); sleep_ms(speed);
						}

					}
					speed*=1.2;
				}
			}
			set_lamps(lamps);
		}
	}
	return -1;
}

// lunar lander
int game3()
{
	int lamps[10],go,scor,got,switches[10];
	float height,velocity,force;
	go=1;

	while (go)
	{
		height=9;
		force=1;
		velocity=0;
		while (height>0)
		{		
			got=get_switches(switches); if (got==0){ go=0;break;}
			if (got==9){ velocity-=force/110;}
			height=height-velocity;
			clear_lamps(lamps);
			if (height<10){set_lamp(lamps,9-height,height);}
			sleep_ms(10);
			velocity+=(force/2500);
		}	
		scor=(velocity)/(0.0086);
		if (scor<0){scor=-scor;}
		scor=10-scor;
		if (scor<0){ scor=0;}
		printf("Velocity %f %d\n",velocity,scor);
		score(scor);
	}
	return -1;
}
// tennis  human / computer
int game4() 
{
	int lamps[10],go,me,comp,got,switches[10],i,j,count,speed,dummy[10];
	go=1;
	clear_lamps(lamps);

	speed=200;
	while (go && speed >50)
	{
		int loc,locd,my_bounce,c_bounce;
		me=0;comp=0;loc=0;count=0;locd=1;my_bounce=0;c_bounce=0;
		while (1)
		{
			got=get_switches(switches); if (got==0){ go=0;break;}
			if (loc==0 && got== 4) // i hit 
			{
				my_bounce=1;
			}
			if (count>speed){ 
				count=0;
				loc+=locd;
				if (loc>9){
					c_bounce=(rand()%(speed))/25;
					if (c_bounce) {
						loc=8;locd=-locd; c_bounce=0;
					}else{
						me++;loc=0; // computer missed back to me
					}
				}
				if (loc<0){
					if (my_bounce) {
						loc=1;locd=-locd; my_bounce=0;
					}else{
						comp++;loc=9; // i missed back to the computer
					}
				}
				if (me==6 || comp==6){ break;}
				for (i=0;i<10;i++){lamps[i]=19;} //clear
				for (i=0;i<comp;i++){lamps[9-i]=8;} // comp score
				for (i=0;i<me;i++){lamps[4-i]=8;} // my score
				if (loc<5){lamps[4-loc]=0;}else{lamps[loc]=0;}
				set_lamps(lamps);
			}	
			sleep_ms(1);
			count++;
		}	

		clear_lamps(dummy);
		clear_lamps(lamps);
		if (comp==6){ for (i=5;i<10;i++){lamps[i]=0;}} 
		else if ( me ==6 ){ for (i=0;i<5;i++){lamps[i]=0;}}
		else return -1;

		for (i=0;i<4;i++)
		{
			sleep_ms(500);
			set_lamps(lamps);
			sleep_ms(500);
			clear_lamps(dummy);
		}
		speed-=20;
	}
	return-1;
}

// memory
int game5()
{
	int lamps[10],click,go,got,switches[10],i,j,cache[10],cachel[10],guessing,show,right,shows,count,timer;
	go=1; click=0;
	clear_lamps(lamps);
	while (go)
	{
		guessing=1;show=10;shows=11;timer=0;
		for (i=0;i<10;i++){ cache[i]=rand()%2;if(cache[i]==0){cachel[i]=19;}else{cachel[i]=0;}}
		while (guessing)
		{
			if ( show==10 || timer==1000){ set_lamps(cachel); sleep_ms(3000);show=0;shows--;timer=0;
			}
			got=get_switches(switches); 
			if (got<10 || timer==0){
				if (got==0){ click++;if (click>3){go=0;break;}}else{click=0;}
				count=0;
				show ++;
				for (i=0;i<10;i++){ if (switches[i]==cache[i]){count++;}}
				for (i=0;i<count;i++){ lamps[i]=9;}
				for (i=0;i<10;i++){ if (switches[i]){lamps[i]=10;}else{lamps[i]=19;}}
				set_lamps(lamps);
				if (count==10 || shows==0){ sleep_ms(1000);guessing=0;}
			}
			sleep_ms(10);
			timer++;
		}
		score(shows);
	}	

	return -1;
}
//metiorites
int game6()
{
	int lamps[10],click,go,got,switches[10],i,j,set,s;
	float metior_speed[4],metior_location[4],metior_bright[4],flamps[20];
	go=1; click=0; set=0;
	clear_lamps(lamps);
	for (i=0;i<4;i++){ metior_speed[i]=0;metior_location[i]=0;metior_bright[i]=0;}
	got=get_switches(switches); 
	s=0;
	while (go)
	{
		//kick off if needed
		got=get_switches(switches); 
		if (got<10){
			if (got==0){ click++;if (click==3){break;}}else{click=0;} // exit route
			for (i=0;i<4;i++){ 
				if (metior_bright[i]<=0)
				{
					if (got<5){
						metior_bright[i]=19;
						metior_location[i]=got;
						metior_speed[i]=0.02*(float)(got+1);
					}else{
						metior_bright[i]=19;
						metior_speed[i]=-0.02*(float)(got-4);
						metior_location[i]=15-got+metior_speed[i];
					}
					break;	
				}
			}	

		}

		// show lamps and collisions
		float dummy[10];
		for (i=0;i<10;i++){ dummy[i]=0;}
		for (i=0;i<4;i++){ 
			if (metior_bright[i]>0)
			{
				int loc;
				loc=metior_location[i];
				if (loc>4){loc=14-loc;}
				dummy[loc]+=metior_bright[i];
			}
		}
		for (i=0;i<10;i++){ if (dummy[i]>19){ lamps[i]=0;}else{ lamps[i]=19-dummy[i];}}
		set_lamps(lamps);

		//move
		for (i=0;i<4;i++){ 
			if (metior_bright[i]>0)
			{
				metior_location[i]+=metior_speed[i];
				if ((int)metior_location[i]>9){ metior_location[i]=0;}
				if (metior_location[i]<0){ metior_location[i]=9.999;}
				metior_bright[i]-=0.001;	
				if (metior_bright[i]<4){metior_bright[i]=0;}
			}
		}

		// collided
		int l;
		for (i=0;i<4;i++){ 
			if (metior_bright[i]>0)
			{
				int j;
				for (j=i+1;j<4;j++)
				{
					if (metior_bright[j]>0)
					{
						float dist;
						dist=metior_location[i]-metior_location[j];
						if ((dist>0 && dist <0.1) || (dist<0 && dist >-0.1))
						{
							// head on collision
							l=metior_location[i];s=100;
							if ((metior_speed[i]>0 && metior_speed[j]<0) || (metior_speed[i]<0 && metior_speed[j]>0))
							{
								float save;
								save=metior_speed[i];
								metior_speed[i]=-(metior_speed[j]*0.995);
								metior_speed[j]=-(save*0.995);
							}	

						}
					}
				}
			}
		}
		sound(100+(l*l),s);s=s*0.99;if (s<10){s=0;}
		//
		//
		sleep_ms(5);
	}

	return -1;
}
//chasers
int game7()
{
	double phi[10],s,phid[10],clip,clipr,clipa,pha[10],phase,beat,beata,last[10],now[10];
	int i,go=1,lamps[10],switches[10],click=0,chaser=0,got,dummy[10];

	got=get_switches(switches);
	clip=1;
	beat=0;beata=2*M_PI/392;

	while (chaser!=-1)
	{
		if (chaser==0)
		{
			go=1;
			for (i=0;i<10;i++){ phi[i]=0;phid[i]=2*M_PI/1300;pha[i]=0;last[i]=19;}
			while (go)
			{
				got=get_switches(switches);
				if (got<10){ 
					if (got==0){ go=0;chaser=-1;}

					//if (got==1){ click++;if (click==3){go=0;chaser=1;}}else{ click=0;}
					//phid[got]*=1.01;}
				}
		
				if (switches[1]){clip=0;}else{clip=1;}
				if (switches[5]){phase=100;}else{phase=10;}
				if (switches[6]) { beat+=beata;}else{beat=0;}
				if (switches[7]) { beata=phid[1]*3.12;}else{beata=phid[1]*0.314;}
				for (i=0;i<10;i++){ 
					float angle;

					if (switches[2])
					{
						phid[i]*=1.001;
						if (phid[i]>1){ phid[i]=2*M_PI/1300;}
					}
					pha[i]=0;
					if (switches[3])
					{
						if (i<5){pha[i]=2*M_PI*(double)i/(phase);}else
						{pha[i]=2*M_PI*(double)(14-i)/(phase);}
					}

					phi[i]+=phid[i];
					angle=phi[i];
					s=sin(angle+pha[i]);


					if (switches[4])
					{
						s=sin(angle-(pha[i]));
					}
					s=s*cos(beat);
					if (s>clip){s=1;} if (s<-clip){s=-1;}
					s=s+1;

					lamps[i]=(s*20)/2;
					if (lamps[i]>19){lamps[i]=19;}
		
				}
				// blurr
				if (switches[8]) { 
					dummy[0]=(lamps[0]+lamps[1]+lamps[5])/3;
					for (i=1;i<4;i++){ dummy[i]=(lamps[i]+lamps[i+1]+lamps[i-1])/3; }
					dummy[4]=(lamps[3]+lamps[9]+lamps[4])/3;
					dummy[5]=(lamps[0]+lamps[6]+lamps[5])/3;
					for (i=6;i<9;i++){ dummy[i]=(lamps[i]+lamps[i+1]+lamps[i-1])/3; }
					dummy[9]=(lamps[8]+lamps[5]+lamps[9])/3;
					for (i=0;i<10;i++){ lamps[i]=dummy[i]; }
				}
				// reverb 
				if (switches[9]) { 
					for (i=0;i<10;i++){ now[i]=lamps[i];}
					for (i=0;i<10;i++){ now[i]=(now[i]+(60*last[i]))/61;}
					for (i=0;i<10;i++){ last[i]=now[i];lamps[i]=now[i];}
				}
				
				set_lamps(lamps);
				sleep_ms(5);
			}
		}	
		if (chaser==1)
		{
			go=1;
			double udf,lrf,uda,lra,s,t;
			udf=2*M_PI/210;
			lrf=2*M_PI/500;
			uda=0;lra=0;s=1;t=1;
			while (go)
			{
				got=get_switches(switches);
				if(got<10)
				{
				if (got==0){ click++;if (click==3){go=0;chaser=-1;}}else{ click=0;}
				if (got==1){ t=t*0.9;}
				if (got==6){ t=t/0.9;}
				}
				uda+=udf;
				lra+=lrf;
				for (i=0;i<10;i++)
				{
					double pha,phd;
					if (i<5){ pha=t*2*M_PI*(double)i/5;}else
					{ pha=t*2*M_PI*(double)(i-5)/5;}
					phd=t*M_PI*(double)(i/5);
					
					s=sin(uda+pha)*cos(lra+phd);
					lamps[i]=((1+s)*20)/2;
				}
				set_lamps(lamps);
				sleep_ms(5);
			}
		}
	}


	return -1;
}
int game8()
{
	sleep_ms(1000);
	return 2;
}
int game9()
{
	sleep_ms(1000);
	return 1;
}

int construct()
{
    	int(*game[10])();
    	game[0]=*game0; game[1]=*game1; game[2]=*game2; game[3]=*game3; game[4]=*game4; 
	game[5]=*game5; game[6]=*game6; game[7]=*game7; game[8]=*game8; game[9]=*game9;

    	int *switches;
    	int *lamps;
    	int running,loop,bright,timeout,clicks,ls;

	running=1;
	timeout=2000;
	clicks=0;
	ls=10;

    	switches=(int *)malloc(sizeof(int)*10);
    	lamps=(int *)malloc(sizeof(int)*10);

	double phi,speed,sd;
	phi=0;speed=10;sd=1;
	int ft,freq;
	float vol;
	ft=1;
	int first;
	first=1;
	freq=1;
	vol=100;
    	while (running)
	{
		int s;
		sleep_ms(50);

		s=get_switches(switches);
		if (s<10){ timeout=0;
			// we got a switch seed the rnd from the time
			if (first)
			{ 	first=0;
				srand(time_us_64());
			}
		}else{ timeout++;}


		if (timeout<1200)
		{
			if (s<10 && !ft)
			{
				clear_lamps(lamps);
				int i;
				for (i=0;i<20;i++)
				{
					set_lamp(lamps,s,i);
					sleep_ms(80);
				}
				going=1;
				add_alarm_in_ms	((uint32_t)20000, game_timeout, NULL, 1);
				clear_lamps(lamps);
				score(game[s]());			
				get_switches(switches);
			}else{ft=0;}
			phi+=(2*M_PI/speed); if (phi>100000){phi=0;}
			if (bright%100){ speed+=sd;}
			if (speed>100){ speed=100;sd=-sd;}
			if (speed<10){ speed=10;sd=-sd;}
			for (loop=0;loop<10;loop++)
			{	
				int l;
				if (loop<5){ l=loop;}else{ l=14-loop;}
				{
					int val;
					val=19*(sin((2*M_PI*(float)loop/80)+phi));
					if (val<0){val=-val;}
					lamps[l]=val;
				}
			}
		}else {
			if (bright%20==0){ lamps[rand()%10]=rand()%20;}else{
				int j;
				for (j=0;j<10;j++)
				{
					lamps[j]=19;
				}
			}
			ft=1;
		}
		set_lamps(lamps);
		bright++;
		vol=vol*0.99;
		if (vol<10){ vol=100;freq=100+rand()%20;}
		//if (freq>200){freq=0;}
		//sound(freq,vol);
	}

	free (switches);
	free (lamps);
}


int main()
{
	const uint LED_PIN = PICO_DEFAULT_LED_PIN;
	const int FIFO_LENGTH = 64;
	element_t element;


	queue_init(&sample_fifo, sizeof(element_t), FIFO_LENGTH);



    stdio_init_all();


    // GPIO initialisation.
    // We will make this GPIO an input, and pull it up by default
    //gpio_init(GPIO);
    //gpio_set_dir(GPIO, GPIO_IN);
    //gpio_pull_up(GPIO);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
	gpio_put( LED_PIN,1);


    int loop;
    for (loop=0; loop<10; loop++)
    {
        int pin; pin=loop+2;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
    }
        gpio_init(22);
        gpio_set_dir(22, GPIO_OUT);

    for (loop=10; loop<20; loop++)
    {
        int pin,on; pin=loop+2;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
	gpio_pull_up(pin);
    }

    sleep_ms(100);

    multicore_launch_core1(lamp_thread);

    while (1)
    {
	    construct();
    }


    puts("Hello, world!");

    return 0;
}
