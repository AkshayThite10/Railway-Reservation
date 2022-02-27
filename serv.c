// Using fork() for concurrent server
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "struc.h" // for structures
#define PORT 8810
// Creating structures for user, train and booking
void options(int nsd);
void login(int nsd);
void signup(int nsd);
int operations(int nsd,int type,int lid);
void trainops(int nsd);
void userops(int nsd);
int utilities(int nsd,int ch,int type,int lid);
void options(int nsd){
	int ch; // ch provided by by client
	printf("\nClient is connected\n");
	do{
        /* ch==1 : login
        ch==2 : signup
        ch==3 : exit*/
        // continues till the client wishes
		read(nsd, &ch, sizeof(int));		
		if(ch==1)
			login(nsd);
		if(ch==2)
			signup(nsd);
		if(ch==3)
			break;
	}while(1);
	close(nsd);
	printf("\nClient is disconnected\n");
}
// function for login
void login(int nsd){
	int fdu = open("database/userdb",O_RDWR);
	int lid,type,u_valid,valid;
    u_valid=0;
    valid=0;
	char pass[50];
	struct user users;
	read(nsd,&lid,sizeof(lid));   // Reading login id from client
	read(nsd,&pass,sizeof(pass)); // Reading pass
    // Locking 
	struct flock locku;
	locku.l_start = (lid-1)*sizeof(struct user);
	locku.l_len = sizeof(struct user);
	locku.l_whence = SEEK_SET;
	locku.l_pid = getpid();
	locku.l_type = F_WRLCK;
	fcntl(fdu,F_SETLKW, &locku);
	// Validating login id and pass
	while(read(fdu,&users,sizeof(users))){
		if(users.login_id==lid){
			u_valid=1;
			if(!strcmp(users.pass,pass)){
				valid = 1;
				type = users.type;
				break;
			}
			else{
				valid = 0;
				break;
			}	
		}		
		else{
			u_valid = 0;
			valid=0;
		}
	}
	// Same agent is allowed multiple logins
	if(type!=2){
		locku.l_type = F_UNLCK;
		fcntl(fdu, F_SETLK, &locku);
		close(fdu);
	}	
	// if user is valid then show operations
	if(u_valid)
	{
		write(nsd,&valid,sizeof(valid));
		if(valid){
			write(nsd,&type,sizeof(type));
			while(operations(nsd,type,lid)!=-1);
		}
	}
	else
    {
        write(nsd,&valid,sizeof(valid));
    }
	// As normal user is not allowed multiple logins
	if(type==2){
		locku.l_type = F_UNLCK;
		fcntl(fdu, F_SETLK, &locku);
		close(fdu);
	}
}
// function for signup
void signup(int nsd){
	int fdu = open("database/userdb",O_RDWR);
	int type,lid=0;
	char uname[50],pass[50];
	struct user u,temp;
    //read type, uname and pass from client
	read(nsd, &type, sizeof(type));
	read(nsd, &uname, sizeof(uname));
	read(nsd, &pass, sizeof(pass));
	int fptr = lseek(fdu, 0, SEEK_END);
    //Locking
	struct flock lk;
	lk.l_type = F_WRLCK;
	lk.l_start = fptr;
	lk.l_len = 0;
	lk.l_whence = SEEK_SET;
	lk.l_pid = getpid();
	fcntl(fdu,F_SETLKW, &lk);
    // if first user then login id = 1
    // else it is incremented by the previous value
	if(fptr==0){
		u.login_id = 1;
		strcpy(u.uname, uname);
		strcpy(u.pass, pass);
		u.type=type;
		write(fdu, &u, sizeof(u));
		write(nsd, &u.login_id, sizeof(u.login_id));
	}
	else{
		fptr = lseek(fdu, -1 * sizeof(struct user), SEEK_END);
		read(fdu, &u, sizeof(u));
		u.login_id++;
		strcpy(u.uname, uname);
		strcpy(u.pass, pass);
		u.type=type;
		write(fdu, &u, sizeof(u));
		write(nsd, &u.login_id, sizeof(u.login_id));
	}
	lk.l_type = F_UNLCK;
	fcntl(fdu, F_SETLK, &lk);
	close(fdu);
}
int operations(int nsd,int type,int lid){
	int ch,retval;
	// for admin user
	if(type==0){
		read(nsd,&ch,sizeof(ch));
		//train operations
		if(ch==1){					
			trainops(nsd);
			return operations(nsd,type,lid);	
		}
		//user operations
		else if(ch==2){				
			userops(nsd);
			return operations(nsd,type,lid);
		}
		//logout
		else if (ch ==3)				
			return -1;
	}
    // for agent and normal user
	else if(type==2 || type==1){				
		read(nsd,&ch,sizeof(ch));
		retval = utilities(nsd,ch,type,lid);
		if(retval!=5)
			return operations(nsd,type,lid);
		else if(retval==5)
			return -1;
	}		
}
int utilities(int nsd,int ch,int type,int lid){
	int valid=0;
    // book tickets
	if(ch==1){						
		trainops(nsd);
		struct flock trainlck;
		struct flock booklck;
		struct train tr;
		struct booking bk;
		int fdt = open("database/traindb", O_RDWR);
		int fdb = open("database/bookingdb", O_RDWR);
		int tid,nseats;
		read(nsd,&tid,sizeof(tid));		
		trainlck.l_type = F_WRLCK;
		trainlck.l_start = tid*sizeof(struct train);
		trainlck.l_len = sizeof(struct train);
		trainlck.l_whence = SEEK_SET;
		trainlck.l_pid = getpid();
		booklck.l_type = F_WRLCK;
		booklck.l_start = 0;
		booklck.l_len = 0;
		booklck.l_whence = SEEK_END;
		booklck.l_pid = getpid();
		fcntl(fdt, F_SETLKW, &trainlck);
		lseek(fdt,tid*sizeof(struct train),SEEK_SET);
		read(fdt,&tr,sizeof(tr));
		read(nsd,&nseats,sizeof(nseats));
		if(tr.train_number==tid)
		{		
			if(tr.available_seats>=nseats){
				valid = 1;
				tr.available_seats -= nseats;
				fcntl(fdb, F_SETLKW, &booklck);
				int fptr = lseek(fdb, 0, SEEK_END);	
				if(fptr > 0){
					lseek(fdb, -1*sizeof(struct booking), SEEK_CUR);
					read(fdb, &bk, sizeof(struct booking));
					bk.booking_id++;
				}
				else 
					bk.booking_id = 0;
				bk.type = type;
				bk.uid = lid;
				bk.tid = tid;
				bk.nseats = nseats;
				write(fdb, &bk, sizeof(struct booking));
				booklck.l_type = F_UNLCK;
				fcntl(fdb, F_SETLK, &booklck);
			 	close(fdb);
			}
		lseek(fdt, -1*sizeof(struct train), SEEK_CUR);
		write(fdt, &tr, sizeof(tr));
		}

		trainlck.l_type = F_UNLCK;
		fcntl(fdt, F_SETLK, &trainlck);
		close(fdt);
		write(nsd,&valid,sizeof(valid));
		return valid;		
	}
	// view bookings
	else if(ch==2){							
		struct flock lk;
		struct booking bk;
		int fdb = open("database/bookingdb", O_RDONLY);
		int nbook = 0;
		lk.l_type = F_RDLCK;
		lk.l_start = 0;
		lk.l_len = 0;
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();	
		fcntl(fdb, F_SETLKW, &lk);
		while(read(fdb,&bk,sizeof(bk))){
			if (bk.uid==lid)
				nbook++;
		}
		write(nsd, &nbook, sizeof(int));
		lseek(fdb,0,SEEK_SET);
		while(read(fdb,&bk,sizeof(bk))){
			if(bk.uid==lid){
				write(nsd,&bk.booking_id,sizeof(int));
				write(nsd,&bk.tid,sizeof(int));
				write(nsd,&bk.nseats,sizeof(int));
			}
		}
		lk.l_type = F_UNLCK;
		fcntl(fdb, F_SETLK, &lk);
		close(fdb);
		return valid;
	}
    // Update booking
	else if (ch==3){							
		int ch = 2,bid,val;
		utilities(nsd,ch,type,lid);
		struct booking bk;
		struct train tr;
		struct flock booklck;
		struct flock trainlck;
		int fdb = open("database/bookingdb", O_RDWR);
		int fdt = open("database/traindb", O_RDWR);
		read(nsd,&bid,sizeof(bid));
		booklck.l_type = F_WRLCK;
		booklck.l_start = bid*sizeof(struct booking);
		booklck.l_len = sizeof(struct booking);
		booklck.l_whence = SEEK_SET;
		booklck.l_pid = getpid();
		fcntl(fdb, F_SETLKW, &booklck);
		lseek(fdb,bid*sizeof(struct booking),SEEK_SET);
		read(fdb,&bk,sizeof(bk));
		lseek(fdb,-1*sizeof(struct booking),SEEK_CUR);
		trainlck.l_type = F_WRLCK;
		trainlck.l_start = (bk.tid)*sizeof(struct train);
		trainlck.l_len = sizeof(struct train);
		trainlck.l_whence = SEEK_SET;
		trainlck.l_pid = getpid();
		fcntl(fdt, F_SETLKW, &trainlck);
		lseek(fdt,(bk.tid)*sizeof(struct train),SEEK_SET);
		read(fdt,&tr,sizeof(tr));
		lseek(fdt,-1*sizeof(struct train),SEEK_CUR);
		read(nsd,&ch,sizeof(ch));
        // Add seats
		if(ch==1){							
			read(nsd,&val,sizeof(val));
			if(tr.available_seats>=val){
				valid=1;
				tr.available_seats -= val;
				bk.nseats += val;
			}
		}
        // Cancel seats
		else if(ch==2){						
			valid=1;
			read(nsd,&val,sizeof(val));
			tr.available_seats += val;
			bk.nseats -= val;	
		}
		write(fdt,&tr,sizeof(tr));
		trainlck.l_type = F_UNLCK;
		fcntl(fdt, F_SETLK, &trainlck);
		close(fdt);
		write(fdb,&bk,sizeof(bk));
		booklck.l_type = F_UNLCK;
		fcntl(fdb, F_SETLK, &booklck);
		close(fdb);
		write(nsd,&valid,sizeof(valid));
		return valid;
	}
    // Cancel booking
	else if(ch==4){							
		int ch = 2,bid;
		utilities(nsd,ch,type,lid);
		struct booking bk;
		struct train tr;
		struct flock booklck;
		struct flock trainlck;
		int fdb = open("database/bookingdb", O_RDWR);
		int fdt = open("database/traindb", O_RDWR);
		read(nsd,&bid,sizeof(bid));
		booklck.l_type = F_WRLCK;
		booklck.l_start = bid*sizeof(struct booking);
		booklck.l_len = sizeof(struct booking);
		booklck.l_whence = SEEK_SET;
		booklck.l_pid = getpid();
		fcntl(fdb, F_SETLKW, &booklck);
		lseek(fdb,bid*sizeof(struct booking),SEEK_SET);
		read(fdb,&bk,sizeof(bk));
		lseek(fdb,-1*sizeof(struct booking),SEEK_CUR);
		trainlck.l_type = F_WRLCK;
		trainlck.l_start = (bk.tid)*sizeof(struct train);
		trainlck.l_len = sizeof(struct train);
		trainlck.l_whence = SEEK_SET;
		trainlck.l_pid = getpid();
		fcntl(fdt, F_SETLKW, &trainlck);
		lseek(fdt,(bk.tid)*sizeof(struct train),SEEK_SET);
		read(fdt,&tr,sizeof(tr));
		lseek(fdt,-1*sizeof(struct train),SEEK_CUR);
		tr.available_seats += bk.nseats;
		bk.nseats = 0;
		valid = 1;
		write(fdt,&tr,sizeof(tr));
		trainlck.l_type = F_UNLCK;
		fcntl(fdt, F_SETLK, &trainlck);
		close(fdt);	
		write(fdb,&bk,sizeof(bk));
		booklck.l_type = F_UNLCK;
		fcntl(fdb, F_SETLK, &booklck);
		close(fdb);	
		write(nsd,&valid,sizeof(valid));
		return valid;	
	}
    // Logout
	else if(ch==5)										
		return 5;
}
void trainops(int nsd){
	int valid=0,tot;	
	int ch;
	read(nsd,&ch,sizeof(ch));
    // Add train
	if(ch==1){  					  	
		char tr_name[50];
		int tid = 0;
		read(nsd,&tr_name,sizeof(tr_name));
		struct train tr,temp;
		struct flock lk;
		int fdt = open("database/traindb", O_RDWR);
		tr.train_number = tid;
		strcpy(tr.train_name,tr_name);
        // We consider total available seats as 15 initially
		tr.total_seats = 15;				
		tr.available_seats = 15;
		int fptr = lseek(fdt, 0, SEEK_END); 
		lk.l_type = F_WRLCK;
		lk.l_start = fptr;
		lk.l_len = 0;
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();
		fcntl(fdt, F_SETLKW, &lk);
		if(fptr == 0){
			valid = 1;
			write(fdt, &tr, sizeof(tr));
			lk.l_type = F_UNLCK;
			fcntl(fdt, F_SETLK, &lk);
			close(fdt);
			write(nsd, &valid, sizeof(valid));
		}
		else{
			valid = 1;
			lseek(fdt, -1 * sizeof(struct train), SEEK_END);
			read(fdt, &temp, sizeof(temp));
			tr.train_number = temp.train_number + 1;
			write(fdt, &tr, sizeof(tr));
			write(nsd, &valid,sizeof(valid));	
		}
		lk.l_type = F_UNLCK;
		fcntl(fdt, F_SETLK, &lk);
		close(fdt);
	}
    // View trains
	else if(ch==2){					
		struct flock lk;
		struct train tr;
		int fdt = open("database/traindb", O_RDONLY);
		lk.l_type = F_RDLCK;
		lk.l_start = 0;
		lk.l_len = 0;
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();
		fcntl(fdt, F_SETLKW, &lk);
		int fptr = lseek(fdt, 0, SEEK_END);
		int no_of_trains = fptr / sizeof(struct train);
		write(nsd, &no_of_trains, sizeof(int));
		lseek(fdt,0,SEEK_SET);
		while(fptr != lseek(fdt,0,SEEK_CUR)){
			read(fdt,&tr,sizeof(tr));
			write(nsd,&tr.train_number,sizeof(int));
			write(nsd,&tr.train_name,sizeof(tr.train_name));
			write(nsd,&tr.total_seats,sizeof(int));
			write(nsd,&tr.available_seats,sizeof(int));
		}
		valid = 1;
		lk.l_type = F_UNLCK;
		fcntl(fdt, F_SETLK, &lk);
		close(fdt);
	}
    // Upadate train 
	else if(ch==3){					
		trainops(nsd);
		int ch,valid=0,tid;
		struct flock lk;
		struct train tr;
		int fdt = open("database/traindb", O_RDWR);
		read(nsd,&tid,sizeof(tid));
		lk.l_type = F_WRLCK;
		lk.l_start = (tid)*sizeof(struct train);
		lk.l_len = sizeof(struct train);
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();
		fcntl(fdt, F_SETLKW, &lk);
		lseek(fdt, 0, SEEK_SET);
		lseek(fdt, (tid)*sizeof(struct train), SEEK_CUR);
		read(fdt, &tr, sizeof(struct train));
		read(nsd,&ch,sizeof(int));
        // change train uname
		if(ch==1){							
			write(nsd,&tr.train_name,sizeof(tr.train_name));
			read(nsd,&tr.train_name,sizeof(tr.train_name));
			
		}
        // change total seats
		else if(ch==2){
            tot=tr.total_seats;						
			write(nsd,&tr.total_seats,sizeof(tr.total_seats));
			read(nsd,&tr.total_seats,sizeof(tr.total_seats));
            tr.available_seats=tr.available_seats+(tr.total_seats-tot);
		}
		lseek(fdt, -1*sizeof(struct train), SEEK_CUR);
		write(fdt, &tr, sizeof(struct train));
		valid=1;
		write(nsd,&valid,sizeof(valid));
		lk.l_type = F_UNLCK;
		fcntl(fdt, F_SETLK, &lk);
		close(fdt);	
	}
    // Delete train
	else if(ch==4){						
		trainops(nsd);
		struct flock lk;
		struct train tr;
		int fdt = open("database/traindb", O_RDWR);
		int tid,valid=0;
		read(nsd,&tid,sizeof(tid));
		lk.l_type = F_WRLCK;
		lk.l_start = (tid)*sizeof(struct train);
		lk.l_len = sizeof(struct train);
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();
		fcntl(fdt, F_SETLKW, &lk);	
		lseek(fdt, 0, SEEK_SET);
		lseek(fdt, (tid)*sizeof(struct train), SEEK_CUR);
		read(fdt, &tr, sizeof(struct train));
		strcpy(tr.train_name,"deleted");
		lseek(fdt, -1*sizeof(struct train), SEEK_CUR);
		write(fdt, &tr, sizeof(struct train));
		valid=1;
		write(nsd,&valid,sizeof(valid));
		lk.l_type = F_UNLCK;
		fcntl(fdt, F_SETLK, &lk);
		close(fdt);	
	}	
}
void userops(int nsd){
	int valid=0;	
	int ch;
	read(nsd,&ch,sizeof(ch));
    //Add user
	if(ch==1){    					
		char uname[50],pass[50];
		int type;
		read(nsd, &type, sizeof(type));
		read(nsd, &uname, sizeof(uname));
		read(nsd, &pass, sizeof(pass));
		struct user us;
		struct flock lk;
		int fdu = open("database/userdb", O_RDWR);
		int fptr = lseek(fdu, 0, SEEK_END);	
		lk.l_type = F_WRLCK;
		lk.l_start = fptr;
		lk.l_len = 0;
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();
		fcntl(fdu, F_SETLKW, &lk);
		if(fptr==0){
			us.login_id = 1;
			strcpy(us.uname, uname);
			strcpy(us.pass, pass);
			us.type=type;
			write(fdu, &us, sizeof(us));
			valid = 1;
			write(nsd,&valid,sizeof(int));
			write(nsd, &us.login_id, sizeof(us.login_id));
		}
		else{
			fptr = lseek(fdu, -1 * sizeof(struct user), SEEK_END);
			read(fdu, &us, sizeof(us));
			us.login_id++;
			strcpy(us.uname, uname);
			strcpy(us.pass, pass);
			us.type=type;
			write(fdu, &us, sizeof(us));
			valid = 1;
			write(nsd,&valid,sizeof(int));
			write(nsd, &us.login_id, sizeof(us.login_id));
		}
		lk.l_type = F_UNLCK;
		fcntl(fdu, F_SETLK, &lk);
		close(fdu);		
	}
    // View users (Normal and agents)
	else if(ch==2){					
		struct flock lk;
		struct user us;
		int fdu = open("database/userdb", O_RDONLY);
		lk.l_type = F_RDLCK;
		lk.l_start = 0;
		lk.l_len = 0;
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();
		fcntl(fdu, F_SETLKW, &lk);
		int fptr = lseek(fdu, 0, SEEK_END);
		int nusers = fptr / sizeof(struct user);
		nusers--;
		write(nsd, &nusers, sizeof(int));
		lseek(fdu,0,SEEK_SET);
		while(fptr != lseek(fdu,0,SEEK_CUR)){
			read(fdu,&us,sizeof(us));
			if(us.type!=0){
				write(nsd,&us.login_id,sizeof(int));
				write(nsd,&us.uname,sizeof(us.uname));
				write(nsd,&us.type,sizeof(int));
			}
		}
		valid = 1;
		lk.l_type = F_UNLCK;
		fcntl(fdu, F_SETLK, &lk);
		close(fdu);
	}
	//Update user
	else if(ch==3){					
		userops(nsd);
		int ch,valid=0,uid;
		char pass[50];
		struct flock lk;
		struct user us;
		int fdu = open("database/userdb", O_RDWR);
		read(nsd,&uid,sizeof(uid));
		lk.l_type = F_WRLCK;
		lk.l_start =  (uid-1)*sizeof(struct user);
		lk.l_len = sizeof(struct user);
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();
		fcntl(fdu, F_SETLKW, &lk);
		lseek(fdu, 0, SEEK_SET);
		lseek(fdu, (uid-1)*sizeof(struct user), SEEK_CUR);
		read(fdu, &us, sizeof(struct user));
		read(nsd,&ch,sizeof(int));
        // Update username
		if(ch==1){					
			write(nsd,&us.uname,sizeof(us.uname));
			read(nsd,&us.uname,sizeof(us.uname));
			valid=1;
			write(nsd,&valid,sizeof(valid));		
		}
        // Update password
		else if(ch==2){				
			read(nsd,&pass,sizeof(pass));
			if(!strcmp(us.pass,pass))
				valid = 1;
			write(nsd,&valid,sizeof(valid));
			read(nsd,&us.pass,sizeof(us.pass));
		}
		lseek(fdu, -1*sizeof(struct user), SEEK_CUR);
		write(fdu, &us, sizeof(struct user));
		if(valid)
			write(nsd,&valid,sizeof(valid));
		lk.l_type = F_UNLCK;
		fcntl(fdu, F_SETLK, &lk);
		close(fdu);	
	}
    // Delete user
	else if(ch==4){						
		userops(nsd);
		struct flock lk;
		struct user us;
		int fdu = open("database/userdb", O_RDWR);
		int uid,valid=0;
		read(nsd,&uid,sizeof(uid));
		lk.l_type = F_WRLCK;
		lk.l_start =  (uid-1)*sizeof(struct user);
		lk.l_len = sizeof(struct user);
		lk.l_whence = SEEK_SET;
		lk.l_pid = getpid();
		fcntl(fdu, F_SETLKW, &lk);
		lseek(fdu, 0, SEEK_SET);
		lseek(fdu, (uid-1)*sizeof(struct user), SEEK_CUR);
		read(fdu, &us, sizeof(struct user));
		strcpy(us.uname,"deleted");
		strcpy(us.pass,"");
		lseek(fdu, -1*sizeof(struct user), SEEK_CUR);
		write(fdu, &us, sizeof(struct user));
		valid=1;
		write(nsd,&valid,sizeof(valid));
		lk.l_type = F_UNLCK;
		fcntl(fdu, F_SETLK, &lk);
		close(fdu);	
	}
}
void main() {
    int sd, nsd, sz; 
    struct sockaddr_in serv, cli; 
    char buf[100]; 
    // Creating Socket
    sd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sd == -1) { 
        printf("Error in creating socket"); 
    } 
    serv.sin_family = AF_INET; 
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORT); 
    // Binding Port Address and IP Address of Server
    if (bind(sd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("Binding failed"); 
    } 
    /* Listen to the connection requests from clients
    Backlog is set to 5 */    
    listen(sd, 5);  
    sz = sizeof(cli); 
    // Running while loop for concurrent server
    while (1){
	    nsd = accept(sd, (struct sockaddr*)&cli, &sz); 
	    if (!fork()){
		    close(sd);
		    options(nsd);
		    exit(0);
	    }
	    else
	    	close(nsd);
    }
}