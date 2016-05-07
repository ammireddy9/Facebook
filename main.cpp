#include<stdio.h>
#include<string.h>
#include<conio.h>
#include<stdlib.h>

#pragma warning(disable: 4996) 

int sum = 0, empty = 0;  //size of elements filled in cache, empty for next pos in the cache table
int cache_table[] = { -1, -1, -1, -1, -1, -1, -1, -1 };  //initial cache table
char **cache = (char**)calloc(8, sizeof(char*));   

/* first_time_msg() and first_time_comm() is the functions for both view,insert,del of msg and comment's */

/*all group related functions*/
void view_groups(FILE *fp); //it displays all groups 
void info_abt_group(FILE *fp, long off); //it display information about the specific group at off
void group_display_frame(FILE *fp, char usr[]); //it displays all groups and asks you to select 
void group_selection_frame(FILE *fp, int grp_no, char usr[], int last_pos[]);  //it displays the information about specified group by it's no
void modify_grp_info(FILE *fp, int grp_no, char usr[], int last_pos[]); //it is used to modify group name and description
long get_free_blk(int no_of_blocks, FILE *fp);  //it used to get the offset from bit vector of size 128*no of blocks

/*all bit vector related functions*/
void off_bits(FILE *fp, long pos, int count);  //it is used to make bits to zero in bit vector for pos  to pos+count
void on_bit(FILE *fp, long pos); //to put one on the specific bit on bit vector

/*all message related functions*//*last_pos[] gives the last position to insert*/
void add_message(FILE *fp, int grp_no, char usr[], int last_pos[]); //To add message by direct or indirect 
void view_messages(FILE *fp, int grp_no, char usr[], int last_pos[]);  //To view all messages both direct and indirect
void delete_message(FILE *fp, int grp_no, int msg_no, char usr[]); //To delete the specific message by its user selection no
long first_time_msg(FILE *fp, struct group* grp, int last_pos[], int type);  //it is executed when each time we enter  in to group
void position_last_pos(int last_pos[], int range);  //it is position the last empty location of the file
void msg_selection_frame(FILE *fp, int msg_no, int grp_no, char usr[], int last_pos[]); //it displays frame for like,comment,del msg for msg
void like_message(FILE *fp, int grp_no, int msg_no, char usr[]); //it is used to add like to a specific msg

/*all comment related functions*/
void comments(FILE *fp, int grp_no, int msg_no, char usr[], int last_pos[]);  // it displays option to add comm,view
void view_comments(FILE *fp, int msg_off, char usr[], int last_pos[]);  // it displays comments of the specific msg_off
long first_time_comm(FILE *fp, struct message* msg, int last_pos[], int type); ///it is executed when each time we enter  in to message
void comm_selection_frame(FILE *fp, int msg_off, char usr[], int comm_no, int last_pos[]); //it displays optns for like comm,del comm
void delete_comment(FILE *fp, int msg_off, char usr[], int comm_no); //it is used to del the comment
void like_comment(FILE *fp, int msg_off, char usr[], int comm_no); // it is used to like the comment
void add_comment(FILE *fp, int msg_off, char usr[], int last_pos[]); //it is used to add the new comment

/*all cache related functions*/
void fwrite1(void *str, int size, int cnt_size, FILE *fp, int offset, int type); //it is used in place of all fseek and fwrite combo
void fread1(void *str, int size, int cnt_size, FILE *fp, int offset, int type); //it is used in place of all fseek and fread combo
void write_into_file(FILE *fp, int type, int offset, void *s); // write data to cache and file , write through
int check_struct(int offset);  // check fun to check wheather offset is in cache or not
int read_into_cache(FILE *fp, int type); // to take from file to cache
int change(FILE *fp, int type);  // used to change cache
int replace(FILE *fp, int type);  // used to fifo


struct group
{
	int direct[10];
	int indirect[4];
	int in_indirect;
	char desp[50];
	char name[10];
	int no_of_user;
	int no_of_msg;
};

struct group_indirect
{
	int direct[64];
};

struct group_in_indirect
{
	int indirect[64];
};

struct message
{
	int direct[10];
	int indirect[4];
	int in_indirect;
	int like;
	char name[8];
	int no_of_comm;
	char msg[180];
};

struct message_indirect
{
	int direct[32];
};

struct message_in_indirect
{
	int indirect[32];
};

struct comment
{
	char name[16];
	int like;
	char data[108];
};

