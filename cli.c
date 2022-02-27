#include <string.h>
#include <sys/socket.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <stdio.h>
#include <stdlib.h> 
#define PORT 8810
int welcome(int sd);
int options(int sd,int type);
int utilities(int sd,int ch);
int trainops(int sd,int ch);
int userops(int sock,int ch);
int welcome(int sd){
	int ch,valid;
	printf("\n.......Railway Ticket Reservation System.......\n");
	printf("1. Sign In\n");
	printf("2. Sign Up\n");
	printf("3. Exit\n");
	printf("Enter Desired Option: ");
	scanf("%d", &ch);
	write(sd, &ch, sizeof(ch));
    //for login
	if (ch == 1){					
		int lid,type;
		char pass[50];
		printf("login id: ");
		scanf("%d", &lid);
		strcpy(pass,getpass("password: "));
		write(sd, &lid, sizeof(lid));
		write(sd, &pass, sizeof(pass));
		read(sd, &valid, sizeof(valid));
		if(valid){
			printf("logged in successfully\n");
			read(sd,&type,sizeof(type));
			while(options(sd,type)!=-1);
			return 1;
		}
		else{
			printf("Login Failed : Incorrect pass or login id entered\n");
			return 1;
		}
	}
    //for signup
	else if(ch == 2){					
		int type,lid;
		char uname[50],pass[50],pin[6];
		system("clear");
		printf("\nEnter The Type Of Account: \n");
		printf("0 : Admin\n1 : Agent\n2 : Normal\n");
		printf("Enter Response: ");
		scanf("%d", &type);
		printf("Enter your user name: ");
		scanf("%s", uname);
		strcpy(pass,getpass("Enter your password: "));
		if(!type){
			while(1){
				strcpy(pin, getpass("Enter pin to create ADMIN account: "));
				if(strcmp(pin, "admin")!=0) 					
					printf("Invalid pin entered. Please try again.\n");
				else
					break;
			}
		}
        //Sending the type, username and password to server
		write(sd, &type, sizeof(type));
		write(sd, &uname, sizeof(uname));
		write(sd, &pass, strlen(pass));
		//Reading login id from server
		read(sd, &lid, sizeof(lid));
		printf("Your login id is : %d\n", lid);
		return 2;
	}
    // for logout
	else
    {
        return 3;
    }							
}
int options(int sd,int type){
	int ch;
    // Agent and normal user
	if(type==2 || type==1){					
		printf("1 : Book Ticket\n");
		printf("2 : View Bookings\n");
		printf("3 : Update Booking\n");
		printf("4 : Cancel booking\n");
		printf("5 : Logout\n");
		printf("Enter Option: ");
		scanf("%d",&ch);
		write(sd,&ch,sizeof(ch));
		return utilities(sd,ch);
	}
    // Admin user
	else if(type==0){					
		printf("\n1 : CRUD operations on train\n");
		printf("2 : CRUD operations on user\n");
		printf("3 : Logout\n");
		printf("Enter Option: ");
		scanf("%d",&ch);
		write(sd,&ch,sizeof(ch));
			if(ch==1){
				printf("1 : Add train\n");
				printf("2 : View train\n");
				printf("3 : Modify train\n");
				printf("4 : Delete train\n");
				printf("Enter Choice: ");
				scanf("%d",&ch);	
				write(sd,&ch,sizeof(ch));
				return trainops(sd,ch);
			}
			else if(ch==2){
				printf("1 : Add User\n");
				printf("2 : View all users\n");
				printf("3 : Modify user\n");
				printf("4 : Delete user\n");
				printf("Enter Choice: ");
				scanf("%d",&ch);
				write(sd,&ch,sizeof(ch));
				return userops(sd,ch);
			}
			else if(ch==3)
				return -1;
	}	
}
int utilities(int sd,int ch){
	int valid =0;
    // book tickets
	if(ch==1){										
		int view=2,tid,nseats;
		write(sd,&view,sizeof(int));
		trainops(sd,view);
		printf("\nEnter the train number you wish to book: ");
		scanf("%d",&tid);
		write(sd,&tid,sizeof(tid));
		printf("\nEnter number of seats: ");
		scanf("%d",&nseats);
		write(sd,&nseats,sizeof(nseats));
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nTicket is booked successfully.\n");
		else
			printf("\nSeats are not available for the entered train.\n");
		return valid;
	}
    // View bookings
	else if(ch==2){									
		int nbook;
		int lid,tid,nseats;
		read(sd,&nbook,sizeof(nbook));
		while(nbook--){
			read(sd,&lid,sizeof(lid));
			read(sd,&tid,sizeof(tid));
			read(sd,&nseats,sizeof(nseats));
			if(nseats!=0)
				printf("\nbooking id=%d\ntrain number=%d\nseats=%d\n\n",lid,tid,nseats);
		}
		return valid;
	}
    // Update booking
	else if(ch==3){									
		int ch = 2,bid,chseats,valid;
		utilities(sd,ch);
		printf("\nEnter the booking id you want to modify: ");
		scanf("%d",&bid);
		write(sd,&bid,sizeof(bid));
		printf("\n1. Increase number of seats\n2. Decrease number of seats\n");
		printf("Enter Option: ");
		scanf("%d",&ch);
		write(sd,&ch,sizeof(ch));
        // Add seats
		if(ch==1){
			printf("\nEnter number of seats to increase: ");
			scanf("%d",&chseats);
			write(sd,&chseats,sizeof(chseats));
		}
        // Cancel seats
		else if(ch==2){
			printf("\nEnter number of tickets to decrease: ");
			scanf("%d",&chseats);
			write(sd,&chseats,sizeof(chseats));
		}
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nBooking is updated successfully.\n");
		else
			printf("\nUpdation failed as seats are unavailable.\n");
		return valid;
	}
    // Cancel booking
	else if(ch==4){									
		int ch = 2,bid,valid;
		utilities(sd,ch);
		printf("\nEnter the booking id you want to cancel: ");
		scanf("%d",&bid);
		write(sd,&bid,sizeof(bid));
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nBooking is cancelled.\n");
		else
			printf("\nCancellation failed.\n");
		return valid;
	}
    // logout
	else if(ch==5)									
		return -1;
}
int trainops(int sd,int ch){
	int valid = 0;
    // Add train
	if(ch==1){				
		char tr_name[50];
		printf("Enter train name: ");
		scanf("%s",tr_name);
		write(sd, &tr_name, sizeof(tr_name));
		read(sd,&valid,sizeof(valid));	
		if(valid)
			printf("Train is added successfully\n");
		return valid;	
	}
	// View trains
	else if(ch==2){			
		int no_of_trains;
		int tno;
		char tr_name[50];
		int tseats;
		int aseats;
		read(sd,&no_of_trains,sizeof(no_of_trains));
        int c=1;
		while(no_of_trains--){
			read(sd,&tno,sizeof(tno));
			read(sd,&tr_name,sizeof(tr_name));
			read(sd,&tseats,sizeof(tseats));
			read(sd,&aseats,sizeof(aseats));	
			if(strcmp(tr_name, "deleted")!=0)
            {
                printf("\ntrain id=%d\ntrain name=%s\nseats=%d\navailable seats=%d\n\n",tno,tr_name,tseats,aseats);
            }
		}
		return valid;	
	}
	// Update train
	else if (ch==3){			
		int tseats,ch=2,valid=0,tid;
		char tr_name[50];
		write(sd,&ch,sizeof(int));
		trainops(sd,ch);
		printf("Enter the train id of train you wish to modify: ");
		scanf("%d",&tid);
		write(sd,&tid,sizeof(tid));
		
		printf("1. Train Name\n2. Total Seats of Train\n");
		printf("Enter Option: ");
		scanf("%d",&ch);
		write(sd,&ch,sizeof(ch));
		if(ch==1){
			read(sd,&tr_name,sizeof(tr_name));
			printf("\n Current name is: %s",tr_name);
			printf("\n Enter new name:");
			scanf("%s",tr_name);
			write(sd,&tr_name,sizeof(tr_name));
		}
		else if(ch==2){
			read(sd,&tseats,sizeof(tseats));
			printf("\n Current value is: %d",tseats);
			printf("\n Enter new value:");
			scanf("%d",&tseats);
			write(sd,&tseats,sizeof(tseats));
		}
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("Train data is updated successfully\n");
		return valid;
	}
    // Delete train
	else if(ch==4){				
		int ch=2,tid,valid=0;
		write(sd,&ch,sizeof(int));
		trainops(sd,ch);
		printf("Enter the train number you want to delete: ");
		scanf("%d",&tid);
		write(sd,&tid,sizeof(tid));
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("Train deleted successfully\n");
		return valid;
	}	
}
int userops(int sd,int ch){
	int valid = 0;
    // Add user
	if(ch==1){							
		int type,id;
		char name[50],password[50];
		printf("\nEnter user type: \n");
		printf("1. Agent\n2. Normal\n");
		printf("Enter Option: ");
		scanf("%d", &type);
		printf("Enter username: ");
		scanf("%s", name);
		strcpy(password,getpass("Enter Password: "));
		write(sd, &type, sizeof(type));
		write(sd, &name, sizeof(name));
		write(sd, &password, strlen(password));
		read(sd,&valid,sizeof(valid));	
		if(valid){
			read(sd,&id,sizeof(id));
			printf("Login id of the created user is: %d\n", id);
		}
		return valid;	
	}
	// View users (Normal and agents)
	else if(ch==2){						
		int no_of_users;
		int id,type;
		char uname[50];
		read(sd,&no_of_users,sizeof(no_of_users));
		while(no_of_users--){
			read(sd,&id,sizeof(id));
			read(sd,&uname,sizeof(uname));
			read(sd,&type,sizeof(type));
			
			if(strcmp(uname, "deleted")!=0)
				printf("\nuser id=%d\nuser name=%s\nuser type=%d\n\n",id,uname,type);
		}

		return valid;	
	}
    // Update user
	else if (ch==3){						
		int ch=2,valid=0,uid;
		char name[50],pass[50];
		write(sd,&ch,sizeof(int));
		userops(sd,ch);
		printf("Enter the login id of user to be updated: ");
		scanf("%d",&uid);
		write(sd,&uid,sizeof(uid));
		printf("1. Username\n2. Password\n");
		printf("Enter Option: ");
		scanf("%d",&ch);
		write(sd,&ch,sizeof(ch));
		if(ch==1){
			read(sd,&name,sizeof(name));
			printf("\nCurrent name is: %s",name);
			printf("\nEnter new name:");
			scanf("%s",name);
			write(sd,&name,sizeof(name));
			read(sd,&valid,sizeof(valid));
		}
		else if(ch==2){
			printf("\nEnter current password: ");
			scanf("%s",pass);
			write(sd,&pass,sizeof(pass));
			read(sd,&valid,sizeof(valid));
			if(valid){
				printf("\nEnter new password:");
				scanf("%s",pass);
			}
			else
				printf("\nIncorrect password entered\n");
			write(sd,&pass,sizeof(pass));
		}
		if(valid){
			read(sd,&valid,sizeof(valid));
			if(valid)
				printf("User data updated successfully\n");
		}
		return valid;
	}
	// Delete a user
	else if(ch==4){						
		int ch=2,uid,valid=0;
		write(sd,&ch,sizeof(int));
		userops(sd,ch);	
		printf("\nEnter the login id you want to delete: ");
		scanf("%d",&uid);
		write(sd,&uid,sizeof(uid));
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nUser deleted successfully");
		return valid;
	}
}
void main() { 
    int sd; 
    struct sockaddr_in serv; 
    char server_reply[50],*serv_ip;
    serv_ip = "127.0.0.1"; 
    sd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sd == -1) { 
        printf("\nSocket creation failed\n"); 
    } 
    serv.sin_addr.s_addr = inet_addr(serv_ip); 
    serv.sin_family = AF_INET; 
    serv.sin_port = htons(PORT); 
    if (connect(sd, (struct sockaddr*)&serv, sizeof(serv)) < 0){
        perror("Could not connect to server"); 
    } 
    while(welcome(sd)!=3);
    close(sd); 
} 
