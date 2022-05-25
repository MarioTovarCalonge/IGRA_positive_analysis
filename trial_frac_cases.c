#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> 
#include <time.h> //Used for clock

#define year 365 //days
#define T_MAX 3*year //In days
#define deltat 1 //In days
#define AG 15
#define countries 3
#define N_trials 10000
#define Ncontrol 1639
#define Nvaccine 1626


void start_sto_sim(double *AG_C_SA, double *AG_V_SA, double *AG_C_K, double *AG_V_K, double *AG_C_Z, double *AG_V_Z, int *mode, double *epsilon, double *VEdis);
int eventDrivenTrial(int a, int NL, int NF, int mode, double eps, double beta, double p, double q, double r, double r_L);
double bern_binom(double p, double n);
double gaussrand();

FILE *f;

void start_sto_sim(double *AG_C_SA, double *AG_V_SA, double *AG_C_K, double *AG_V_K, double *AG_C_Z, double *AG_V_Z, int *mode, double *epsilon, double *VEdis){
    int age;
    int scan, i, j, iter;
    int D_c, D_v;
    FILE *fp;
    char aux[100];
    
    double PRC[3][2][AG];
    for(i=0; i<AG; i++){
        PRC[0][0][i] = AG_C_SA[i];
        PRC[0][1][i] = AG_V_SA[i];
        PRC[1][0][i] = AG_C_K[i];
        PRC[1][1][i] = AG_V_K[i];
        PRC[2][0][i] = AG_C_Z[i];
        PRC[2][1][i] = AG_V_Z[i];
    }
    
    int total_population_control = Ncontrol, total_population_vaccine = Nvaccine;
    
    double L_frac[3][301][AG]; //[country][Model iteration][age group]; countries are loaded as 0 -> South Africa, 1 -> Kenya, 2 -> Zambia
    
    //Loading F_L distribution in South Africa;
    fp=fopen("Data_countries/Fracs_INF_SA.txt", "rt");
    for(iter=0; iter<301; iter++){
        for(i=0; i<AG; i++){
            //scan = fscanf(fp, "%lf ",&L_frac_SA[iter][i]);
            scan = fscanf(fp, "%lf ",&L_frac[0][iter][i]);
        }
    }
    fclose(fp);
    
    //Loading F_L distribution in Kenya;
    fp=fopen("Data_countries/Fracs_INF_Kenya.txt", "rt");
    for(iter=0; iter<301; iter++){
        for(i=0; i<AG; i++){
            //scan = fscanf(fp, "%lf ",&L_frac_Kenya[iter][i]);
            scan = fscanf(fp, "%lf ",&L_frac[1][iter][i]);
        }
    }
    fclose(fp);
    
    //Loading F_L distribution in Zambia;
    fp=fopen("Data_countries/Fracs_INF_Zambia.txt", "rt");
    for(iter=0; iter<301; iter++){
        for(i=0; i<AG; i++){
            //scan = fscanf(fp, "%lf ",&L_frac_Zambia[iter][i]);
            scan = fscanf(fp, "%lf ",&L_frac[2][iter][i]);
        }
    }
    fclose(fp);
    
    double param[3][301][6];
    fp=fopen("Data_countries/param_SA.txt", "rt");
    for(iter=0; iter<301; iter++){
        scan = fscanf(fp, "%lf %lf %lf %lf %lf %lf\n", &param[0][iter][0], &param[0][iter][1], &param[0][iter][2], &param[0][iter][3], &param[0][iter][4], &param[0][iter][5]);
    }
    fclose(fp);
    
    fp=fopen("Data_countries/param_Kenya.txt", "rt");
    for(iter=0; iter<301; iter++){
        scan = fscanf(fp, "%lf %lf %lf %lf %lf %lf\n", &param[1][iter][0], &param[1][iter][1], &param[1][iter][2], &param[1][iter][3], &param[1][iter][4], &param[1][iter][5]);
    }
    fclose(fp);
    
    fp=fopen("Data_countries/param_Zambia.txt", "rt");
    for(iter=0; iter<301; iter++){
        scan = fscanf(fp, "%lf %lf %lf %lf %lf %lf\n", &param[2][iter][0], &param[2][iter][1], &param[2][iter][2], &param[2][iter][3], &param[2][iter][4], &param[2][iter][5]);
    }
    fclose(fp);
    
    double beta_inf[3][301][AG];
    //Loading force of infection South Africa;
    fp=fopen("Data_countries/F_INF_SA.txt", "rt");
    for(iter=0; iter<301; iter++){
        for(i=0; i<AG; i++){
            scan = fscanf(fp, "%lf ",&beta_inf[0][iter][i]);
        }
    }
    fclose(fp);
    //Loading force of infection Kenya;
    fp=fopen("Data_countries/F_INF_Kenya.txt", "rt");
    for(iter=0; iter<301; iter++){
        for(i=0; i<AG; i++){
            scan = fscanf(fp, "%lf ",&beta_inf[1][iter][i]);
        }
    }
    fclose(fp);
    //Loading force of infection Zambia;
    fp=fopen("Data_countries/F_INF_Zambia.txt", "rt");
    for(iter=0; iter<301; iter++){
        for(i=0; i<AG; i++){
            scan = fscanf(fp, "%lf ",&beta_inf[2][iter][i]);
        }
    }
    fclose(fp);
    
    //--------------------------------------------------------------------------//
    
    //Initiate C's own random generator.
    srand(time(NULL));
    
    int iter_select;
    double p, q, r, r_L;
    
    double med_D_control = 0;
    int D_cont[N_trials];
    int L_ini = 0, F_ini = 0, calc_aux = 0, L_ini_vac=0, F_ini_vac=0;
    
    int i_c = 0;
    double D_c_age[3], D_v_age[3];
    double rho = 0;
    double efs_con[N_trials], efs_vac[N_trials], F_con[3], F_vac[3];
    
    for(j = 0; j < N_trials; j++){
        rho = 0;
        for(i_c=0; i_c<3; i_c++){
            D_c_age[i_c] = 0;
            D_v_age[i_c] = 0;
            F_con[i_c] = 0;
            F_vac[i_c] = 0;
                
            iter_select = (double)rand()/RAND_MAX*301;
            
            q = param[i_c][iter_select][3];
            r = param[i_c][iter_select][4];
            r_L = param[i_c][iter_select][5];
            
            int aux_D_c = 0.0, aux_D_v = 0.0, aux_starting_pop;
            
            for(age = 3; age < 11; age++){
                
                //Control
                aux_starting_pop = total_population_control; //ages_cont[age];
                calc_aux = round(L_frac[i_c][iter_select][age]/100 * aux_starting_pop); 
                
                if(*mode==1){
                    calc_aux = bern_binom(L_frac[i_c][iter_select][age]/100, aux_starting_pop);
                }
                
                L_ini = calc_aux;
                F_ini = aux_starting_pop - calc_aux;
                
                //Vaccine
                aux_starting_pop = total_population_vaccine; //ages_vac[age];
                calc_aux = round(L_frac[i_c][iter_select][age]/100 * aux_starting_pop); 
                
                if(*mode==1){
                    calc_aux = bern_binom(L_frac[i_c][iter_select][age]/100, aux_starting_pop);
                }
                    
                L_ini_vac = calc_aux;
                F_ini_vac = aux_starting_pop - calc_aux;
                
  
                if(age==0){
                    p = param[i_c][iter_select][0];
                }
                else if(age==1){
                    p = param[i_c][iter_select][1];
                }
                else{
                    p = param[i_c][iter_select][2];
                }
                
                D_c = eventDrivenTrial(age, L_ini, F_ini, 10, 0.0, beta_inf[i_c][iter_select][age], p, q, r, r_L);
                D_v = eventDrivenTrial(age, L_ini_vac, F_ini_vac, *mode, (1.0 - *epsilon), beta_inf[i_c][iter_select][age], p, q, r, r_L);
                
                aux_D_c += D_c;
                aux_D_v += D_v;
                D_c_age[i_c] += (double)D_c*PRC[i_c][0][age];
                D_v_age[i_c] += (double)D_v*PRC[i_c][1][age];
                
                F_con[i_c] += (double)F_ini*PRC[i_c][0][age];
                F_vac[i_c] += (double)F_ini_vac*PRC[i_c][1][age];
            }
        }

        rho = ( 0.803813*D_v_age[0] + 0.1506765*D_v_age[1] + 0.04551046*D_v_age[2] )/(0.808178*D_c_age[0] + 0.1491281*D_c_age[1] + 0.04269393*D_c_age[2])  *  ((double)Ncontrol)/((double)Nvaccine);
        //printf("%lf\n", rho);
        VEdis[j] = 100*(1.0 - rho);
        
        efs_con[j] = 0.808178*F_con[0] + 0.1491281*F_con[1] + 0.04269393*F_con[2];
        efs_vac[j] = 0.803813*F_vac[0] + 0.1506765*F_vac[1] + 0.04551046*F_vac[2];
        
        //VEdis[j] = 100*(1.0 - (double)aux_D_v/(double)aux_D_c);
        //VEdis[j] = 100*(1.0 - D_v_age/D_c_age);
    }
    
    if(*mode==1){
        sprintf(aux, "F_outputs/file_%.3lf.txt", *epsilon);
        f = fopen(aux, "wt");
        for(j=0; j<N_trials; j++){
            fprintf(f, "%lf %lf\n", efs_con[j], efs_vac[j]);
        }
        fclose(f);
    }
    
    //--------------------------------------------------------------------------//
}