int write_into_cache_file(void *s, int type, int offset, FILE *fp){
	if (type == 0){					//type=0->group
		struct group *grp=NULL;
		grp = (struct group *)s;
		memcpy(grp, s, sizeof(struct group*));
		cache[empty] = (char *)malloc(sizeof(struct group));
		memset(cache[empty], 0, sizeof(struct group));
		memcpy(cache[empty], grp, sizeof(struct group));
		sum += 128;
		fseek(fp, offset, SEEK_SET);
		fwrite(grp, sizeof(struct group), 1, fp);
		empty = (empty + 1) % 8;
		if (empty == 0){
			return 7;
		}
		return empty - 1;
	}
	else if (type == 1){				//type=1->message
		struct message *msg=NULL;
		msg = (struct message *)s;
		memcpy(msg,s,sizeof(struct message*));
		cache[empty] = (char *)malloc(sizeof(struct message));
		memset(cache[empty], 0, sizeof(struct message));
		memcpy(cache[empty], msg, sizeof(struct message));
		cache[(empty + 1) % 8] = NULL;
		sum += 256;
		fseek(fp, offset, SEEK_SET);
		fwrite(msg, sizeof(struct message), 1, fp);
		empty = (empty + 2) % 8;
		if (empty == 0){
			return 6;
		}
	}
	else if (type == 2){				//type=2->comment
		struct comment *comm=NULL;
		comm = (struct comment *)s;
		memcpy(comm,s,sizeof(struct comment*));
		cache[empty] = (char *)malloc(sizeof(struct comment));
		memset(cache[empty], 0, sizeof(struct comment));
		memcpy(cache[empty], comm, sizeof(struct comment));
		sum += 128;
		fseek(fp, offset, SEEK_SET);
		fwrite(comm, sizeof(struct comment), 1, fp);
		empty = (empty + 1) % 8;
		if (empty == 0){
			return 7;
		}
		return empty - 1;
	}
	else if (type == 3){			//type=3->group_indirect
		struct group_indirect *gi=NULL;
		gi = (struct group_indirect *)s;
		memcpy(gi,s,sizeof(struct group_indirect*));
		cache[empty] = (char *)malloc(sizeof(struct group_indirect));
		memset(cache[empty], 0, sizeof(struct group_indirect));
		memcpy(cache[empty], gi, sizeof(struct group_indirect));
		cache[(empty + 1) % 8] = NULL;
		sum += 256;
		fseek(fp, offset, SEEK_SET);
		fwrite(gi, sizeof(struct group_indirect), 1, fp);
		empty = (empty + 2) % 8;
		if (empty == 0){
			return 6;
		}
	}
	else if (type == 4){			//type=4->message_indirect
		struct message_indirect *mi=NULL;
		mi = (struct message_indirect *)s;
		memcpy(mi,s,sizeof(message_indirect*));
		cache[empty] = (char *)malloc(sizeof(struct message_indirect));
		memset(cache[empty], 0, sizeof(struct message_indirect));
		memcpy(cache[empty], mi, sizeof(struct message_indirect));
		sum += 128;
		fseek(fp, offset, SEEK_SET);
		fwrite(mi, sizeof(struct message_indirect), 1, fp);
		empty = (empty + 1) % 8;
		if (empty == 0){
			return 7;
		}
		return empty - 1;
	}
	return 0;
}

void fwrite1(void *str, int size, int cnt_size, FILE *fp, int offset, int type)
{
	int i = check_struct(offset);
	if ((i) != -1){
		cache_table[i] = -1;
		if (type == 1){
			cache_table[(i + 1) % 8] = -1;
		}
		if (type == 3){
			cache_table[(i + 1) % 8] = -1;
		}
	}
	write_into_cache_file(str, type, offset, fp);
}

void copy_cache(void *str, int type, int off){
	if (type == 0){
		struct group grp;
		memcpy(&grp, cache[off], sizeof(grp));
		memcpy(str, &grp, sizeof(grp));
	}
	else if (type == 1){
		struct message msg;
		memcpy(&msg, cache[off], sizeof(msg));
		memcpy(str, &msg, sizeof(msg));
	}
	else if (type == 2){
		struct comment comm;
		memcpy(&comm, cache[off], sizeof(comm));
		memcpy(str, &comm, sizeof(comm));
	}
	else if (type == 3){
		struct group_indirect gi;
		memcpy(&gi, cache[off], sizeof(gi));
		memcpy(str, &gi, sizeof(gi));
	}
	else if (type == 4){
		struct message_indirect mi;
		memcpy(&mi, cache[off], sizeof(mi));
		memcpy(str, &mi, sizeof(mi));
	}
}

void fread1(void *str, int size, int cnt_size, FILE *fp, int offset, int type){
	int i = check_struct(offset), off;
	if ((i) != -1){
		copy_cache(str, type, i);
	}
	else{
		fseek(fp, offset, SEEK_SET);
		off = read_into_cache(fp, type);
		copy_cache(str, type, off);
	}
}

