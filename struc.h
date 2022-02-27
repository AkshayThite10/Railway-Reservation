struct user
{
		int login_id;
		char pass[50];
		char uname[50];
		int type;
};
struct train
{
		int train_number;
		char train_name[50];
		int total_seats;
		int available_seats;
};
struct booking
{
		int booking_id;
		int type;
		int uid;
		int tid;
		int nseats;
};