double bern_binom(double p, double n){
    int i, c=0;
    double aux;
    for(i=0; i<n; i++){
        aux = (double)rand()/RAND_MAX;
        if(aux < p){
            c++;
        }
    }
    return c;
}

double gaussrand(){
	static double V1, V2, S;
	static int phase = 0;
	double X;

	if(phase == 0) {
		do {
			double U1 = (double)rand() / RAND_MAX;
			double U2 = (double)rand() / RAND_MAX;

			V1 = 2 * U1 - 1;
			V2 = 2 * U2 - 1;
			S = V1 * V1 + V2 * V2;
			} while(S >= 1 || S == 0);

		X = V1 * sqrt(-2 * log(S) / S);
	} else
		X = V2 * sqrt(-2 * log(S) / S);

	phase = 1 - phase;

	return X;
}

int eventDrivenTrial(int a, int NL, int NF, int mode, double eps, double beta, double p, double q, double r, double r_L){
    double t=0, dt, P;
    int i, n_l=0, n_f=0, n_i=0, n_i_total = 0, calc_aux = 0;
    int n_l_vac = 0, n_f_vac = 0, n_i_vac=0;
    
    double re[3], R=0, rn;
    double r1=0, r2=0;
    switch (mode) 
    { 
        case 1: //printf("Model arquitecture 1"); 
                //----------------------------------------------------MODEL I-----------------------------------------------------
                n_l=NL;
                n_f=NF;

                //calc_aux = round( (1.0 - eps)*n_f );
                calc_aux = bern_binom((1.0 - eps), n_f);
                if((n_f - calc_aux) >= 0){
                    n_f -= calc_aux;
                    n_l += calc_aux;
                }
                else{
                    n_l += n_f;
                    n_f = 0;
                }
                
                break; 
        case 2: //printf("Model arquitecture 2"); 
                //-----------------------------------------------------MODEL II-----------------------------------------------------
                n_l=NL;
                n_f=NF;

                //calc_aux = round( (1.0 - eps)*n_f );
                calc_aux = bern_binom((1.0 - eps), n_f);
                if((n_f - calc_aux) >= 0){
                    n_f -= calc_aux;
                    n_f_vac += calc_aux;
                }
                else{
                    n_f_vac += n_f;
                    n_f = 0;
                }
                
                //calc_aux = round( (1.0 - eps)*n_l );
                calc_aux = bern_binom((1.0 - eps), n_l);
                if((n_l - calc_aux) >= 0){
                    n_l -= calc_aux;
                    n_l_vac += calc_aux;
                }
                else{
                    n_l_vac += n_l;
                    n_l = 0;
                }
                
                //Vaccine part with efficacy:
                t=0.0;
                while(t < T_MAX/(double)year){
                    r1 = r_L*n_l_vac; //Reactivation from Latency
                    r2 = r*n_f_vac; //Fast progress
                    R = r1 + r2;
                 
                    rn = (double)rand()/RAND_MAX; //generador();
                    while(rn<1E-24){
                        rn = (double)rand()/RAND_MAX;// (double)rand()/RAND_MAX;//generador();
                    }
                    dt = log(rn)*(-1.0/R);
                    P = (double)rand()/RAND_MAX*R; //generador()*R;

                    if(P < r1){ //Slow progress
                        if(n_l_vac>0){
                            n_l_vac -= 1;
                            n_i_vac += 1;
                        }
                    } 
                    else{
                        if(n_f_vac>0){ //Fast progress
                            n_f_vac -= 1;
                            n_i_vac += 1;
                        }
                    }
                    t+=dt;
                }
        
                break; 
        case 3: //printf("Model arquitecture 3"); 
                //-----------------------------------------------------MODEL III-----------------------------------------------------
                n_l=NL;
                n_f=NF;

                //calc_aux = round( (1.0 - eps)*n_f );
                calc_aux = bern_binom((1.0 - eps), n_f);
                if((n_f - calc_aux) >= 0){
                    n_f -= calc_aux;
                    n_f_vac += calc_aux;
                }
                else{
                    n_f_vac += n_f;
                    n_f = 0;
                }
                    
                //calc_aux = round( (1.0 - eps)*n_l );
                calc_aux = bern_binom((1.0 - eps), n_l);
                if((n_l - calc_aux) >= 0){
                    n_l -= calc_aux;
                    n_l_vac += calc_aux;
                }
                else{
                    n_l_vac += n_l;
                    n_l = 0;
                }
                
                
                //Vaccine part with efficacy:
                t=0.0;
                while(t < T_MAX/(double)year){
                    r1 = beta*q*p*n_l_vac; //Reinfection
                    r2 = r*n_f_vac; //Fast progress
                    R=r1 + r2;
                
                    rn = (double)rand()/RAND_MAX; //generador();
                    while(rn<1E-24){
                        rn = (double)rand()/RAND_MAX;// (double)rand()/RAND_MAX;//generador();
                    }
                    dt = log(rn)*(-1.0/R);
                    P = (double)rand()/RAND_MAX*R; //generador()*R;

                    if(P < r1){ //Reinfection
                        if(n_l_vac>0){
                            n_l_vac -= 1;
                            n_f_vac += 1;
                        }
                    } 
                    else{
                        if(n_f_vac>0){ //Fast progress
                            n_f_vac -= 1;
                            n_i_vac += 1;
                        }
                    }
                    
                    t+=dt;
                }
        
                break; 
        case 4: //printf("Model arquitecture 1+2"); 
                //-----------------------------------------------------MODEL I + II-----------------------------------------------------
                n_l=NL;
                n_f=NF;

                //calc_aux = round( (1.0 - eps)*n_f );
                calc_aux = bern_binom((1.0 - eps), n_f);
                if((n_f - calc_aux) >= 0){
                    n_f -= calc_aux;
                    n_l_vac += calc_aux;
                }
                else{
                    n_l_vac += n_f;
                    n_f = 0;
                }
                    
                //calc_aux = round( (1.0 - eps)*n_l );
                calc_aux = bern_binom((1.0 - eps), n_l);
                if((n_l - calc_aux) >= 0){
                    n_l -= calc_aux;
                    n_l_vac += calc_aux;
                }
                else{
                    n_l_vac += n_l;
                    n_l = 0;
                }
                
                
                //Vaccine part with efficacy:
                t=0.0;
                while(t < T_MAX/(double)year){
                    r1 = r_L*n_l_vac; //Reactivation from Latency
                    R=r1;
                
                    rn = (double)rand()/RAND_MAX; //generador();
                    while(rn<1E-24){
                        rn = (double)rand()/RAND_MAX;// (double)rand()/RAND_MAX;//generador();
                    }
                    dt = log(rn)*(-1.0/R);
                    P = (double)rand()/RAND_MAX*R; //generador()*R;
                    if(P < r1){ //Reinfection
                        if(n_l_vac>0){
                            n_l_vac -= 1;
                            n_i_vac += 1;
                        }
                    } 
                    t+=dt;
                }
                
                break;
        case 5: //printf("Model arquitecture 1+3"); 
                //-----------------------------------------------------MODEL I + III-----------------------------------------------------
        
                n_l=NL;
                n_f=NF;
 
                //calc_aux = round( (1.0 - eps)*n_f );
                calc_aux = bern_binom((1.0 - eps), n_f);
                n_f -= calc_aux;
                n_l_vac += calc_aux;
                    
                //calc_aux = round( (1.0 - eps)*n_l );
                calc_aux = bern_binom((1.0 - eps), n_l);
                n_l -= calc_aux;
                n_l_vac += calc_aux;
                
                
                //Vaccine part with efficacy:
                t=0.0;
                while(t < T_MAX/(double)year){
                    r1 = beta*q*p*n_l_vac; //Reinfection
                    r2 = r*n_f_vac; //Fast progress
                    R=r1 + r2;
                
                    rn = (double)rand()/RAND_MAX; //generador();
                    while(rn<1E-24){
                        rn = (double)rand()/RAND_MAX;// (double)rand()/RAND_MAX;//generador();
                    }
                    dt = log(rn)*(-1.0/R);
                    P = (double)rand()/RAND_MAX*R; //generador()*R;

                    if(P < r1){ //Reinfection
                        if(n_l_vac>0){
                            n_l_vac -= 1;
                            n_f_vac += 1;
                        }
                    } 
                    else{
                        if(n_f_vac>0){ //Fast progress
                            n_f_vac -= 1;
                            n_i_vac += 1;
                        }
                    }
                    
                    t+=dt;
                }
        
                break;
        case 6: //printf("Model arquitecture 2+3"); 
                //-----------------------------------------------------MODEL II + III-----------------------------------------------------
        
                n_l=NL;
                n_f=NF;

                //calc_aux = round( (1.0 - eps)*n_f );
                calc_aux = bern_binom((1.0 - eps), n_f);
                n_f -= calc_aux;
                n_f_vac += calc_aux;
                    
                //calc_aux = round( (1.0 - eps)*n_l );
                calc_aux = bern_binom((1.0 - eps), n_l);
                n_l -= calc_aux;
                n_l_vac += calc_aux;
                
                
                //Vaccine part with efficacy:
                t=0.0;
                while(t < T_MAX/(double)year){
                    r1 = r*n_f_vac; //Fast progress
                    R=r1;
                
                    rn = (double)rand()/RAND_MAX; //generador();
                    while(rn<1E-24){
                        rn = (double)rand()/RAND_MAX;// (double)rand()/RAND_MAX;//generador();
                    }
                    dt = log(rn)*(-1.0/R);
                    P = (double)rand()/RAND_MAX*R; //generador()*R;

                    if(P < r1){ //Fast progress
                        if(n_l_vac>0){
                            n_f_vac -= 1;
                            n_i_vac += 1;
                        }
                    } 
                    t+=dt;
                }
        
                break;
        case 7: //printf("Model arquitecture ALL"); 
                //-----------------------------------------------------MODEL ALL-----------------------------------------------------
        
                n_l=NL;
                n_f=NF;

                //calc_aux = round( (1.0 - eps)*n_f );
                calc_aux = bern_binom((1.0 - eps), n_f);
                n_f -= calc_aux;
                n_l_vac += calc_aux;
                    
                //calc_aux = round( (1.0 - eps)*n_l );
                calc_aux = bern_binom((1.0 - eps), n_l);
                n_l -= calc_aux;
                n_l_vac += calc_aux;
                
        
                break;
        default: //printf("Control cohort"); 
                n_l=NL;
                n_f=NF;
                break;   
    } 
    
    if(n_f!=0 || n_l!=0){
        //Vaccine part with zero efficacy, i.e, rrest of the vaccine cohort:
        t=0;
        while(t < T_MAX/(double)year){
            
            re[0] = r_L*n_l; //Reactivation from Latency
            re[1] = beta*q*p*n_l; //Reinfection in Latency
            re[2] = r*n_f; //Fast progress
                        
            R=0;
            for(i=0; i<3; i++)
                R += re[i];
                    
            rn = (double)rand()/RAND_MAX; //generador();
            while(rn<1E-24){
                rn = (double)rand()/RAND_MAX;// (double)rand()/RAND_MAX;//generador();
            }
            dt = log(rn)*(-1.0/R);
            P = (double)rand()/RAND_MAX*R; //generador()*R;

            if(P < re[0]){ //Slow progress
                if(n_l>0){
                    n_l -= 1;
                    n_i += 1;
                }
            } 
            else if( P < (re[0] + re[1]) ){ //Reinfection
                if(n_l>0){
                    n_l -= 1;
                    n_f += 1;
                }
            }
            else{
                if(n_f>0){ //Fast progress
                    n_f -= 1;
                    n_i += 1;
                }
            }
            t+=dt;
        }
        n_i_total = n_i + n_i_vac;
    }
    else{
        n_i_total = n_i_vac;
    }
    
    return n_i_total;
}