int check_struct(int offset){
	int i = 0;
	for (i = 0; i < 8; i++){
		if (cache_table[i] == offset){
			return i;
		}
	}
	return -1;
}

int read_into_cache(FILE *fp, int type){
	if (sum >= 1024){
		return replace(fp, type);
	}
	return change(fp, type);
}


int change(FILE *fp, int type){//type -0 group
	if (type == 0){//0-group
		struct group g;
		cache_table[empty] = ftell(fp);
		fread(&g, sizeof(g), 1, fp);
		cache[empty] = (char*)malloc(128 * sizeof(char));
		memcpy(cache[empty], &g, sizeof(g));
		sum += 128;
		empty = (empty + 1) % 8;
		if (empty == 0){
			return 7;
		}
		return empty - 1;
	}
	else if (type == 1){//1-message
		struct message m;
		cache_table[empty] = ftell(fp);
		cache_table[(empty + 1) % 8] = ftell(fp);
		fseek(fp, cache_table[empty], SEEK_SET);
		fread(&m, sizeof(m), 1, fp);
		cache[empty] = (char*)malloc(256 * sizeof(char));
		memcpy(cache[empty], &m, sizeof(m));
		cache[(empty + 1) % 8] = NULL;
		sum += 256;
		empty = (empty + 2) % 8;
		if (empty == 0){
			return 6;
		}
		else if (empty == 1){
			return 7;
		}
		return empty - 2;
	}
	else if (type == 2){//type 2 - comment
		struct comment c;
		cache_table[empty] = ftell(fp);
		fseek(fp, cache_table[empty], SEEK_SET);
		fread(&c, sizeof(c), 1, fp);
		cache[empty] = (char*)malloc(128 * sizeof(char));
		memcpy(cache[empty], &c, sizeof(c));
		sum += 128;
		empty = (empty + 1) % 8;
		if (empty == 0){
			return 7;
		}
		return empty - 1;
	}
	else if (type == 3){
		struct group_indirect gi;
		cache_table[empty] = ftell(fp);
		cache_table[(empty + 1) % 8] = ftell(fp);
		fseek(fp, cache_table[empty], SEEK_SET);
		fread(&gi, sizeof(gi), 1, fp);
		cache[empty] = (char*)malloc(256 * sizeof(char));
		memcpy(cache[empty], &gi, sizeof(gi));
		cache[(empty + 1) % 8] = NULL;
		sum += 256;
		empty = (empty + 2) % 8;
		if (empty == 0){
			return 6;
		}
		else if (empty == 1){
			return 7;
		}
		return empty - 2;
	}
	else if (type == 4){
		struct message_indirect mi;
		cache_table[empty] = ftell(fp);
		fseek(fp, cache_table[empty], SEEK_SET);
		fread(&mi, sizeof(mi), 1, fp);
		cache[empty] = (char*)malloc(128 * sizeof(char));
		memcpy(cache[empty], &mi, sizeof(mi));
		sum += 128;
		empty = (empty + 1) % 8;
		if (empty == 0){
			return 7;
		}
		return empty - 1;
	}
	return 0;
}

int replace(FILE *fp, int type){
	if (cache_table[empty] == cache_table[(empty + 1) % 8]){
		cache_table[(empty + 1) % 8] = -1;
	}
	return change(fp, type);
}

/*last_pos[] gives the last position to insert*/
void add_message(FILE *fp, int grp_no, char usr[], int last_pos[])
{
	struct group grp;
	struct group_indirect gi;
	struct group_in_indirect gii;
	struct message msg, msg1;
	fread1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
	memset(&msg, 0, sizeof(struct message));
	printf("\nEnter message: ");
	char s[30];
	fflush(stdin);
	gets(s);
	strcpy(msg.msg, s);
	strcpy(msg.name, usr);
	if (last_pos[0] == -1)	first_time_msg(fp, &grp, last_pos, 0);
	position_last_pos(last_pos, 64);
	if (last_pos[0] == 0)
	{
		fread1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
		grp.direct[last_pos[2]] = 1048580 + get_free_blk(2, fp);
		fwrite1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
		fwrite1(&msg, sizeof(msg), 1, fp, grp.direct[last_pos[2]], 1);
		last_pos[2]++;
	}
	else if (last_pos[0] == 1)
	{
		struct group_indirect grpi;
		fread1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
		if (last_pos[2] == 0)
		{
			grp.indirect[last_pos[1]] = 1048580 + get_free_blk(2, fp);
			fwrite1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
			memset(&grpi, 0, sizeof(struct group_indirect));
			fwrite1(&grpi, sizeof(grpi), 1, fp, grp.indirect[last_pos[1]], 3);
		}
		fread1(&grpi, sizeof(grpi), 1, fp, grp.indirect[last_pos[1]], 3);
		grpi.direct[last_pos[2]] = 1048580 + get_free_blk(2, fp);
		fwrite1(&msg, sizeof(msg), 1, fp, grpi.direct[last_pos[2]], 1);
		fwrite1(&grpi, sizeof(grpi), 1, fp, grp.indirect[last_pos[1]], 3);
		last_pos[2]++;
	}
	else
	{
		struct group_indirect grpi;
		struct group_in_indirect grpii;
		fseek(fp, grp_no * 128 + 1048580, SEEK_SET);
		fread(&grp, sizeof(grp), 1, fp);
		fseek(fp, grp.in_indirect, SEEK_SET);
		if (last_pos[1] == 0)
		{
			memset(&grpii, 0, sizeof(struct group_in_indirect));
			fwrite(&grpii, sizeof(grpii), 1, fp);
		}
		fseek(fp, grp.in_indirect, SEEK_SET);
		fread(&grpii, sizeof(grpii), 1, fp);
		fseek(fp, grpii.indirect[last_pos[1]], SEEK_SET);
		if (last_pos[2] == 0)
		{
			memset(&grpi, 0, sizeof(struct group_indirect));
			fwrite(&grpi, sizeof(grpi), 1, fp);
		}
		fseek(fp, grpii.indirect[last_pos[1]], SEEK_SET);
		fread(&grpi, sizeof(grpi), 1, fp);
		grpi.direct[last_pos[2]] = 1048580 + get_free_blk(2, fp);
		fseek(fp, grpi.direct[last_pos[2]], SEEK_SET);
		fwrite(&msg, sizeof(msg), 1, fp);
		last_pos[2]++;
	}
	group_selection_frame(fp, grp_no + 1, usr, last_pos);
}


void on_bit(FILE *fp, long pos)
{
	int byte_no = pos / 8;
	pos = pos % 8;
	char ch;
	fseek(fp, byte_no + 3, SEEK_SET);
	fread(&ch, sizeof(ch), 1, fp);
	ch |= (128 >> pos);
	fseek(fp, byte_no + 3, SEEK_SET);
	fwrite(&ch, sizeof(ch), 1, fp);
}


void initialize_groups(FILE *fp)
{
	for (int pos = 0; pos<8; pos++)
	{
		on_bit(fp, pos);
	}
	long off = 1048580;
	for (int i = 0; i<8; i++)
	{
		struct group grp;
		memset(&grp, 0, sizeof(struct group));
		long temp = off + (128 * i);
		fseek(fp, temp, SEEK_SET);
		char s[30];
		scanf("%9[^\n]s", s);
		fflush(stdin);
		char de[50];
		scanf("%49[^\n]s", de);
		strcpy(grp.name, s);
		grp.no_of_user = 0;
		grp.no_of_msg = 0;
		strcpy(grp.desp, de);
		for (int i = 0; i<10; i++)
			grp.direct[i] = 0;
		for (int i = 0; i<4; i++)
			grp.indirect[i] = 0;
		grp.in_indirect = 0;
		fseek(fp, temp, SEEK_SET);
		fwrite(&grp, sizeof(grp), 1, fp);
		fflush(stdin);
	}
}


void initialize(FILE* fp)
{
	char name[3] = "pb";
	fwrite(&name, sizeof(name), 1, fp);
	char ch = 0;
	for (int i = 0; i<1048575; i++)
	{
		fwrite(&ch, sizeof(ch), 1, fp);
	}
}


void view_groups(FILE *fp)
{
	struct group grp;
	long off = 1048580;
	for (int i = 0; i<8; i++)
	{
		fseek(fp, i * 128 + off, SEEK_SET);
		fread(&grp, sizeof(grp), 1, fp);
		printf("%d.%s\n", i + 1, grp.name);
	}
}

void info_abt_group(FILE *fp, long off)
{
	struct group grp;
	fread1(&grp, sizeof(grp), 1, fp, off, 0);
	printf("\nName: %s\nDescription: %s\nNo.of messages: %d\nNo.of users: %d", grp.name, grp.desp, grp.no_of_msg, grp.no_of_user);
}




void group_selection_frame(FILE *fp, int grp_no, char usr[], int last_pos[])
{
	char ch;
	grp_no--;
	info_abt_group(fp, grp_no * 128 + 1048580);
	printf("\n1.To modify group info\n2.Add Message\n3.View Message\n4.Back\nEnter choice: ");
	scanf("%d", &ch);
	if (ch == 1)   modify_grp_info(fp, grp_no, usr, last_pos);
	else if (ch == 2)  add_message(fp, grp_no, usr, last_pos);
	else if (ch == 3)  view_messages(fp, grp_no, usr, last_pos);
	else if (ch == 4)	group_display_frame(fp, usr);
}

long get_free_blk(int no_of_blocks, FILE* fp)
{
	char byte;
	int count = 0, pos = -1;
	fseek(fp, 3, SEEK_SET);
	for (long i = 0; i<1048577; i++)
	{
		fread(&byte, sizeof(byte), 1, fp);
		for (int j = 0; j<8; j++)
		{
			if ((byte&(128 >> j)) == 0)
				count++;
			else
				count = 0;
			pos++;
			if (count == no_of_blocks)
			{
				while (count--)
				{
					on_bit(fp, pos);
					pos--;
				}
				pos++;
				return 128 * (pos);
			}
		}
	}
	return 0;
}



long first_time_comm(FILE *fp, struct message* msg, int last_pos[], int type)
{
	struct message_indirect mi;
	struct message_in_indirect mii;
	int counter = 1;
	struct comment comm;
	for (int i = 0; i<10; i++)
	{
		if (msg->direct[i] != 0)
		{
			if (type == 0)
			{
				fread1(&comm, sizeof(comm), 1, fp, msg->direct[i], 2);
				printf("%d.%s\ncomment: %s\nno.of likes: %d\n", counter, comm.name, comm.data, comm.like);
			}
			if (counter == type)   return msg->direct[i];
			if (counter == -1 * type)
			{
				int temp = msg->direct[i];
				msg->direct[i] = 0;
				return temp;
			}
			last_pos[0] = 0; last_pos[1] = 0; last_pos[2] = i + 1;
			counter++;
		}
	}
	for (int i = 0; i<4; i++)
	{
		if (msg->indirect[i] != 0)
		{
			fread1(&mi, sizeof(mi), 1, fp, msg->indirect[i], 4);
			for (int j = 0; j<32; j++)
			{
				if (mi.direct[j] != 0)
				{
					if (type == 0)
					{
						fread1(&comm, sizeof(comm), 1, fp, mi.direct[j], 2);
						printf("%d.%s\ncomment: %s\nno.of likes: %d\n", counter, comm.name, comm.data, comm.like);
					}
					if (counter == type)   return mi.direct[j];
					if (counter == -1 * type)
					{
						int temp = mi.direct[j];
						mi.direct[j] = 0;
						fwrite1(&mi, sizeof(mi), 1, fp, msg->indirect[i], 4);
						return temp;
					}
					last_pos[0] = 1; last_pos[1] = i; last_pos[2] = j + 1;
					counter++;
				}
			}
		}
	}
	if (msg->in_indirect != 0)
	{
		fseek(fp, msg->in_indirect, SEEK_SET);
		fread(&mii, sizeof(mii), 1, fp);
		for (int i = 0; i<32; i++)
		{
			if (mii.indirect[i] != 0)
			{
				fseek(fp, mii.indirect[i], SEEK_SET);
				fread(&mi, sizeof(mi), 1, fp);
				for (int j = 0; j<64; j++)
				{
					if (mi.direct[j] != 0)
					{
						if (type == 0)
						{
							fseek(fp, mi.direct[i], SEEK_SET);
							fread(&comm, sizeof(comm), 1, fp);
							printf("%d.%s\ncomment: %s\nno.of likes: %d\n", counter, comm.name, comm.data, comm.like);
						}
						if (counter == type)   return msg->direct[i];
						if (counter == -1 * type)
						{
							int temp = msg->direct[i];
							msg->direct[i] = 0;
							return temp;
						}
						last_pos[0] = 2; last_pos[1] = i; last_pos[2] = j + 1;
						counter++;
					}
				}
			}
		}
	}
	if (last_pos[0] == -1)
	{
		last_pos[0] = 0; last_pos[1] = 0; last_pos[2] = 0;
	}
	return 0;
}


long first_time_msg(FILE *fp, struct group* grp, int last_pos[], int type)
{

	struct group_indirect gi;
	struct group_in_indirect gii;
	int counter = 1;
	struct message msg;
	for (int i = 0; i<10; i++)
	{
		if (grp->direct[i] != 0)
		{
			if (type == 0)
			{
				fread1(&msg, sizeof(msg), 1, fp, grp->direct[i], 1);
				printf("%d.%s\nmessage: %s\nno.of likes: %d\n", counter, msg.name, msg.msg, msg.like);
			}
			if (counter == type)   return grp->direct[i];
			if (counter == -1 * type)
			{
				int temp = grp->direct[i];
				grp->direct[i] = 0;
				return temp;
			}
			last_pos[0] = 0; last_pos[1] = 0; last_pos[2] = i;
			counter++;
		}
	}
	for (int i = 0; i<4; i++)
	{
		if (grp->indirect[i] != 0)
		{
			fread1(&gi, sizeof(gi), 1, fp, grp->indirect[i], 3);
			for (int j = 0; j<64; j++)
			{
				if (gi.direct[j] != 0)
				{
					if (type == 0)
					{
						fread1(&msg, sizeof(msg), 1, fp, gi.direct[j], 1);
						printf("%d.%s\nmessage: %s\nno.of likes: %d\n", counter, msg.name, msg.msg, msg.like);
					}
					if (counter == type)   return gi.direct[j];
					if (counter == -1 * type)
					{
						int temp = gi.direct[j];
						gi.direct[j] = 0;
						fwrite1(&gi, sizeof(gi), 1, fp, grp->indirect[i], 0);
						return temp;
					}
					last_pos[0] = 1; last_pos[1] = i; last_pos[2] = j;
					fwrite1(&gi, sizeof(gi), 1, fp, grp->indirect[i], 3);
					counter++;
				}
			}
		}
	}
	if (grp->in_indirect != 0)
	{
		fseek(fp, grp->in_indirect, SEEK_SET);
		fread(&gii, sizeof(gii), 1, fp);
		for (int i = 0; i<64; i++)
		{
			if (gii.indirect[i] != 0)
			{
				fseek(fp, gii.indirect[i], SEEK_SET);
				fread(&gi, sizeof(gi), 1, fp);
				for (int j = 0; j<64; j++)
				{
					if (gi.direct[j] != 0)
					{
						if (type == 0)
						{
							fseek(fp, gi.direct[j], SEEK_SET);
							fread(&msg, sizeof(msg), 1, fp);
							printf("%d.%s\nmessage: %s\nno.of likes: %d\n", counter, msg.name, msg.msg, msg.like);
						}
						if (counter == type)   return gi.direct[i];
						if (counter == -1 * type)
						{
							int temp = gi.direct[i];
							gi.direct[i] = 0;
							return temp;
						}
						last_pos[0] = 1; last_pos[1] = i; last_pos[2] = j;
						fseek(fp, grp->indirect[i], SEEK_SET);
						fread(&gi, sizeof(gi), 1, fp);
						counter++;
					}
				}
			}
		}
	}
	if (last_pos[0] == -1)
	{
		last_pos[0] = 0; last_pos[1] = 0; last_pos[2] = 0;
	}
	return 0;
}

void position_last_pos(int last_pos[], int range)
{
	if (last_pos[0] == 0)
	{
		if (last_pos[2]>9)
		{
			last_pos[0]++;
			last_pos[1] = 0;
			last_pos[2] = 0;
		}
	}
	else if (last_pos[0] == 1)
	{
		if (last_pos[2]>range)
		{
			last_pos[2] = 0;
			last_pos[1]++;
		}
		if (last_pos[1]>4)
		{
			last_pos[0] = 2;
			last_pos[1] = 0;
			last_pos[2] = 0;
		}
	}
	else
	{
		if (last_pos[2]>range)
		{
			last_pos[2] = 0;
			last_pos[1]++;
		}
	}
}

void like_message(FILE *fp, int grp_no, int msg_no, char usr[])
{
	struct group grp;
	struct message msg;
	int last_pos[] = { -1,-1,-1};
	fread1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
	long change = first_time_msg(fp, &grp, last_pos, msg_no);
	fread1(&msg, sizeof(msg), 1, fp, change, 1);
	msg.like++;
	fwrite1(&msg, sizeof(msg), 1, fp, change, 1);
	view_messages(fp, grp_no, usr, last_pos);
}

void add_comment(FILE *fp, int msg_off, char usr[], int last_pos[])
{
	struct message msg;
	fread1(&msg, sizeof(msg), 1, fp, msg_off, 1);
	struct comment comm;
	memset(&comm, 0, sizeof(struct comment));
	printf("\nEnter comment: ");
	char s[30];
	fflush(stdin);
	scanf("%[^\n]s", s);
	comm.like = 0;
	strcpy(comm.data, s);
	strcpy(comm.name, usr);
	if (last_pos[0] == -1)
	{
		first_time_comm(fp, &msg, last_pos, 0);
	}
	position_last_pos(last_pos, 32);
	if (last_pos[0] == 0)
	{
		msg.direct[last_pos[2]] = 1048580 + get_free_blk(1, fp);
		fwrite1(&comm, sizeof(comm), 1, fp, msg.direct[last_pos[2]], 2);
		last_pos[2]++;
	}
	else if (last_pos[0] == 1)
	{
		struct message_indirect msgi;
		fread1(&msg, sizeof(msg), 1, fp, msg_off, 1);
		if (last_pos[2] == 0)
		{
			msg.indirect[last_pos[1]] = 1048580 + get_free_blk(1, fp);
			memset(&msgi, 0, sizeof(struct message_indirect));
			fwrite1(&msg, sizeof(msg), 1, fp, msg_off, 1);
			fwrite1(&msgi, sizeof(msgi), 1, fp, msg.indirect[last_pos[1]], 4);
		}
		fread1(&msgi, sizeof(msgi), 1, fp, msg.indirect[last_pos[1]], 4);
		msgi.direct[last_pos[2]] = 1048580 + get_free_blk(1, fp);
		fwrite1(&comm, sizeof(comm), 1, fp, msgi.direct[last_pos[2]], 2);
		fwrite1(&msgi, sizeof(msgi), 1, fp, msg.indirect[last_pos[1]], 4);
		last_pos[2]++;
	}
	fwrite1(&msg, sizeof(msg), 1, fp, msg_off, 1);
}

void like_comment(FILE *fp, int msg_off, char usr[], int comm_no)
{
	int last_pos[] = { -1, -1, -1 };
	struct message msg;
	struct comment comm;
	fread1(&msg, sizeof(msg), 1, fp, msg_off, 1);
	long change = first_time_comm(fp, &msg, last_pos, comm_no);
	fread1(&comm, sizeof(comm), 1, fp, change, 2);
	comm.like++;
	fwrite1(&comm, sizeof(comm), 1, fp, change, 2);
}

void delete_comment(FILE *fp, int msg_off, char usr[], int comm_no)
{
	struct message msg;
	fread1(&msg, sizeof(msg), 1, fp, msg_off, 1);
	int last_pos[] = { -1, -1, -1 };
	long change = first_time_comm(fp, &msg, last_pos, -1 * comm_no);
	if (change != 0)	off_bits(fp, (change - 1048580) / 128, 1);
	fwrite1(&msg, sizeof(msg), 1, fp, msg_off, 1);
}


void comm_selection_frame(FILE *fp, int msg_off, char usr[], int comm_no, int last_pos[])
{
	printf("1.Like\n2.Delete Comment\n3.Back\nEnter Choice: ");
	int ch;
	scanf("%d", &ch);
	switch (ch)
	{
		case 1: like_comment(fp, msg_off, usr, comm_no); break;
		case 2: delete_comment(fp, msg_off, usr, comm_no);
		case 3: view_comments(fp, msg_off, usr, last_pos); break;
	}
	comm_selection_frame(fp, msg_off, usr, comm_no, last_pos);
}

void view_comments(FILE *fp, int msg_off, char usr[], int last_pos[])
{
	struct message msg;
	fread1(&msg, sizeof(msg), 1, fp, msg_off, 1);
	struct comment comm;
	first_time_comm(fp, &msg, last_pos, 0);
	printf("\nEnter choice or 0 to go back: ");
	int ch;
	scanf("%d", &ch);
	if (ch>0)
	{
		long change = first_time_comm(fp, &msg, last_pos, ch);
		fread1(&comm, sizeof(comm), 1, fp, change, 2);
		printf("\n%s\ncomment : %s\nno.of likes: %d\n", comm.name, comm.data, comm.like);
		fwrite1(&msg, sizeof(msg), 1, fp, msg_off, 1);
		comm_selection_frame(fp, msg_off, usr, ch, last_pos);
	}
	group_display_frame(fp, usr);
}

void comments(FILE *fp, int grp_no, int msg_no, char usr[], int last_pos[])
{
	struct group grp;
	struct message msg;
	fread1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580,0 );
	long change = first_time_msg(fp, &grp, last_pos, msg_no);
	fread1(&msg, sizeof(msg), 1, fp, change, 1);
	printf("1.Add Comment\n2.View Comment\n3.Back\nEnter Choice: ");
	int ch;
	scanf("%d", &ch);
	switch (ch)
	{
		case 1:add_comment(fp, change, usr, last_pos); break;
		case 2:view_comments(fp, change, usr, last_pos); break;
		case 3:msg_selection_frame(fp, msg_no, grp_no, usr, last_pos); break;
	}
	comments(fp, grp_no, msg_no, usr, last_pos);
}

void off_bits(FILE *fp, long pos, int count)
{
	char c;
	int byte_no = pos / 8;
	pos = pos % 8;
	fseek(fp, 3 + byte_no, SEEK_SET);
	fread(&c, sizeof(char), 1, fp);
	while (count--)
	{
		c &= ~(128 >> pos);
		pos++;
		if (pos == 8)
		{
			fseek(fp, 3 + byte_no, SEEK_SET);
			fwrite(&c, sizeof(char), 1, fp);
			byte_no++;
			fseek(fp, 3 + byte_no, SEEK_SET);
			fread(&c, sizeof(char), 1, fp);
			pos = 0;
		}
	}
	fseek(fp, 3 + byte_no, SEEK_SET);
	fwrite(&c, sizeof(char), 1, fp);
}

void delete_message(FILE *fp, int grp_no, int msg_no, char usr[])
{
	struct group grp;
	int last_pos[] = { -1, -1, -1 };
	fread1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
	long change = first_time_msg(fp, &grp, last_pos, -1 * msg_no);
	fwrite1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
	if (change != 0)	off_bits(fp, (change - 1048580) / 128, 2);
	fwrite1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
}

void msg_selection_frame(FILE *fp, int msg_no, int grp_no, char usr[], int last_pos[])
{
	int comm_last_pos[] = { -1, -1, -1 };
	printf("\n1.Like\n2.Comment\n3.Delete Message\n4.Back\nEnter choice: ");
	int ch;
	scanf("%d", &ch);
	if (ch == 1)   like_message(fp, grp_no, msg_no, usr);
	else if (ch == 2)  comments(fp, grp_no, msg_no, usr, comm_last_pos);
	else if (ch == 3)  delete_message(fp, grp_no, msg_no, usr);
	else if (ch == 4)  view_messages(fp, grp_no, usr, last_pos);
	view_messages(fp, grp_no, usr, last_pos);
}



void view_messages(FILE *fp, int grp_no, char usr[], int last_pos[])
{
	struct group grp;
	struct message msg;
	fread1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
	int tem[] = { -1, -1, -1 };
	first_time_msg(fp, &grp, tem, 0);
	printf("\nEnter choice or 0 to go back: ");
	int ch;
	scanf("%d", &ch);
	if (ch>0)
	{
		long change = first_time_msg(fp, &grp, tem, ch);
		fread1(&msg, sizeof(msg), 1, fp, change, 1);
		printf("\n%s\nmessage: %s\nno.of likes: %d\n", msg.name, msg.msg, msg.like);
		msg_selection_frame(fp, ch, grp_no, usr, last_pos);
	}
	else        group_selection_frame(fp, grp_no + 1, usr, last_pos);
	fwrite1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
	group_selection_frame(fp, grp_no + 1, usr, last_pos);
}


void group_display_frame(FILE *fp, char usr[])
{
	int last_pos[3] = { -1, -1, -1 };
	view_groups(fp);
	printf("Select Choice: ");
	int grp_no;
	scanf("%d", &grp_no);
	if (grp_no<0 && grp_no>10) group_display_frame(fp, usr);
	else
	{
		group_selection_frame(fp, grp_no, usr, last_pos);
	}
}


void modify_grp_info(FILE *fp, int grp_no, char usr[], int last_pos[])
{
	int ch;
	struct group grp;
	char s[40];
	fread1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
	printf("\n1.Modify group name\n2.Modify group descprition\n3.Back\nEnter choice: ");
	scanf("%d", &ch);
	fflush(stdin);
	if (ch == 1)
	{
		printf("\nEnter new name: ");
		scanf("%[^\n]s", s);
		strcpy(grp.name, s);
		fwrite1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
		group_selection_frame(fp, grp_no + 1, usr, last_pos);
	}
	else if (ch == 2)
	{
		printf("\nEnter new descprition: ");
		scanf("%[^\n]s", s);
		strcpy(grp.desp, s);
		fwrite1(&grp, sizeof(grp), 1, fp, grp_no * 128 + 1048580, 0);
		group_selection_frame(fp, grp_no + 1, usr, last_pos);
	}
	else if (ch == 3)
	{
		group_selection_frame(fp, grp_no + 1, usr, last_pos);
	}
	else modify_grp_info(fp, grp_no + 1, usr, last_pos);
}


int main()
{
	char usr[30];
	FILE* fp;
	fp = fopen("fs.bin", "wb");
	initialize(fp);
	fclose(fp);
	fp = fopen("fs.bin", "rb+");
	initialize_groups(fp);
	fclose(fp);
	fp = fopen("fs.bin", "rb+");
	printf("Enter user name: ");
	fflush(stdin);
	scanf(" %[^\n]s", usr);
	group_display_frame(fp, usr);
	return 0;
}
